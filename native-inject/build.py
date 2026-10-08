from pathlib import Path
import subprocess
import ziglang
root=Path(__file__).resolve().parent.parent
zig=Path(ziglang.__path__[0])/'zig'
common=[str(zig),'cc','-target','x86_64-windows-gnu','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-nostdlib','-isystem',str(zig.parent/'lib/libc/include/any-windows-any')]
subprocess.run(common+['-shared','-Wl,--entry,DllMain',str(root/'native-inject/bridge.c'),'-lkernel32','-ladvapi32','-luser32','-o',str(root/'inject/LegacySteam-InjectBridge.dll')],check=True)

subprocess.run(common+['-Wl,--entry,mainCRTStartup','-Wl,--subsystem,windows',str(root/'native-inject/launcher.c'),'-lkernel32','-luser32','-lshell32','-ladvapi32','-o',str(root/'inject/LegacySteam-Injector.exe')],check=True)

import hashlib,re
p=root/'src/LaunchProfiles.cs';text=p.read_text();text=re.sub(r'InjectorHash="[a-f0-9]+",BridgeHash="[a-f0-9]+"','InjectorHash="'+hashlib.sha256((root/'inject/LegacySteam-Injector.exe').read_bytes()).hexdigest()+'",BridgeHash="'+hashlib.sha256((root/'inject/LegacySteam-InjectBridge.dll').read_bytes()).hexdigest()+'"',text);p.write_text(text)
print('Injection components and managed checksums updated. Rebuild managed program with Build.cmd.')
