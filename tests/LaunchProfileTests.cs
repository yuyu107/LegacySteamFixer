using System;using System.IO;using System.Collections.Generic;using System.Web.Script.Serialization;
class LaunchProfileTests {
 static void A(bool value,string why){if(!value)throw new Exception(why);}
 static void Refused(Action a,string message){bool blocked=false;try{a();}catch(Exception e){blocked=e.Message.IndexOf(message,StringComparison.OrdinalIgnoreCase)>=0;}A(blocked,"Expected refusal: "+message);}
 static void Main(string[] args){string original=Path.GetFullPath(args[0]),nativeExe=Path.GetFullPath(args[1]);string tmp=Path.Combine(Path.GetTempPath(),"launch-profile-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(tmp);try{
 string steam=Path.Combine(tmp,"steam"),root=Path.Combine(steam,"steamapps/common/Test Game 中文");Directory.CreateDirectory(root);File.WriteAllText(Path.Combine(steam,"steam.exe"),"fixture");File.WriteAllText(Path.Combine(steam,"steamclient64.dll"),"fixture client");File.WriteAllText(Path.Combine(steam,"steamapps/appmanifest_999.acf"),"\"AppState\" { \"appid\" \"999\" \"name\" \"Fixture\" \"installdir\" \"Test Game 中文\" }");
 File.Copy(original,Path.Combine(root,"steam_api64.dll"));File.Copy(nativeExe,Path.Combine(root,"Game.exe"));Core.Base=tmp;Core.Modules=new Module[0];
 var g=new Game{Root=root,AppId="999",Name="Fixture"};var plan=new AutoPlan{Root=root,Steam=steam,ClientHash=Core.Hash(Path.Combine(steam,"steamclient64.dll"))};var f=new AutoFile{Path="steam_api64.dll",Backup="steam_api64_original.dll",OriginalHash=Core.Hash(Path.Combine(root,"steam_api64.dll"))};plan.Files.Add(f);
 A(LaunchProfiles.ValidateInjectionPlan(g,"Game.exe",plan)==f.Path,"arbitrary game/API paths accepted");string hash=f.OriginalHash;
 var p=new LaunchProfile{Name=g.Name,Root=root,Exe="Game.exe",Steam=steam,AppId=g.AppId,Mode="inject",Api=f.Path,ApiHash=hash,ClientHash=plan.ClientHash};LaunchProfiles.Save(p);A(LaunchProfiles.Last().Api==f.Path&&LaunchProfiles.Load(p.Id).Root==root,"persist reload Unicode paths");A(Core.Hash(Path.Combine(root,f.Path))==hash&&!File.Exists(Path.Combine(root,f.Backup)),"saving injection does not replace game DLL/create proxy backup");
 LaunchProfiles.Save(p);A(LaunchProfiles.Last().Id==p.Id,"atomic overwrite");Refused(()=>LaunchProfiles.Load("../escape"),"profile ID");
 var bad=new LaunchProfile{Root=root,Exe="../Game.exe",AppId="999",Mode="inject",Api=f.Path};Refused(()=>LaunchProfiles.Save(bad),"outside root");
 File.WriteAllText(Path.Combine(root,f.Path),"updated");Refused(()=>LaunchProfiles.ValidateForLaunch(p),"SDK changed");File.Copy(original,Path.Combine(root,f.Path),true);
 File.WriteAllText(Path.Combine(steam,"steamclient64.dll"),"updated");Refused(()=>LaunchProfiles.ValidateForLaunch(p),"Steam client changed");File.WriteAllText(Path.Combine(steam,"steamclient64.dll"),"fixture client");
 File.WriteAllText(Path.Combine(steam,"steamapps/appmanifest_999.acf"),"\"AppState\" { \"appid\" \"998\" \"installdir\" \"Test Game 中文\" }");Refused(()=>LaunchProfiles.ValidateForLaunch(p),"manifest");
 LaunchProfiles.Remove(p);A(!File.Exists(LaunchProfiles.JsonPath(p.Id))&&LaunchProfiles.Last()==null&&Core.Hash(Path.Combine(root,f.Path))==hash,"profile removal leaves original DLL intact");
 plan.Files.Add(f);Refused(()=>LaunchProfiles.ValidateInjectionPlan(g,"Game.exe",plan),"uniquely match");
 var unity=new AutoPlan();var main=new AutoFile{Path="Rizline_Data\\Plugins\\x86_64\\steam_api64.dll",OriginalHash="main"};var editor=new AutoFile{Path="Editor/Rizline Editor_Data/Plugins/x86_64/steam_api64.dll",OriginalHash="editor"};unity.Files.Add(editor);unity.Files.Add(main);A(LaunchProfiles.SelectInjectionFile("Rizline.exe",unity)==main,"main SDK selected regardless of scan order");A(LaunchProfiles.SelectInjectionFile("Editor\\Rizline Editor.exe",unity)==editor,"editor SDK selected");Refused(()=>LaunchProfiles.SelectInjectionFile("Other.exe",unity),"uniquely match");
 Console.WriteLine("PASS: configurable native x64 injection plan, Unicode paths, profile save/reload/overwrite, no game DLL writes during save/remove, path confinement, changed SDK/client/manifest refusals, ambiguous SDK refusal. No live Windows injection test.");
 }finally{Directory.Delete(tmp,true);}}
}
