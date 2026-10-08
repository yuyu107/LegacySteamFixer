#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "client_map.h"
#include "friends_map.h"
/* Freestanding MinGW DLL linker requires the TLS index symbol. No TLS variables are used. */
DWORD _tls_index;
#ifdef _WIN64
#define THIS
#define ORIGNAME "steam_api64_original.dll"
#define CLIENTNAME "steamclient64.dll"
#else
#define THIS __attribute__((thiscall))
#define ORIGNAME "steam_api_original.dll"
#define CLIENTNAME "steamclient.dll"
#endif
typedef struct {void **vt; void *original;} Object;
typedef struct RegistryNode {void *(__cdecl *create)(void); const char *name; struct RegistryNode *next;} RegistryNode;
typedef void *(__cdecl *Factory)(const char *,int *);
typedef void *(THIS *Getter)(void *,int,int,const char *);
static HMODULE self, original_api;
static CRITICAL_SECTION cs;
static char logpath[MAX_PATH];
static Object *client;
static RegistryNode node;
static Object *core_client;
static RegistryNode core_node;
static int prepared;
typedef USHORT (WINAPI *CaptureFrames)(ULONG,ULONG,PVOID*,PULONG);
static CaptureFrames capture_frames;
static PVOID exception_observer;

static struct {void *original; int kind; Object *object;} cache[64];
static int cache_count;
/* A regular PE import forces the original DLL to load before this proxy's
   forwarded exports are resolved, including on the Windows 7 loader. */
__declspec(dllimport) int __cdecl SteamAPI_GetHSteamPipe(void);
static int (__cdecl * volatile original_dependency)(void);
/* Keep the bridge independent of VC/UCRT installation on legacy Windows. */
void *memcpy(void *d,const void *s,size_t n){unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d;}
int memcmp(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;x++;y++;}return 0;}
int strcmp(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return (unsigned char)*a-(unsigned char)*b;}
char *strcpy(char *d,const char *s){char *r=d;while((*d++=*s++));return r;}
char *strcat(char *d,const char *s){char *r=d;while(*d)d++;strcpy(d,s);return r;}
char *strrchr(const char *s,int c){const char *r=NULL;do{if(*s==c)r=s;}while(*s++);return (char*)r;}
static void logmsg(const char *fmt,...) {
 char buf[2048];va_list args;va_start(args,fmt);int n=wvsprintfA(buf,fmt,args);va_end(args);
 if(n<0)return;if(n>(int)sizeof(buf)-3)n=sizeof(buf)-3;buf[n++]='\r';buf[n++]='\n';
 HANDLE h=CreateFileA(logpath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h!=INVALID_HANDLE_VALUE){DWORD written;WriteFile(h,buf,n,&written,NULL);CloseHandle(h);}
}
/* Diagnostic observation only: first-chance faults may be handled by Unity.
   Always continue normal exception dispatch; never turn a failure into success. */
static void * volatile last_target;
void *memset(void *d,int c,size_t n){unsigned char *p=d;while(n--)*p++=(unsigned char)c;return d;}
static void log_exit_stack(LPVOID reserved){
 /* No heap allocation, symbol engine, synchronization or module loading here.
    A detach stack is best-effort; forced termination can omit this callback. */
 void *frames[48];
 logmsg("PROCESS_DETACH kind=%s thread=%lu tick=%lu last_forward_target=%p",reserved?"process-exit":"dll-unload",GetCurrentThreadId(),GetTickCount(),last_target);
 if(!capture_frames){logmsg("EXIT_STACK unavailable: RtlCaptureStackBackTrace missing");return;}
 USHORT n=capture_frames(0,48,frames,NULL);
 logmsg("EXIT_STACK frames=%u (shutdown observation, not proof of original trigger)",(unsigned)n);
 for(USHORT i=0;i<n;i++){
  MEMORY_BASIC_INFORMATION m;char name[MAX_PATH];name[0]=0;void *base=NULL;
  if(VirtualQuery(frames[i],&m,sizeof(m))){base=m.AllocationBase;GetModuleFileNameA((HMODULE)base,name,MAX_PATH);}
  logmsg("EXIT_FRAME %u pc=%p module=%s base=%p offset=%p",(unsigned)i,frames[i],name,base,(void*)((uintptr_t)frames[i]-(uintptr_t)base));
 }
}

