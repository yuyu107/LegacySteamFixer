"""Package an already built runtime; emits the filenames consumed by UpdateService."""
from pathlib import Path
import hashlib, re, sys, zipfile
root=Path(__file__).resolve().parents[1]
version=re.search(r'const string Version="([^"]+)"',(root/'src/AppInfo.cs').read_text()).group(1)
output=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'dist'
output.mkdir(parents=True,exist_ok=True)
files=[root/p for p in ['LegacySteamFixer.exe','LegacySteamFixer.exe.config','README.txt','RELEASE_NOTES.md','VALIDATION.txt','LICENSE.txt','LICENSE-GMod.txt','LICENSE-tModLoader.txt','UPSTREAM.txt']]
for directory in ['modules','inject','templates','probes','zstd']:
    files.extend(p for p in (root/directory).rglob('*') if p.is_file() and 'source' not in p.relative_to(root).parts and p.suffix not in ['.pdb','.obj','.lib'])
for required in ['probes/SteamProbe-x86.exe','probes/SteamProbe-x64.exe','zstd/LegacySteam-ZstdModule.dll','zstd/steam_zstd.dll','zstd/vsza_hook.bin']:
    if not (root/required).is_file():raise SystemExit('Missing runtime file: '+required)
if any(p.name=='OldSteam_VSZa_Launcher.exe' for p in files):raise SystemExit('Obsolete standalone Zstd launcher must not be packaged')
name='LegacySteamFixer-v'+version
package=output/(name+'.zip')
with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as z:
    for path in sorted(files):z.write(path,'LegacySteamFixer/'+path.relative_to(root).as_posix())
checksum=hashlib.sha256(package.read_bytes()).hexdigest()
(output/(name+'.sha256.txt')).write_text(checksum+'  '+package.name+'\n')
print(package)
print(checksum)
