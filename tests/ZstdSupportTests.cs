using System;using System.IO;using System.Collections.Generic;
class ZstdSupportTests {
 static void A(bool v,string why){if(!v)throw new Exception(why);}
 static void Main(){string shipped=Core.Base;ZstdSupport.VerifyComponents();string root=Path.Combine(Path.GetTempPath(),"zstd-integration-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);try{
 string componentDir=Path.Combine(root,"zstd");Directory.CreateDirectory(componentDir);foreach(var entry in ZstdSupport.Components)File.Copy(Path.Combine(shipped,"zstd",entry.Key),Path.Combine(componentDir,entry.Key));Core.Base=root;ZstdSupport.VerifyComponents();
 string client=Path.Combine(root,"steamclient.dll");File.WriteAllText(client,"unsupported build");string hash=Core.Hash(client);bool refused=false;try{ZstdSupport.VerifyClient(root);}catch(Exception e){refused=e.Message.Contains("does not match");}A(refused&&Core.Hash(client)==hash,"unsupported target rejected without writes");
 File.AppendAllText(Path.Combine(componentDir,"vsza_hook.bin"),"changed");refused=false;try{ZstdSupport.VerifyComponents();}catch(Exception e){refused=e.Message.Contains("vsza_hook.bin");}A(refused&&Core.Hash(client)==hash,"tampered hook blocked before injection");File.Delete(Path.Combine(componentDir,"steam_zstd.dll"));refused=false;try{ZstdSupport.VerifyComponents();}catch(Exception e){refused=e.Message.Contains("steam_zstd.dll");}A(refused,"missing decoder DLL blocked");
 Console.WriteLine("PASS: packaged Zstd component hashes, unsupported client refusal without disk writes, tampered hook refusal, missing helper refusal. No Steam process or live download was used.");
 }finally{Core.Base=shipped;Directory.Delete(root,true);}}
}