#include "unity_quit_trace.h"
static LONG WINAPI observe_exception(EXCEPTION_POINTERS *e){
 DWORD c=e->ExceptionRecord->ExceptionCode;
 if(c==EXCEPTION_BREAKPOINT && is_unity_quit_break(e->ExceptionRecord->ExceptionAddress))return observe_unity_quit(e);
 if(c!=EXCEPTION_ACCESS_VIOLATION && c!=EXCEPTION_ILLEGAL_INSTRUCTION && c!=EXCEPTION_STACK_OVERFLOW && c!=EXCEPTION_IN_PAGE_ERROR)return EXCEPTION_CONTINUE_SEARCH;
 static LONG busy;if(InterlockedCompareExchange(&busy,1,0))return EXCEPTION_CONTINUE_SEARCH;
 MEMORY_BASIC_INFORMATION m;char name[MAX_PATH];name[0]=0;
 void *pc=e->ExceptionRecord->ExceptionAddress;void *base=NULL;
 if(VirtualQuery(pc,&m,sizeof(m))){base=m.AllocationBase;GetModuleFileNameA((HMODULE)base,name,MAX_PATH);}
 logmsg("FIRST_CHANCE fault code=%08lx pc=%p module=%s base=%p offset=%p thread=%lu last_forward_target=%p (may be handled by game)",c,pc,name,base,(void*)((uintptr_t)pc-(uintptr_t)base),GetCurrentThreadId(),last_target);
#ifdef _WIN64
 logmsg("Registers RIP=%p RSP=%p RCX=%p RDX=%p R8=%p R9=%p",(void*)e->ContextRecord->Rip,(void*)e->ContextRecord->Rsp,(void*)e->ContextRecord->Rcx,(void*)e->ContextRecord->Rdx,(void*)e->ContextRecord->R8,(void*)e->ContextRecord->R9);
#endif
 if(e->ExceptionRecord->NumberParameters>=2)logmsg("Fault operation=%p address=%p",(void*)e->ExceptionRecord->ExceptionInformation[0],(void*)e->ExceptionRecord->ExceptionInformation[1]);
 InterlockedExchange(&busy,0);return EXCEPTION_CONTINUE_SEARCH;
}
#include "exit_trace.h"
/* Tail thunks preserve Microsoft's original argument and return ABI, including
   CSteamID hidden result pointers. Only the interface's this pointer changes. */
