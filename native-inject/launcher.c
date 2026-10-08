#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <wincrypt.h>
#include <stdint.h>
static WCHAR logpath[MAX_PATH];
void *memset(void *d,int c,size_t n){BYTE *p=d;while(n--)*p++=(BYTE)c;return d;}
static void logline(const WCHAR *s){HANDLE h=CreateFileW(logpath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(h!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(h,s,lstrlenW(s)*2,&n,NULL);WriteFile(h,L"\r\n",4,&n,NULL);CloseHandle(h);}}
static void lognum(const WCHAR *s,DWORD n){WCHAR b[512];wsprintfW(b,L"%s %lu (0x%08lx)",s,n,n);logline(b);}
static int filehash(const WCHAR *path,WCHAR *out){
 HCRYPTPROV provider=0;HCRYPTHASH hash=0;HANDLE f=INVALID_HANDLE_VALUE;int ok=0;static BYTE buffer[65536];BYTE digest[32];DWORD n=0,len=32;
 f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(f==INVALID_HANDLE_VALUE)goto end;
 if(!CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)||!CryptCreateHash(provider,CALG_SHA_256,0,0,&hash))goto end;
 for(;;){if(!ReadFile(f,buffer,sizeof(buffer),&n,NULL))goto end;if(!n)break;if(!CryptHashData(hash,buffer,n,0))goto end;}
 if(!CryptGetHashParam(hash,HP_HASHVAL,digest,&len,0)||len!=32)goto end;
 for(int i=0;i<32;i++){out[i*2]=L"0123456789abcdef"[digest[i]>>4];out[i*2+1]=L"0123456789abcdef"[digest[i]&15];}out[64]=0;ok=1;
 end:if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);return ok;
}
static uintptr_t modulebase(DWORD pid,const WCHAR *name){
 HANDLE h=INVALID_HANDLE_VALUE;for(int i=0;i<10;i++){h=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);if(h!=INVALID_HANDLE_VALUE||GetLastError()!=ERROR_BAD_LENGTH)break;}
 if(h==INVALID_HANDLE_VALUE)return 0;MODULEENTRY32W m;m.dwSize=sizeof(m);uintptr_t result=0;
 if(Module32FirstW(h,&m))do{if(!lstrcmpiW(m.szModule,name)){result=(uintptr_t)m.modBaseAddr;break;}}while(Module32NextW(h,&m));CloseHandle(h);return result;
}
static int runthread(HANDLE process,void *fn,void *param,DWORD *exitcode){
 HANDLE t=CreateRemoteThread(process,NULL,0,(LPTHREAD_START_ROUTINE)fn,param,0,NULL);if(!t){lognum(L"CreateRemoteThread error",GetLastError());return 0;}
 DWORD wait=WaitForSingleObject(t,20000);if(wait!=WAIT_OBJECT_0){lognum(L"Remote thread wait",wait);CloseHandle(t);return 0;}
 int ok=GetExitCodeThread(t,exitcode);CloseHandle(t);return ok;
}
void WINAPI mainCRTStartup(void){
 static WCHAR package[MAX_PATH],dll[MAX_PATH],game[MAX_PATH],root[MAX_PATH],sdk[MAX_PATH],command[2048];
 GetModuleFileNameW(NULL,package,MAX_PATH);WCHAR *last=NULL;for(WCHAR *p=package;*p;p++)if(*p==L'\\')last=p;if(!last)ExitProcess(1);*last=0;
 wsprintfW(dll,L"%s\\LegacySteam-InjectBridge.dll",package);
 int argc=0;WCHAR **argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv||argc!=2)ExitProcess(10);
 static WCHAR cfg[2048];HANDLE cf=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(cf==INVALID_HANDLE_VALUE)ExitProcess(11);DWORD cn=0;DWORD sz=GetFileSize(cf,NULL);
 if(sz<4||sz>sizeof(cfg)-2||!ReadFile(cf,cfg,sz,&cn,NULL)||cn!=sz||sz%2){CloseHandle(cf);ExitProcess(12);}CloseHandle(cf);cfg[sz/2]=0;
 if(cfg[0]!=0xfeff)ExitProcess(13);WCHAR *fields[4];WCHAR *cursor=cfg+1;
 for(int i=0;i<4;i++){fields[i]=cursor;while(*cursor&&*cursor!=L'\n'&&*cursor!=L'\r')cursor++;if(!*cursor)ExitProcess(14);*cursor++=0;if(*cursor==L'\n')cursor++;if(!*fields[i]||lstrlenW(fields[i])>=MAX_PATH)ExitProcess(15);}
 lstrcpyW(game,fields[0]);lstrcpyW(root,fields[1]);lstrcpyW(sdk,fields[2]);WCHAR appid[64];if(lstrlenW(fields[3])>=64)ExitProcess(16);lstrcpyW(appid,fields[3]);
 for(WCHAR *p=appid;*p;p++)if(*p<L'0'||*p>L'9')ExitProcess(17);
 lstrcpyW(logpath,argv[1]);last=NULL;for(WCHAR *p=logpath;*p;p++)if(*p==L'\\')last=p;if(!last)ExitProcess(18);lstrcpyW(last+1,L"LegacySteam-Injector.log");LocalFree(argv);
 HANDLE logfile=CreateFileW(logpath,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(logfile!=INVALID_HANDLE_VALUE){WORD bom=0xfeff;DWORD n;WriteFile(logfile,&bom,2,&n,NULL);CloseHandle(logfile);}
 logline(L"=== LegacySteamFixer 0.4.0-test1 external injection ===");logline(game);logline(sdk);
 if(GetFileAttributesW(dll)==INVALID_FILE_ATTRIBUTES||GetFileAttributesW(game)==INVALID_FILE_ATTRIBUTES||GetFileAttributesW(sdk)==INVALID_FILE_ATTRIBUTES)ExitProcess(19);
 WCHAR before[65],after[65];if(!filehash(sdk,before)){lognum(L"SDK SHA256 read failure",GetLastError());ExitProcess(20);}logline(L"SDK SHA256 before bootstrap:");logline(before);
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W pe;pe.dwSize=sizeof(pe);int steam=0,active=0;
 if(snap!=INVALID_HANDLE_VALUE){if(Process32FirstW(snap,&pe))do{if(!lstrcmpiW(pe.szExeFile,L"steam.exe"))steam=1;WCHAR *gamename=game;for(WCHAR *p=game;*p;p++)if(*p==L'\\')gamename=p+1;if(!lstrcmpiW(pe.szExeFile,gamename))active=1;}while(Process32NextW(snap,&pe));CloseHandle(snap);}
 if(!steam||active){MessageBoxW(NULL,L"Run Steam and sign in first. Close existing game processes, then retry.",L"LegacySteam",MB_ICONERROR);ExitProcess(1);}
 if(!SetEnvironmentVariableW(L"SteamAppId",appid)||!SetEnvironmentVariableW(L"SteamGameId",appid))ExitProcess(21);logline(L"Installed manifest AppID:");logline(appid);
 wsprintfW(command,L"\"%s\" -logFile \"%s\\LegacySteam-Unity.log\"",game,root);
 STARTUPINFOW si={0};si.cb=sizeof(si);PROCESS_INFORMATION pi={0};
 if(!CreateProcessW(game,command,NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,root,&si,&pi)){lognum(L"CreateProcess error",GetLastError());MessageBoxW(NULL,L"Could not create game. See LegacySteam-Injector.log.",L"LegacySteam",MB_ICONERROR);ExitProcess(1);}
 lognum(L"Created suspended game PID",pi.dwProcessId);int ok=0;LPVOID remote=NULL;DWORD code=0;
 /* A bootstrap thread lets Windows complete DLL initialization while the primary
    thread remains suspended. Validate the shared ntdll entry before invoking it. */
 BYTE check[16];SIZE_T got=0;
 FARPROC end=GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlExitUserThread");
 if(!end||!ReadProcessMemory(pi.hProcess,(void*)end,check,sizeof(check),&got)||got!=sizeof(check))goto done;
 for(int i=0;i<16;i++)if(check[i]!=((BYTE*)end)[i])goto done;
 if(!runthread(pi.hProcess,(void*)end,NULL,&code))goto done;
 FARPROC load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
 HMODULE owner=NULL;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)load,&owner))goto done;
 WCHAR ownerpath[MAX_PATH];GetModuleFileNameW(owner,ownerpath,MAX_PATH);WCHAR *ownername=ownerpath;for(WCHAR *p=ownerpath;*p;p++)if(*p==L'\\')ownername=p+1;
 uintptr_t rb=modulebase(pi.dwProcessId,ownername);if(!rb){logline(L"Loader owner module missing after initialization");goto done;}void *remoteLoad=(void*)(rb+(uintptr_t)load-(uintptr_t)owner);
 if(!ReadProcessMemory(pi.hProcess,remoteLoad,check,16,&got)||got!=16)goto done;
 for(int i=0;i<16;i++)if(check[i]!=((BYTE*)load)[i])goto done;
 remote=VirtualAllocEx(pi.hProcess,NULL,MAX_PATH*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!remote)goto done;
 if(!WriteProcessMemory(pi.hProcess,remote,dll,(lstrlenW(dll)+1)*2,&got))goto done;
 if(!runthread(pi.hProcess,remoteLoad,remote,&code))goto done;
 uintptr_t bridgebase=modulebase(pi.dwProcessId,L"LegacySteam-InjectBridge.dll");if(!bridgebase){logline(L"Injected module absent");goto done;}
 HMODULE local=LoadLibraryExW(dll,NULL,DONT_RESOLVE_DLL_REFERENCES);if(!local)goto done;FARPROC start=GetProcAddress(local,"BridgeStart");uintptr_t off=(uintptr_t)start-(uintptr_t)local;FreeLibrary(local);if(!start)goto done;
 if(!WriteProcessMemory(pi.hProcess,remote,sdk,(lstrlenW(sdk)+1)*2,&got))goto done;
 if(!runthread(pi.hProcess,(void*)(bridgebase+off),remote,&code))goto done;
 lognum(L"BridgeStart result",code);if(code!=0)goto done;
 if(!filehash(sdk,after)||lstrcmpW(before,after)){logline(L"SDK changed during bootstrap; refusing to resume");goto done;}logline(L"SDK SHA256 after bootstrap (unchanged):");logline(after);
 if(ResumeThread(pi.hThread)==(DWORD)-1)goto done;ok=1;logline(L"Game primary thread resumed. Injection ready; actual gameplay remains unverified.");
 done:
 if(!ok){lognum(L"Bootstrap failure / last Win32 error",GetLastError());TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,3000);MessageBoxW(NULL,L"Bootstrap failed; only the game process created by this launcher was ended. Send LegacySteam-Injector.log and LegacySteam-InjectBridge.log.",L"LegacySteam",MB_ICONERROR);}
 if(remote&&ok)VirtualFreeEx(pi.hProcess,remote,0,MEM_RELEASE);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);ExitProcess(ok?0:1);
}
