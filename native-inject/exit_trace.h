/* Diagnostic IAT observation. No on-disk patch, instruction detour, exit veto,
   or symbol loading. Snapshot covers modules loaded when Steam setup finishes. */
#include <tlhelp32.h>
static void (WINAPI *real_exit_process)(UINT);
static BOOL (WINAPI *real_terminate_process)(HANDLE,UINT);
static void (WINAPI *real_rtl_exit)(LONG);
static void (__cdecl *real_crt_exit)(int),(__cdecl *real_crt_fast_exit)(int);
static struct {void **slot;void *old;void *replacement;} exit_patches[512];
static unsigned exit_patch_count;
static __attribute__((noinline)) void trace_exit_call(const char *api,UINT code){
 void *frames[48];
 logmsg("EXIT_CALL api=%s code=%08x thread=%lu tick=%lu last_forward_target=%p",api,code,GetCurrentThreadId(),GetTickCount(),last_target);
 if(!capture_frames){logmsg("CALL_STACK unavailable");return;}
 USHORT n=capture_frames(0,48,frames,NULL);
 for(USHORT i=0;i<n;i++){
  MEMORY_BASIC_INFORMATION m;char name[MAX_PATH];name[0]=0;void *base=NULL;
  if(VirtualQuery(frames[i],&m,sizeof(m))){base=m.AllocationBase;GetModuleFileNameA((HMODULE)base,name,MAX_PATH);}
  logmsg("CALL_FRAME %u pc=%p module=%s base=%p offset=%p",(unsigned)i,frames[i],name,base,(void*)((uintptr_t)frames[i]-(uintptr_t)base));
 }
}
static void WINAPI observed_exit_process(UINT code){trace_exit_call("ExitProcess",code);real_exit_process(code);}
static BOOL WINAPI observed_terminate_process(HANDLE p,UINT code){
 if(p==GetCurrentProcess() || GetProcessId(p)==GetCurrentProcessId())trace_exit_call("TerminateProcess(self)",code);
 return real_terminate_process(p,code);
}
static void WINAPI observed_rtl_exit(LONG code){trace_exit_call("RtlExitUserProcess",(UINT)code);real_rtl_exit(code);}
static void __cdecl observed_crt_exit(int code){trace_exit_call("CRT exit",(UINT)code);real_crt_exit(code);}
static void __cdecl observed_crt_fast_exit(int code){trace_exit_call("CRT _exit",(UINT)code);real_crt_fast_exit(code);}
static int image_range(size_t size,DWORD rva,size_t bytes){return rva<size && bytes<=size-rva;}
static void patch_exit_imports(HMODULE mod,const char *path){
 if(mod==self || mod==GetModuleHandleA("kernel32.dll") || mod==GetModuleHandleA("kernelbase.dll") || mod==GetModuleHandleA("ntdll.dll"))return;
 BYTE *b=(BYTE*)mod;IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)b;
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<=0 || dos->e_lfanew>0x100000)return;
 IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64*)(b+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return;
 size_t size=nt->OptionalHeader.SizeOfImage;
 IMAGE_DATA_DIRECTORY d=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
 if(!image_range(size,d.VirtualAddress,d.Size) || d.Size<sizeof(IMAGE_IMPORT_DESCRIPTOR))return;
 for(size_t di=0;di+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=d.Size;di+=sizeof(IMAGE_IMPORT_DESCRIPTOR)){
  IMAGE_IMPORT_DESCRIPTOR *imp=(IMAGE_IMPORT_DESCRIPTOR*)(b+d.VirtualAddress+di);
  if(!imp->Name)break;if(!imp->OriginalFirstThunk)continue;
  for(size_t ti=0;ti<size/8;ti++){
   size_t names=(size_t)imp->OriginalFirstThunk+ti*8,slots=(size_t)imp->FirstThunk+ti*8;
   if(names>0xFFFFFFFF || slots>0xFFFFFFFF || !image_range(size,(DWORD)names,8)||!image_range(size,(DWORD)slots,8))break;
   ULONGLONG rva=*(ULONGLONG*)(b+names);if(!rva)break;if(IMAGE_SNAP_BY_ORDINAL64(rva)||rva>0xFFFFFFFF||!image_range(size,(DWORD)rva,3))continue;
   char *name=(char*)(b+(DWORD)rva+2);size_t remain=size-(DWORD)rva-2,n=0;while(n<remain && n<128 && name[n])n++;if(n==remain||n==128)continue;
   void **slot=(void**)(b+slots);void *old=*slot,*replacement=NULL;
   if(!strcmp(name,"ExitProcess") && old==(void*)real_exit_process)replacement=observed_exit_process;
   else if(!strcmp(name,"TerminateProcess") && old==(void*)real_terminate_process)replacement=observed_terminate_process;
   else if(!strcmp(name,"RtlExitUserProcess") && old==(void*)real_rtl_exit)replacement=observed_rtl_exit;
   else if(!strcmp(name,"exit")){if(!real_crt_exit)real_crt_exit=(void(__cdecl*)(int))old;if(old==(void*)real_crt_exit)replacement=observed_crt_exit;}
   else if(!strcmp(name,"_exit")){if(!real_crt_fast_exit)real_crt_fast_exit=(void(__cdecl*)(int))old;if(old==(void*)real_crt_fast_exit)replacement=observed_crt_fast_exit;}
   if(!replacement||!old||exit_patch_count>=512)continue;
   DWORD protect;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protect))continue;
   void *found=InterlockedCompareExchangePointer(slot,replacement,old);
   DWORD ignored;VirtualProtect(slot,sizeof(void*),protect,&ignored);
   if(found!=old)continue;
   exit_patches[exit_patch_count].slot=slot;exit_patches[exit_patch_count].old=old;exit_patches[exit_patch_count++].replacement=replacement;
   logmsg("Exit import observer: module=%s api=%s",path,name);
  }
 }
}
static void install_exit_observers(void){
 real_exit_process=(void(WINAPI*)(UINT))GetProcAddress(GetModuleHandleA("kernel32.dll"),"ExitProcess");
 real_terminate_process=(BOOL(WINAPI*)(HANDLE,UINT))GetProcAddress(GetModuleHandleA("kernel32.dll"),"TerminateProcess");
 real_rtl_exit=(void(WINAPI*)(LONG))GetProcAddress(GetModuleHandleA("ntdll.dll"),"RtlExitUserProcess");
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
 if(snap==INVALID_HANDLE_VALUE){logmsg("Exit import observer snapshot failed=%lu",GetLastError());return;}
 MODULEENTRY32 m;memset(&m,0,sizeof(m));m.dwSize=sizeof(m);
 if(Module32First(snap,&m))do{patch_exit_imports(m.hModule,m.szExePath);}while(Module32Next(snap,&m));
 CloseHandle(snap);logmsg("Exit import observers installed=%u; later modules and dynamic function lookups are not covered",exit_patch_count);
}
static void restore_exit_observers(void){
 for(unsigned i=0;i<exit_patch_count;i++){
  MEMORY_BASIC_INFORMATION m;if(!VirtualQuery(exit_patches[i].slot,&m,sizeof(m))||m.State!=MEM_COMMIT)continue;
  DWORD protect;if(!VirtualProtect(exit_patches[i].slot,sizeof(void*),PAGE_READWRITE,&protect))continue;
  InterlockedCompareExchangePointer(exit_patches[i].slot,exit_patches[i].old,exit_patches[i].replacement);
  DWORD ignored;VirtualProtect(exit_patches[i].slot,sizeof(void*),protect,&ignored);
 }
}
