/* One-shot x64 diagnostic breakpoint at the locally resolved Unity icall.
   Restore its original byte before resuming; no quit suppression or trampoline. */
typedef struct {BYTE *p;BYTE original;int armed;int zero;} QuitPoint;
static QuitPoint unity_points[8];
#define unity_quit_target unity_points[0].p
#define unity_quit_byte unity_points[0].original
#define unity_quit_armed unity_points[0].armed
static int readable_protection(DWORD p){p&=0xff;return p==PAGE_READONLY||p==PAGE_READWRITE||p==PAGE_WRITECOPY||p==PAGE_EXECUTE_READ||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY;}
static int executable_protection(DWORD p){p&=0xff;return p==PAGE_EXECUTE||p==PAGE_EXECUTE_READ||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY;}
static int restore_quit_point(QuitPoint *q){
 if(!q->p||!q->armed)return 1;
 DWORD protect;if(!VirtualProtect(q->p,1,PAGE_EXECUTE_READWRITE,&protect))return 0;
 *q->p=q->original;FlushInstructionCache(GetCurrentProcess(),q->p,1);
 DWORD ignored;VirtualProtect(q->p,1,protect,&ignored);q->armed=0;return 1;
}
static int restore_unity_quit(void){int ok=1;for(int k=0;k<8;k++)if(!restore_quit_point(&unity_points[k]))ok=0;return ok;}
static int is_unity_quit_break(void *p){if(!p)return 0;for(int k=0;k<8;k++)if(p==unity_points[k].p)return 1;return 0;}
static const char *check_labels[]={"unity_genuine_false","required_file_check_false","content_table_check_false","content_compare_false","file_digest_missing"};
static int check_readable(uintptr_t a,size_t n){MEMORY_BASIC_INFORMATION m;return a&&VirtualQuery((void*)a,&m,sizeof(m))&&m.State==MEM_COMMIT&&!(m.Protect&PAGE_GUARD)&&readable_protection(m.Protect)&&a>=(uintptr_t)m.BaseAddress&&n<=m.RegionSize&&a-(uintptr_t)m.BaseAddress<=m.RegionSize-n;}
static void log_check_string(const char *label,uintptr_t object){
 if(!check_readable(object,24)){logmsg("CHECK_PATH %s unavailable object=%p",label,(void*)object);return;}
 int n=*(int*)(object+16);if(n<0||n>512||!check_readable(object+20,(size_t)n*2)){logmsg("CHECK_PATH %s unavailable length=%d",label,n);return;}
 char out[1537];int bytes=WideCharToMultiByte(CP_UTF8,0,(WCHAR*)(object+20),n,out,1536,NULL,NULL);
 if(bytes<0||bytes>1536)bytes=0;out[bytes]=0;logmsg("CHECK_PATH %s utf8=%s",label,out);
}
static void log_quit_address(const char *label,unsigned i,void *pc){
 MEMORY_BASIC_INFORMATION m;char name[MAX_PATH];name[0]=0;void *base=NULL;
 if(VirtualQuery(pc,&m,sizeof(m))){base=m.AllocationBase;GetModuleFileNameA((HMODULE)base,name,MAX_PATH);}
 logmsg("%s %u pc=%p module=%s base=%p offset=%p",label,i,pc,name,base,(void*)((uintptr_t)pc-(uintptr_t)base));
}
static LONG observe_unity_quit(EXCEPTION_POINTERS *e){
 QuitPoint *q=&unity_points[0];for(int k=1;k<8;k++)if(e->ExceptionRecord->ExceptionAddress==unity_points[k].p)q=&unity_points[k];
 if(!restore_quit_point(q)){logmsg("QUIT breakpoint restore failed=%lu; normal exception dispatch continues",GetLastError());return EXCEPTION_CONTINUE_SEARCH;}
 if(q>=&unity_points[3]){
  int k=(int)(q-unity_points)-3;int conditional=k<3;
  logmsg("GAME_CHECK stage=%s failure_branch=%d flags=%08x AL=%02x thread=%lu (original decision unchanged; one-shot)",check_labels[k],conditional?!!(e->ContextRecord->EFlags&0x40):1,(unsigned)e->ContextRecord->EFlags,(unsigned)e->ContextRecord->Rax&255,GetCurrentThreadId());
  if(k==1)log_check_string("required_file",(uintptr_t)e->ContextRecord->Rbx);
  if(k>=3)log_check_string("compared_file",(uintptr_t)e->ContextRecord->R12);
  e->ContextRecord->Rip=(DWORD64)(uintptr_t)q->p;return EXCEPTION_CONTINUE_EXECUTION;
 }
 if(q==&unity_points[2]){
  uintptr_t obj=(uintptr_t)e->ContextRecord->Rcx;MEMORY_BASIC_INFORMATION om;
  if(VirtualQuery((void*)obj,&om,sizeof(om))&&om.State==MEM_COMMIT&&!(om.Protect&PAGE_GUARD)&&readable_protection(om.Protect)&&obj<=(uintptr_t)om.BaseAddress+om.RegionSize-32)
   logmsg("QUIT_TASK_FIRST_ENTRY state=%d delay_input=%d object=%p thread=%lu (field interpretation inferred from validated code)",*(int*)obj,*(int*)(obj+16),(void*)obj,GetCurrentThreadId());
  else logmsg("QUIT_TASK_FIRST_ENTRY object unreadable=%p",(void*)obj);
 }
 logmsg(q==&unity_points[2]?"QUIT_TASK_OBSERVED code=%08x thread=%lu target=%p RSP=%p (original code resumes)":"UNITY_QUIT_REQUEST code=%08x thread=%lu target=%p RSP=%p (original code resumes)",q->zero?0:(unsigned)e->ContextRecord->Rcx,GetCurrentThreadId(),q->p,(void*)e->ContextRecord->Rsp);
 uintptr_t sp=(uintptr_t)e->ContextRecord->Rsp;
 MEMORY_BASIC_INFORMATION stack;
 if(VirtualQuery((void*)sp,&stack,sizeof(stack))&&stack.State==MEM_COMMIT&&!(stack.Protect&PAGE_GUARD)&&readable_protection(stack.Protect)){
  uintptr_t end=(uintptr_t)stack.BaseAddress+stack.RegionSize;
  size_t words=(end>sp?(end-sp)/sizeof(void*):0);if(words>128)words=128;
  for(size_t i=0;i<words;i++){
   void *pc=((void**)sp)[i];MEMORY_BASIC_INFORMATION m;
   if(!i)log_quit_address(q==&unity_points[2]?"TASK_DIRECT_RETURN":"QUIT_DIRECT_RETURN",0,pc);
   else if(pc&&VirtualQuery(pc,&m,sizeof(m))&&m.State==MEM_COMMIT&&!(m.Protect&PAGE_GUARD)&&executable_protection(m.Protect))log_quit_address(q==&unity_points[2]?"TASK_STACK_CANDIDATE":"QUIT_STACK_CANDIDATE",(unsigned)i,pc);
  }
 }
 /* Stack candidates can be stale addresses/data, not verified unwind frames. */
 e->ContextRecord->Rip=(DWORD64)(uintptr_t)q->p;
 return EXCEPTION_CONTINUE_EXECUTION;
}
static const BYTE quit_arg_code[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0x5,0x13,0xa1,0xa0,0x0,0x8b,0xd9,0x48,0x85,0xc0,0x75,0x13,0x48,0x8d,0xd,0xb5,0x5a,0x32,0x0,0xe8,0xe0,0xb2,0xc7,0xfe,0x48,0x89,0x5,0xf9,0xa0,0xa0,0x0,0x8b,0xcb,0x48,0x83,0xc4,0x20,0x5b,0x48,0xff,0xe0};
static const BYTE quit_arg_mask[]={1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1};
static const BYTE quit_zero_code[]={0x48,0x83,0xec,0x28,0x48,0x8b,0x5,0xd5,0xa0,0xa0,0x0,0x48,0x85,0xc0,0x75,0x13,0x48,0x8d,0xd,0x79,0x5a,0x32,0x0,0xe8,0xa4,0xb2,0xc7,0xfe,0x48,0x89,0x5,0xbd,0xa0,0xa0,0x0,0x33,0xc9,0x48,0x83,0xc4,0x28,0x48,0xff,0xe0};
static const BYTE quit_zero_mask[]={1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1};
static int matches_quit_code(BYTE *p,const BYTE *code,const BYTE *mask,size_t n){for(size_t i=0;i<n;i++)if(mask[i]&&p[i]!=code[i])return 0;return 1;}
static int local_exec(BYTE *p,HMODULE module){MEMORY_BASIC_INFORMATION m;return VirtualQuery(p,&m,sizeof(m))&&m.AllocationBase==(void*)module&&m.State==MEM_COMMIT&&!(m.Protect&PAGE_GUARD)&&readable_protection(m.Protect)&&executable_protection(m.Protect);}

