using System;using System.IO;using System.Web.Script.Serialization;
public class UpdateSettings {
 public bool Automatic=true,Preview=AppInfo.Version.Contains("-test");
 public DateTime LastAttemptUtc=DateTime.MinValue;
 public static string FilePath {get{return Path.Combine(Core.Base,"settings/update.json");}}
 public bool Due(DateTime now){return Automatic&&(LastAttemptUtc>now||now-LastAttemptUtc>=TimeSpan.FromHours(24));}
 public static UpdateSettings Load(){try{if(File.Exists(FilePath)){var settings=new JavaScriptSerializer().Deserialize<UpdateSettings>(File.ReadAllText(FilePath))??new UpdateSettings();settings.LastAttemptUtc=settings.LastAttemptUtc.ToUniversalTime();return settings;}}catch{}return new UpdateSettings();}
 public void Save(){Directory.CreateDirectory(Path.GetDirectoryName(FilePath));string temp=FilePath+".tmp-"+Guid.NewGuid().ToString("N");try{File.WriteAllText(temp,new JavaScriptSerializer().Serialize(this));if(File.Exists(FilePath))File.Replace(temp,FilePath,null);else File.Move(temp,FilePath);}finally{if(File.Exists(temp))File.Delete(temp);}}
}
