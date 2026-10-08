"""Developer-only native template build; Windows users run the precompiled package."""
from pathlib import Path
import subprocess,hashlib,re
import ziglang
root=Path(__file__).resolve().parent.parent
native=root/'native';zig=Path(ziglang.__path__[0])/'zig'
subprocess.run([str(zig),'dlltool','-m','i386:x86-64','-d',str(native/'dependency64.def'),'-l',str(native/'dependency64.lib')],check=True)
out=root/'templates/bridge-x64.dll'
subprocess.run([str(zig),'cc','-target','x86_64-windows-gnu','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-nostdlib','-isystem',str(zig.parent/'lib/libc/include/any-windows-any'),'-shared','-Wl,--entry,DllMain',str(native/'bridge.c'),str(native/'dependency64.lib'),'-lkernel32','-ladvapi32','-luser32','-o',str(out)],check=True)
p=root/'src/AutoBridge.cs';text=p.read_text();text=re.sub(r'TemplateHash="[0-9a-f]{64}"','TemplateHash="'+hashlib.sha256(out.read_bytes()).hexdigest()+'"',text);p.write_text(text)
print('Native template and checksum updated. Rebuild managed program with Build.cmd.')
