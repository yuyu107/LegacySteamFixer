"""Build the x86 callable Zstd module with pip's ziglang (CI uses 0.16.0; also tested locally with 0.17.0)."""
from pathlib import Path
import hashlib, re, subprocess
import ziglang
root=Path(__file__).resolve().parents[2]
zig=Path(ziglang.__path__[0])/('zig.exe' if __import__('os').name=='nt' else 'zig')
out=root/'zstd/LegacySteam-ZstdModule.dll'
subprocess.run([str(zig),'cc','-target','x86-windows-gnu','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-nostdlib','-isystem',str(zig.parent/'lib/libc/include/any-windows-any'),'-shared','-Wl,--entry,DllMain@12','-Wl,--major-os-version,6','-Wl,--minor-os-version,1','-Wl,--major-subsystem-version,6','-Wl,--minor-subsystem-version,1',str(root/'zstd/source/unified_i18n.c'),'-lkernel32','-luser32','-ladvapi32','-o',str(out)],check=True)
p=root/'src/ZstdSupport.cs'
s=p.read_text();s=re.sub(r'\{"LegacySteam-ZstdModule.dll","[a-f0-9]+"\}', '{"LegacySteam-ZstdModule.dll","'+hashlib.sha256(out.read_bytes()).hexdigest()+'"}',s);p.write_text(s)

p=root/"zstd/UPSTREAM.txt"
s=p.read_text();s=re.sub(r"[a-f0-9]{64}  LegacySteam-ZstdModule.dll",hashlib.sha256(out.read_bytes()).hexdigest()+"  LegacySteam-ZstdModule.dll",s);p.write_text(s)
