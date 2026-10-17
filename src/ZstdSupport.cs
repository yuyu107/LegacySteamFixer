using System;using System.IO;using System.Diagnostics;using System.Collections.Generic;
public static class ZstdSupport {
 public const string SupportedClientHash="d0e83c515f17ca57090c8c73664e5d61e37eae718dfa3a5cbb1e4b909548fc34";
 public static readonly Dictionary<string,string> Components=new Dictionary<string,string>{{"LegacySteam-ZstdModule.dll","6df6d82ce5bfea5423a372a49ae2b435a8274d1a19ae59f479c9fee9f342cf6d"},{"steam_zstd.dll","6dea9ef6f0fd1a956dc81f63551fb27ff280082b87de4790271e5f6f3247e5bb"},{"vsza_hook.bin","fcbadbf70e968b380c564258401c02d28d0b0d676d5b0b43ecd67d54880db055"}};
 public static void VerifyComponents(){foreach(var item in Components){string path=Core.Inside(Core.Base,"zstd/"+item.Key);if(!File.Exists(path)||Core.Hash(path)!=item.Value)throw new Exception("Zstd component missing or changed: "+item.Key);}}
 public static void VerifyClient(string steam){string path=Core.Inside(steam,"steamclient.dll");if(!File.Exists(path)||Core.Hash(path)!=SupportedClientHash)throw new Exception("Zstd download support requires the supported November 2024 32-bit steamclient.dll. Current file does not match; no patch was applied.");}
 public static void Enable(string steam,Action<string> log){VerifyComponents();VerifyClient(steam);string registered=Core.SteamRoot();if(string.IsNullOrEmpty(registered)||!Path.GetFullPath(registered).TrimEnd('\\','/').Equals(Path.GetFullPath(steam).TrimEnd('\\','/'),StringComparison.OrdinalIgnoreCase))throw new Exception("Selected Steam folder differs from the registered Steam installation. Set the correct Steam folder before enabling Zstd.");
 foreach(var proc in Process.GetProcessesByName("steam"))using(proc){try{string path=proc.MainModule.FileName;if(!Path.GetDirectoryName(path).TrimEnd('\\','/').Equals(Path.GetFullPath(steam).TrimEnd('\\','/'),StringComparison.OrdinalIgnoreCase))throw new InvalidOperationException("A Steam process is running from another installation. Close it or select that installation first.");}catch(System.ComponentModel.Win32Exception){if(IsAdministrator())throw;log("Steam path will be checked by elevated Fixer.");}catch(NotSupportedException){throw new Exception("Steam process path inspection unavailable.");}}
 if(IntPtr.Size!=4)throw new Exception("Zstd module requires the x86 build of LegacySteamFixer.");
 var message=new System.Text.StringBuilder(1024);
 if(IsAdministrator()) {int result=CallModule(message);log(message.ToString());if(result!=0)throw new Exception(message.ToString());}
 else {
 string resultPath=Path.Combine(Path.GetTempPath(),"LegacySteamFixer-Zstd-"+Guid.NewGuid().ToString("N")+".json");
 try {using(var process=Process.Start(new ProcessStartInfo(Core.Inside(Core.Base,"LegacySteamFixer.exe"),"--enable-zstd "+Quote(steam)+" "+Quote(resultPath)){UseShellExecute=true,Verb="runas",WorkingDirectory=Core.Base})) {
 if(process==null)throw new Exception("Could not elevate LegacySteamFixer.");process.WaitForExit();
 if(!File.Exists(resultPath))throw new Exception("Elevated Fixer did not return a Zstd result (exit "+process.ExitCode+").");
 var result=new System.Web.Script.Serialization.JavaScriptSerializer().Deserialize<ZstdResult>(File.ReadAllText(resultPath));
 if(result==null)throw new Exception("Invalid Zstd result");log(result.Message);if(process.ExitCode!=0||result.Code!=0)throw new Exception(result.Message);
 }}finally{if(File.Exists(resultPath))File.Delete(resultPath);}
 }
 log("Zstd enabled by Fixer. Reapply after Steam fully exits or restarts.");
 }
 public class ZstdResult {public int Code;public string Message;}
 public static bool IsAdministrator(){using(var id=System.Security.Principal.WindowsIdentity.GetCurrent())return new System.Security.Principal.WindowsPrincipal(id).IsInRole(System.Security.Principal.WindowsBuiltInRole.Administrator);}
 internal static string Quote(string text){return "\""+text.Replace("\"", "") .TrimEnd('\\')+"\"";}
 [System.Runtime.InteropServices.UnmanagedFunctionPointer(System.Runtime.InteropServices.CallingConvention.Cdecl,CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
 delegate int EnableModule(System.Text.StringBuilder message,uint capacity);
 [System.Runtime.InteropServices.DllImport("kernel32.dll",CharSet=System.Runtime.InteropServices.CharSet.Unicode,SetLastError=true)]static extern IntPtr LoadLibraryEx(string path,IntPtr reserved,uint flags);
 [System.Runtime.InteropServices.DllImport("kernel32.dll",CharSet=System.Runtime.InteropServices.CharSet.Ansi,ExactSpelling=true)]static extern IntPtr GetProcAddress(IntPtr module,string name);
 [System.Runtime.InteropServices.DllImport("kernel32.dll")]static extern bool FreeLibrary(IntPtr module);
 static int CallModule(System.Text.StringBuilder message){IntPtr module=LoadLibraryEx(Core.Inside(Core.Base,"zstd/LegacySteam-ZstdModule.dll"),IntPtr.Zero,8);if(module==IntPtr.Zero)throw new System.ComponentModel.Win32Exception(System.Runtime.InteropServices.Marshal.GetLastWin32Error());try{IntPtr entry=GetProcAddress(module,"EnableZstd");if(entry==IntPtr.Zero)throw new Exception("Zstd module entry missing");return ((EnableModule)System.Runtime.InteropServices.Marshal.GetDelegateForFunctionPointer(entry,typeof(EnableModule)))(message,1024);}finally{FreeLibrary(module);}}
 public static int Elevated(string steam,string resultPath){var result=new ZstdResult();try{if(!IsAdministrator())throw new Exception("Administrator privileges required");Enable(steam,s=>result.Message=s);result.Code=0;}catch(Exception e){result.Code=1;result.Message=e.Message;}try{File.WriteAllText(resultPath,new System.Web.Script.Serialization.JavaScriptSerializer().Serialize(result));}catch{return 2;}return result.Code;}
}