/* Recognize only the observed state-machine structure, then require a unique
   direct call to a validated zero-argument Quit wrapper. Unknown shapes skip. */
static void scan_quit_task(HMODULE assembly,BYTE *base,DWORD size,IMAGE_DATA_DIRECTORY dir){
 const BYTE prefix[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x20,0x48,0x89,0x4c,0x24,0x08,0x57,0x48,0x83,0xec,0x60,0x48,0x8b,0xf9};
 BYTE *found=NULL;unsigned matches=0;BYTE *quit=unity_points[1].p;
 if(!quit)return;
 for(DWORD i=0;i<dir.Size;i+=12){DWORD *f=(DWORD*)(base+dir.VirtualAddress+i);DWORD b=f[0],end=f[1];
  if(b>=size||end>size||end<=b||end-b<220||end-b>4096)continue;
  BYTE *p=base+b;if(!local_exec(p,assembly)||memcmp(p,prefix,sizeof(prefix)))continue;
  int delay=0,state=0,calls=0;
  for(DWORD j=23;j+5<end-b;j++){
   if(p[j]==0x8b&&p[j+1]==0x77&&p[j+2]==0x10)delay++;
   if(p[j]==0x8b&&p[j+1]==0x07&&p[j+2]==0x33&&p[j+3]==0xdb)state++;
   if(p[j]==0xe8&&p+j+5+*(int32_t*)(p+j+1)==quit)calls++;
  }
  if(delay==1&&state==1&&calls==1){found=p;matches++;}
 }
 if(matches!=1){logmsg("QUIT_TASK scan skipped: validated structure matches=%u",matches);return;}
 DWORD protect;if(*found==0xcc||!VirtualProtect(found,1,PAGE_EXECUTE_READWRITE,&protect))return;
 QuitPoint *q=&unity_points[2];q->p=found;q->original=*found;q->armed=1;q->zero=1;
 *found=0xcc;FlushInstructionCache(GetCurrentProcess(),found,1);DWORD ignored;VirtualProtect(found,1,protect,&ignored);
 log_quit_address("QUIT_TASK_OBSERVER_READY",0,found);
}


