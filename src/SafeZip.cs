using System;using System.IO;using System.IO.Compression;using System.Text;using System.Collections.Generic;
// Bounded ZIP32 reader, so self-update also works with .NET Framework 4.0.
public static class SafeZip {
 class Entry {public string Name;public uint Packed,Size,Crc,Offset;public ushort Flags,Method;}
 public static void Extract(string zip,string destination){
  using(var file=File.OpenRead(zip))using(var r=new BinaryReader(file)){
   long end=-1;for(long p=file.Length-22;p>=Math.Max(0,file.Length-65557);p--){file.Position=p;if(r.ReadUInt32()==0x06054b50){file.Position=p+20;if(p+22+r.ReadUInt16()==file.Length){end=p;break;}}}
   if(end<0)throw new Exception("ZIP end record missing");file.Position=end+4;
   if(r.ReadUInt16()!=0||r.ReadUInt16()!=0)throw new Exception("Multi-disk ZIP refused");int diskCount=r.ReadUInt16(),count=r.ReadUInt16();uint centralSize=r.ReadUInt32(),centralOffset=r.ReadUInt32();
   if(count!=diskCount||count==0||count>4096||centralOffset+(long)centralSize!=end)throw new Exception("Invalid ZIP directory");
   var entries=new List<Entry>();var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);long total=0;file.Position=centralOffset;
   for(int i=0;i<count;i++){
    if(r.ReadUInt32()!=0x02014b50)throw new Exception("Invalid ZIP entry");r.ReadUInt16();r.ReadUInt16();var e=new Entry();e.Flags=r.ReadUInt16();e.Method=r.ReadUInt16();r.ReadUInt32();e.Crc=r.ReadUInt32();e.Packed=r.ReadUInt32();e.Size=r.ReadUInt32();int n=r.ReadUInt16(),extra=r.ReadUInt16(),comment=r.ReadUInt16();if(r.ReadUInt16()!=0)throw new Exception("Multi-disk entry");r.ReadUInt16();uint attr=r.ReadUInt32();e.Offset=r.ReadUInt32();
    if((e.Flags&~0x808)!=0||(e.Method!=0&&e.Method!=8)||e.Size>64*1024*1024||e.Packed>64*1024*1024||(attr>>16&0xf000)==0xa000)throw new Exception("Unsupported ZIP entry");
    byte[] name=r.ReadBytes(n);if(name.Length!=n)throw new EndOfStreamException();e.Name=new UTF8Encoding(false,true).GetString(name);ValidateName(e.Name);if(!names.Add(e.Name.TrimEnd('/')))throw new Exception("Duplicate ZIP path");total+=e.Size;if(total>256*1024*1024)throw new Exception("ZIP is too large");file.Position+=extra+comment;if(file.Position>centralOffset+(long)centralSize)throw new Exception("ZIP directory overflow");entries.Add(e);
   }
   if(file.Position!=end)throw new Exception("ZIP directory length mismatch");
   Directory.CreateDirectory(destination);
   foreach(var e in entries){string output=Core.Inside(destination,e.Name.TrimEnd('/'));if(e.Name.EndsWith("/")){Directory.CreateDirectory(output);continue;}
    if(e.Offset+(long)30>centralOffset)throw new Exception("Invalid ZIP offset");file.Position=e.Offset;if(r.ReadUInt32()!=0x04034b50)throw new Exception("ZIP local header missing");r.ReadUInt16();if(r.ReadUInt16()!=e.Flags||r.ReadUInt16()!=e.Method)throw new Exception("ZIP local header mismatch");file.Position+=16;int n=r.ReadUInt16(),extra=r.ReadUInt16();byte[] name=r.ReadBytes(n);if(new UTF8Encoding(false,true).GetString(name)!=e.Name)throw new Exception("ZIP local path mismatch");file.Position+=extra;if(file.Position+e.Packed>centralOffset)throw new Exception("ZIP payload overflow");byte[] packed=r.ReadBytes((int)e.Packed);if(packed.Length!=e.Packed)throw new EndOfStreamException();
    Directory.CreateDirectory(Path.GetDirectoryName(output));using(var input=new MemoryStream(packed))using(var data=e.Method==8?(Stream)new DeflateStream(input,CompressionMode.Decompress):input)using(var target=new FileStream(output,FileMode.CreateNew)){
     byte[] buffer=new byte[65536];long received=0;uint crc=0xffffffff;int got;while((got=data.Read(buffer,0,buffer.Length))>0){received+=got;if(received>e.Size)throw new Exception("ZIP size exceeded");for(int j=0;j<got;j++){crc^=buffer[j];for(int bit=0;bit<8;bit++)crc=(crc>>1)^((crc&1)!=0?0xedb88320u:0);}target.Write(buffer,0,got);}if(received!=e.Size||~crc!=e.Crc)throw new Exception("ZIP CRC or size mismatch");
    }
   }
  }
 }
 public static void ValidateName(string name){if(string.IsNullOrEmpty(name)||name.StartsWith("/")||name.Contains("\\")||name.Contains(":"))throw new Exception("Unsafe ZIP path");foreach(string part in name.TrimEnd('/').Split('/')){if(part==""||part=="."||part==".."||part.EndsWith(".")||part.EndsWith(" "))throw new Exception("Unsafe ZIP path");foreach(char c in part)if(c<32||"<>\"|?*".IndexOf(c)>=0)throw new Exception("Unsafe ZIP filename");string device=part.Split('.')[0].ToUpperInvariant();if(device=="CON"||device=="PRN"||device=="AUX"||device=="NUL"||System.Text.RegularExpressions.Regex.IsMatch(device,@"^(COM|LPT)[0-9]$"))throw new Exception("Reserved ZIP filename");}}
}