static void *thunk(void *obj,void *target) {
 unsigned char *p=VirtualAlloc(NULL,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!p)return NULL;
#ifdef _WIN64
 p[0]=0x48;p[1]=0xb9;memcpy(p+2,&obj,8);
 void *record=&last_target;
 p[10]=0x49;p[11]=0xbb;memcpy(p+12,&record,8);
 p[20]=0x48;p[21]=0xb8;memcpy(p+22,&target,8);
 p[30]=0x49;p[31]=0x89;p[32]=0x03;p[33]=0xff;p[34]=0xe0;
#else
 p[0]=0xb9;memcpy(p+1,&obj,4);p[5]=0xb8;memcpy(p+6,&target,4);p[10]=0xff;p[11]=0xe0;
#endif
 DWORD protect;if(!VirtualProtect(p,64,PAGE_EXECUTE_READ,&protect)){VirtualFree(p,0,MEM_RELEASE);return NULL;}
 FlushInstructionCache(GetCurrentProcess(),p,64);return p;
}
static Object *newobject(void *orig,int count,const int *map,int direct_count) {
 Object *o=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*o));if(!o)return NULL;
 o->vt=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,count*sizeof(void*));if(!o->vt){HeapFree(GetProcessHeap(),0,o);return NULL;}o->original=orig;
 for(int i=0;i<count;i++)if(i<direct_count){o->vt[i]=thunk(orig,(*(void***)orig)[map?map[i]:i]);if(!o->vt[i])return NULL;}
 return o;
}
static void *query(Object *,int,int,const char *);
static void *timeline_vt[24];
static Object timeline;
static uint64_t THIS timeline_noop(Object *o){(void)o;logmsg("Timeline004 no-op call thread=%lu",GetCurrentThreadId());return 0;}
static void *THIS generic(Object *o,int u,int p,const char *v){return query(o,u,p,v);}
static uint32_t THIS ugc_count(Object *o,int include_disabled){(void)include_disabled;typedef uint32_t(THIS *F)(void*);return ((F)(*(void***)o->original)[74])(o->original);}
static uint32_t THIS ugc_items(Object *o,uint64_t *items,uint32_t cap,int include_disabled){(void)include_disabled;typedef uint32_t(THIS *F)(void*,uint64_t*,uint32_t);return ((F)(*(void***)o->original)[75])(o->original,items,cap);}
static int THIS unsupported_items3(Object *o,void *ids,uint32_t n,int flag){(void)o;(void)ids;(void)n;(void)flag;logmsg("Unsupported: SetItemsDisabledLocally");return 0;}
static int THIS unsupported_items2(Object *o,void *ids,uint32_t n){(void)o;(void)ids;(void)n;logmsg("Unsupported: new UGC item operation");return 0;}
static int THIS unsupported_item1(Object *o,uint64_t id){(void)o;(void)id;logmsg("Unsupported: MarkDownloadedItemAsUnused");return 0;}
static uint32_t THIS unsupported_count(Object *o){(void)o;logmsg("Unsupported: new item count");return 0;}
static int THIS beta_count(Object *o,int *available,int *priv){(void)o;if(available)*available=0;if(priv)*priv=0;logmsg("Unsupported: GetNumBetas");return 0;}
static int THIS beta_info(Object *o,int i,uint32_t *flags,uint32_t *build,char *name,int size,char *desc,int dsize,uint32_t *updated){(void)o;(void)i;if(flags)*flags=0;if(build)*build=0;if(updated)*updated=0;if(name&&size>0)*name=0;if(desc&&dsize>0)*desc=0;logmsg("Unsupported: GetBetaInfo");return 0;}
static int THIS beta_set(Object *o,const char *name){(void)o;(void)name;logmsg("Unsupported: SetActiveBeta");return 0;}
static void THIS performance(Object *o,int mode){(void)o;(void)mode;logmsg("Unsupported: SetGamePerformanceSetting");}
static void THIS resolution(Object *o,uint32_t w,uint32_t h){(void)o;(void)w;(void)h;logmsg("Unsupported: SetGameRenderResolution");}
static int THIS remote_bool0(Object *o){(void)o;return 0;}
static int THIS remote_bool1(Object *o,uint32_t s){(void)o;(void)s;return 0;}
static int THIS remote_avatar(Object *o,uint32_t s){(void)o;(void)s;return -1;}
static void THIS remote_void0(Object *o){(void)o;}
static uint32_t THIS remote_input(Object *o,void *out,uint32_t n){(void)o;(void)out;(void)n;return 0;}
static void THIS remote_mousevisible(Object *o,uint32_t s,int v){(void)o;(void)s;(void)v;}
static void THIS remote_mousepos(Object *o,uint32_t s,float x,float y){(void)o;(void)s;(void)x;(void)y;}
static uint32_t THIS remote_cursor(Object *o,int w,int h,int hx,int hy,const void *pixels,int pitch){(void)o;(void)w;(void)h;(void)hx;(void)hy;(void)pixels;(void)pitch;return 0;}
static void THIS remote_setcursor(Object *o,uint32_t s,uint32_t c){(void)o;(void)s;(void)c;}
static int THIS remote_show(Object *o){typedef unsigned char(THIS *F)(void*,int);return ((F)(*(void***)o->original)[6])(o->original,1);}
static Object *adapt(void *orig,int kind) {
 EnterCriticalSection(&cs);
 for(int i=0;i<cache_count;i++)if(cache[i].original==orig&&cache[i].kind==kind){Object *o=cache[i].object;LeaveCriticalSection(&cs);return o;}
 if(cache_count==64){LeaveCriticalSection(&cs);return NULL;}
 Object *o=NULL;
 if(kind==1)o=newobject(orig,sizeof(friends_map)/sizeof(int),friends_map,sizeof(friends_map)/sizeof(int));
 if(kind==2){o=newobject(orig,35,NULL,30);if(o){o->vt[30]=beta_count;o->vt[31]=beta_info;o->vt[32]=beta_set;o->vt[33]=performance;o->vt[34]=resolution;}}
 if(kind==3){o=newobject(orig,99,NULL,94);if(o){o->vt[74]=ugc_count;o->vt[75]=ugc_items;o->vt[94]=unsupported_items3;o->vt[95]=unsupported_items2;o->vt[96]=unsupported_item1;o->vt[97]=unsupported_count;o->vt[98]=unsupported_items2;}}
 if(kind==5){o=newobject(orig,15,NULL,8);if(o){o->vt[6]=remote_show;o->vt[8]=remote_bool0;o->vt[9]=remote_void0;o->vt[10]=remote_input;o->vt[11]=remote_mousevisible;o->vt[12]=remote_mousepos;o->vt[13]=remote_cursor;o->vt[14]=remote_setcursor;}}
 if(kind==4){o=newobject(orig,20,NULL,0);if(o){int m[]={0,1,-1,2,-1,-1,-1,-1,3,4,5,-1,7};for(int i=0;i<13;i++)if(m[i]>=0)o->vt[i]=thunk(orig,(*(void***)orig)[m[i]]);o->vt[2]=remote_bool1;o->vt[4]=remote_bool1;o->vt[5]=remote_avatar;o->vt[6]=remote_avatar;o->vt[7]=remote_avatar;o->vt[11]=remote_show;o->vt[13]=remote_bool0;o->vt[14]=remote_void0;o->vt[15]=remote_input;o->vt[16]=remote_mousevisible;o->vt[17]=remote_mousepos;o->vt[18]=remote_cursor;o->vt[19]=remote_setcursor;}}
 if(o){cache[cache_count].original=orig;cache[cache_count].kind=kind;cache[cache_count++].object=o;}
 LeaveCriticalSection(&cs);return o;
}
static void *query(Object *o,int user,int pipe,const char *version) {
 if(!version)return NULL;
 if(!strcmp(version,"STEAMTIMELINE_INTERFACE_V004")){for(int i=0;i<24;i++)timeline_vt[i]=timeline_noop;timeline.vt=timeline_vt;timeline.original=NULL;logmsg("Timeline004 unavailable backend: no-op adapter");return &timeline;}
 const char *old=version;int kind=0;
 if(!strcmp(version,"SteamFriends018")){old="SteamFriends017";kind=1;}
 else if(!strcmp(version,"STEAMAPPS_INTERFACE_VERSION009")||!strcmp(version,"SteamApps009")){old="STEAMAPPS_INTERFACE_VERSION008";kind=2;}
 else if(!strcmp(version,"STEAMUGC_INTERFACE_VERSION021")||!strcmp(version,"SteamUGC021")){old="STEAMUGC_INTERFACE_VERSION020";kind=3;}
 else if(!strcmp(version,"STEAMREMOTEPLAY_INTERFACE_VERSION003")||!strcmp(version,"SteamRemotePlay003")){old="STEAMREMOTEPLAY_INTERFACE_VERSION002";kind=5;}
 else if(!strcmp(version,"STEAMREMOTEPLAY_INTERFACE_VERSION004")||!strcmp(version,"SteamRemotePlay004")){old="STEAMREMOTEPLAY_INTERFACE_VERSION002";kind=4;}
 void *raw=((Getter)(*(void***)o->original)[12])(o->original,user,pipe,old);
 if(!raw){logmsg("Interface unavailable: %s (requested fallback %s) user=%d pipe=%d",version,old,user,pipe);return NULL;}
 if(kind){Object *adapted=adapt(raw,kind);logmsg("Adapter: %s -> %s %p",version,old,(void*)adapted);return adapted;}
 return raw;
}
static void * __cdecl instantiate(void){return client;}
static void * __cdecl instantiate_core(void){return core_client;}
static int setup(void) {
 EnterCriticalSection(&cs);if(prepared){int r=prepared==1;LeaveCriticalSection(&cs);return r;}
 logmsg("LegacySteam Generic Bridge 0.4.0-test5; pointer bits=%u pid=%lu",(unsigned)(8*sizeof(void*)),GetCurrentProcessId());
 
 capture_frames=(CaptureFrames)GetProcAddress(GetModuleHandleA("ntdll.dll"),"RtlCaptureStackBackTrace");
 logmsg("Exit stack observer=%s",capture_frames?"ready":"unavailable");
 char path[MAX_PATH];GetModuleFileNameA(self,path,sizeof(path));char *slash=strrchr(path,'\\');if(!slash)goto fail;strcpy(slash+1,ORIGNAME);
 original_api=LoadLibraryExA(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);if(!original_api){logmsg("Original API load failed: %lu",GetLastError());goto fail;}
 HMODULE sc=GetModuleHandleA(CLIENTNAME);
 if(!sc){HKEY key;DWORD size=sizeof(path),type;if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Valve\\Steam",0,KEY_READ,&key)!=ERROR_SUCCESS)goto fail;
 LONG err=RegQueryValueExA(key,"SteamPath",NULL,&type,(BYTE*)path,&size);RegCloseKey(key);if(err!=ERROR_SUCCESS||type!=REG_SZ||size==0||size>=sizeof(path)-32)goto fail;path[sizeof(path)-1]=0;
 strcat(path,"\\" CLIENTNAME);sc=LoadLibraryExA(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);if(!sc){logmsg("Steam client load failed: %lu",GetLastError());goto fail;}}
 Factory factory=(Factory)GetProcAddress(sc,"CreateInterface");if(!factory)goto fail;
 if(factory("SteamClient023",NULL)){logmsg("Native SteamClient023 present; adapter unnecessary");prepared=1;LeaveCriticalSection(&cs);return 1;}
 void *raw=factory("SteamClient021",NULL);if(!raw){logmsg("SteamClient021 missing");goto fail;}
 void *raw_core=factory("SteamClient017",NULL);if(!raw_core){logmsg("Internal SteamClient017 missing");goto fail;}
 /* Strict factory byte signatures from the two supplied November 2024 DLLs. */
 unsigned char *fn=(unsigned char*)factory;RegistryNode **head=NULL;
#ifdef _WIN64
 const BYTE sig[]={0x4c,0x8b,0x0d};const BYTE suffix[]={0x4c,0x8b,0xd2,0x4c,0x8b,0xd9};
 if(memcmp(fn,sig,3)||memcmp(fn+7,suffix,6)){logmsg("Unknown x64 CreateInterface layout; refusing modification");goto fail;}
 int32_t disp;memcpy(&disp,fn+3,4);head=(RegistryNode**)(fn+7+disp);
#else
 const BYTE sig[]={0x55,0x8b,0xec,0x56,0x8b,0x35};
 if(memcmp(fn,sig,6)){logmsg("Unknown x86 CreateInterface layout; refusing modification");goto fail;}
 memcpy(&head,fn+6,4);
#endif
 MEMORY_BASIC_INFORMATION mbi;if(!VirtualQuery(head,&mbi,sizeof(mbi))||mbi.State!=MEM_COMMIT||!(*head)){logmsg("Invalid interface registry");goto fail;}
 client=newobject(raw,sizeof(client_map)/sizeof(int),client_map,sizeof(client_map)/sizeof(int));if(!client)goto fail;
 /* Getter slots all share (this,user,pipe,version), except GetISteamUtils. */
 const int slots[]={5,6,8,10,11,12,13,14,15,16,17,18,23,24,25,26,27,31,32,33,34,35,36};
 for(unsigned i=0;i<sizeof(slots)/sizeof(int);i++)client->vt[slots[i]]=generic;
 /* The SDK uses an internal SteamClient017 object, distinct from the
    public SteamClient023 accessor. Preserve its own layout and ABI. */
 core_client=newobject(raw_core,36,NULL,36);if(!core_client)goto fail;
 core_client->vt[8]=generic;core_client->vt[12]=generic;core_client->vt[15]=generic;
 node.create=instantiate;node.name="SteamClient023";node.next=*head;
 core_node.create=instantiate_core;core_node.name="SteamClient017";core_node.next=&node;
 DWORD protect;if(!VirtualProtect(head,sizeof(void*),PAGE_READWRITE,&protect))goto fail;
 InterlockedExchangePointer((void *volatile*)head,&core_node);DWORD ignored;VirtualProtect(head,sizeof(void*),protect,&ignored);
 if(factory("SteamClient023",NULL)!=client){logmsg("Registry verification failed");goto fail;}
 logmsg("SteamClient023 and internal SteamClient017 bridges registered in GAME process; Steam installation unchanged");prepared=1;LeaveCriticalSection(&cs);return 1;
fail: prepared=-1;logmsg("Setup failed; Win32 error=%lu",GetLastError());LeaveCriticalSection(&cs);return 0;
}
static char *validation_versions(const char *versions) {
 if(!versions)return NULL;
 char *out=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,16384);if(!out)return NULL;
 unsigned offset=0;
 while(*versions){unsigned len=0;while(len<512&&versions[len])len++;if(len==512||offset+len+2>16384){HeapFree(GetProcessHeap(),0,out);return NULL;}
  const char *v=versions;
  if(!strcmp(v,"SteamFriends018"))v="SteamFriends017";
  else if(!strcmp(v,"STEAMAPPS_INTERFACE_VERSION009"))v="STEAMAPPS_INTERFACE_VERSION008";
  else if(!strcmp(v,"STEAMUGC_INTERFACE_VERSION021"))v="STEAMUGC_INTERFACE_VERSION020";
  else if(!strcmp(v,"STEAMREMOTEPLAY_INTERFACE_VERSION004")||!strcmp(v,"STEAMREMOTEPLAY_INTERFACE_VERSION003"))v="STEAMREMOTEPLAY_INTERFACE_VERSION002";
  else if(!strcmp(v,"STEAMTIMELINE_INTERFACE_V004")){logmsg("Init validation: skip unsupported Timeline004; no-op adapter remains available");versions+=len+1;continue;}
  if(v!=versions)logmsg("Init validation: %s -> %s",versions,v);
  strcpy(out+offset,v);offset+=len+1;versions+=len+1;
 }out[offset]=0;return out;
}
__declspec(dllexport) int __cdecl BridgeInit(const char *versions,char *error) {
 if(!setup()){if(error)strcpy(error,"LegacySteam TEST3 setup failed; see LegacySteam-AutoBridge.log");return 2;}
 typedef int(__cdecl *F)(const char*,char*);F f=(F)GetProcAddress(original_api,"SteamInternal_SteamAPI_Init");if(!f)return 2;
 char *check=validation_versions(versions);if(versions&&!check){if(error)strcpy(error,"LegacySteam interface validation list is invalid or allocation failed");return 2;}
 int r=f(check,error);if(check)HeapFree(GetProcessHeap(),0,check);logmsg("SteamInternal_SteamAPI_Init result=%d message=%s",r,(r!=0&&error)?error:"");return r;
}
__declspec(dllexport) void *__cdecl BridgeCreate(const char *v) {
 if(!setup())return NULL;typedef void *(__cdecl *F)(const char*);F f=(F)GetProcAddress(original_api,"SteamInternal_CreateInterface");return f?f(v):NULL;
}
__declspec(dllexport) int __cdecl BridgeInitFlat(char *error) {
 if(!setup()){if(error)strcpy(error,"LegacySteam TEST3 setup failed");return 2;}typedef int(__cdecl *F)(char*);F f=(F)GetProcAddress(original_api,"SteamAPI_InitFlat");return f?f(error):2;
}
__declspec(dllexport) void *__cdecl BridgeTimeline(void){if(!setup())return NULL;logmsg("Timeline004 SDK accessor intercepted");return query(core_client,0,0,"STEAMTIMELINE_INTERFACE_V004");}
__declspec(dllexport) void *__cdecl BridgeFindUser(int user,const char *version){if(!setup())return NULL;if(version&&!strcmp(version,"STEAMTIMELINE_INTERFACE_V004")){logmsg("Timeline004 FindOrCreateUserInterface intercepted");return query(core_client,user,0,version);}typedef void *(__cdecl *F)(int,const char*);F f=(F)GetProcAddress(original_api,"SteamInternal_FindOrCreateUserInterface");return f?f(user,version):NULL;}
__declspec(dllexport) int __cdecl BridgeInitSafe(void) {
 if(!setup())return 0;typedef int(__cdecl *F)(void);F f=(F)GetProcAddress(original_api,"SteamAPI_InitSafe");return f?f():0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved){(void)reserved;if(reason==DLL_PROCESS_ATTACH){original_dependency=SteamAPI_GetHSteamPipe;self=module;DisableThreadLibraryCalls(module);InitializeCriticalSection(&cs);GetModuleFileNameA(NULL,logpath,sizeof(logpath));char *p=strrchr(logpath,'\\');if(p)strcpy(p+1,"LegacySteam-AutoBridge.log");}else if(reason==DLL_PROCESS_DETACH){if(!reserved){restore_unity_quit();restore_exit_observers();}if(exception_observer)RemoveVectoredExceptionHandler(exception_observer);log_exit_stack(reserved);}return TRUE;}