/* Specific diagnostic profile derived from supplied build; not a general repair rule. */
static void install_check_profile(HMODULE module){
 BYTE *base=(BYTE*)module;
 static const BYTE guard0[]={0xc9,0xe8,0xa,0xb9,0x20,0x1,0x84,0xc0,0xf,0x84,0xba,0x4,0x0,0x0,0x48,0x8b,0xd,0x6b,0xb5,0xb2,0x1,0x83,0xb9,0xe0,0x0,0x0,0x0,0x0,0x75,0xc,0xe8,0xcd,0x66,0xe8,0xff,0x48,0x8b,0xd,0x56,0xb5};
 static const BYTE guard1[]={0xc8,0xe8,0x46,0xa1,0xd5,0x0,0x84,0xc0,0xf,0x84,0xd6,0x3,0x0,0x0,0x33,0xd2,0x48,0x8b,0xcb,0xe8,0xf4,0xb1,0xd5,0x0,0x48,0x8b,0xf8,0x48,0x8b,0xd,0x6a,0x7a,0xb0,0x1,0x83,0xb9,0xe0,0x0,0x0,0x0};
 static const BYTE guard2[]={0x81,0x1,0x0,0x41,0x80,0x7e,0x20,0x0,0xf,0x84,0xf6,0x1,0x0,0x0,0x33,0xff,0xf,0x1f,0x40,0x0,0x66,0x66,0xf,0x1f,0x84,0x0,0x0,0x0,0x0,0x0,0x48,0x8b,0xd,0x89,0x78,0xb0,0x1,0x83,0xb9,0xe0};
 static const BYTE guard3[]={0xd3,0x87,0xc4,0x0,0x84,0xc0,0x75,0x4,0xeb,0x25,0xeb,0x23,0xff,0xc7,0xe9,0x34,0xfe,0xff,0xff,0xb0,0x1,0x48,0x8b,0x5c,0x24,0x68,0x48,0x8b,0x74,0x24,0x78,0x48,0x83,0xc4,0x30,0x41,0x5f,0x41,0x5e,0x41};
 static const BYTE guard4[]={0xc4,0x0,0x84,0xc0,0x75,0x4,0xeb,0x25,0xeb,0x23,0xff,0xc7,0xe9,0x34,0xfe,0xff,0xff,0xb0,0x1,0x48,0x8b,0x5c,0x24,0x68,0x48,0x8b,0x74,0x24,0x78,0x48,0x83,0xc4,0x30,0x41,0x5f,0x41,0x5e,0x41,0x5d,0x41};
 DWORD offsets[]={0x3c1218,0x3c12fc,0x3c14da,0x3c16b1,0x3c16b3};
 const BYTE *guards[]={guard0,guard1,guard2,guard3,guard4};
 IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64*)(base+dos->e_lfanew);
 if(nt->OptionalHeader.SizeOfImage<0x3c1800){logmsg("GAME_CHECK_PROFILE skipped: unsupported image size");return;}
 for(int k=0;k<5;k++){BYTE *p=base+offsets[k]-8;if(!local_exec(p,module)||!check_readable((uintptr_t)p,40)||memcmp(p,guards[k],40)){logmsg("GAME_CHECK_PROFILE skipped: supplied-build signature differs at %d",k);return;}}
 for(int k=0;k<5;k++){BYTE *p=base+offsets[k];DWORD protect;if(!VirtualProtect(p,1,PAGE_EXECUTE_READWRITE,&protect))continue;
  QuitPoint *q=&unity_points[k+3];q->p=p;q->original=*p;q->armed=1;*p=0xcc;FlushInstructionCache(GetCurrentProcess(),p,1);DWORD ignored;VirtualProtect(p,1,protect,&ignored);
  logmsg("GAME_CHECK_OBSERVER_READY stage=%s offset=%08x",check_labels[k],offsets[k]);
 }
}
static void scan_quit_wrappers(HMODULE assembly){
 BYTE *base=(BYTE*)assembly;IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x100000)return;
 IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64*)(base+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return;
 DWORD size=nt->OptionalHeader.SizeOfImage;IMAGE_DATA_DIRECTORY dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
 if(!dir.VirtualAddress||dir.VirtualAddress>=size||dir.Size>size-dir.VirtualAddress||dir.Size%12||dir.Size>32000000)return;
 BYTE *found[2]={NULL,NULL};unsigned count[2]={0,0};
 for(DWORD i=0;i<dir.Size;i+=12){DWORD *f=(DWORD*)(base+dir.VirtualAddress+i);DWORD begin=f[0],end=f[1];
  if(begin>=size||end>size||end<=begin)continue;BYTE *p=base+begin;int kind=-1;
  if(end-begin==49&&local_exec(p,assembly)&&matches_quit_code(p,quit_arg_code,quit_arg_mask,49))kind=0;
  else if(end-begin==44&&local_exec(p,assembly)&&matches_quit_code(p,quit_zero_code,quit_zero_mask,44))kind=1;
  if(kind<0)continue;
  int lea=kind?16:20,call=kind?23:27,load=kind?4:6,store=kind?28:32;
  intptr_t str=(intptr_t)begin+lea+7+*(int32_t*)(p+lea+3),resolver=(intptr_t)begin+call+5+*(int32_t*)(p+call+1);
  intptr_t cache1=(intptr_t)begin+load+7+*(int32_t*)(p+load+3),cache2=(intptr_t)begin+store+7+*(int32_t*)(p+store+3);
  const char wanted[]="UnityEngine.Application::Quit(System.Int32)";
  if(str<0||str>(intptr_t)size-(intptr_t)sizeof(wanted)||memcmp(base+str,wanted,sizeof(wanted))||resolver<0||resolver>=size||!local_exec(base+resolver,assembly)||cache1!=cache2||cache1<0||cache1>(intptr_t)size-8)continue;
  found[kind]=p;count[kind]++;
 }
 for(int kind=0;kind<2;kind++){
  if(count[kind]!=1){logmsg("Local Quit wrapper kind=%d skipped: matches=%u",kind,count[kind]);continue;}
  QuitPoint *q=&unity_points[kind];BYTE *p=found[kind];DWORD protect;
  if(*p==0xcc||!VirtualProtect(p,1,PAGE_EXECUTE_READWRITE,&protect))continue;
  q->p=p;q->original=*p;q->zero=kind;q->armed=1;*p=0xcc;FlushInstructionCache(GetCurrentProcess(),p,1);
  DWORD ignored;VirtualProtect(p,1,protect,&ignored);log_quit_address("LOCAL_QUIT_WRAPPER_READY",kind,p);
 }
 scan_quit_task(assembly,base,size,dir);install_check_profile(assembly);
}
static void install_unity_quit_observer(void){
 if(!exception_observer){logmsg("Unity quit observer skipped: exception observer unavailable");return;}
 HMODULE assembly=GetModuleHandleA("GameAssembly.dll"),player=GetModuleHandleA("UnityPlayer.dll");
 if(!assembly||!player){logmsg("Unity quit observer unavailable: not a loaded IL2CPP Unity game");return;}
 typedef void *(__cdecl *ResolveIcall)(const char*);
 ResolveIcall resolve=(ResolveIcall)GetProcAddress(assembly,"il2cpp_resolve_icall");
 if(!resolve){logmsg("Unity quit export absent; scanning validated local wrapper structures");scan_quit_wrappers(assembly);return;}
 BYTE *p=(BYTE*)resolve("UnityEngine.Application::Quit(System.Int32)");
 if(!p)p=(BYTE*)resolve("UnityEngine.Application::Quit");
 MEMORY_BASIC_INFORMATION m;
 if(!p||!VirtualQuery(p,&m,sizeof(m))||m.State!=MEM_COMMIT||m.AllocationBase!=(void*)player||!executable_protection(m.Protect)||!readable_protection(m.Protect)||(m.Protect&PAGE_GUARD)){
  logmsg("Unity quit observer refused: unresolved or unexpected target=%p",p);return;
 }
 if(*p==0xcc){logmsg("Unity quit observer refused: existing breakpoint");return;}
 unity_quit_byte=*p;unity_quit_target=p;
 DWORD protect;if(!VirtualProtect(p,1,PAGE_EXECUTE_READWRITE,&protect)){unity_quit_target=NULL;logmsg("Unity quit observer protect failed=%lu",GetLastError());return;}
 unity_quit_armed=1;*p=0xcc;FlushInstructionCache(GetCurrentProcess(),p,1);
 DWORD ignored;VirtualProtect(p,1,protect,&ignored);
 log_quit_address("UNITY_QUIT_OBSERVER_READY",0,p);
}
