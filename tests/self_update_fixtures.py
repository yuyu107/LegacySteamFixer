"""Generate independent ZIP fixtures after building the runtime and a future-version stub.
Usage: python self_update_fixtures.py RUNTIME_DIR FUTURE_VERSION_EXE OUTPUT_DIR
"""
from pathlib import Path
import sys, zipfile
runtime, stub, output=map(Path,sys.argv[1:]);output.mkdir(parents=True,exist_ok=True)
files={p: (runtime/p).read_bytes() for p in ['LegacySteamFixer.exe.config','modules/catalog.json','inject/LegacySteam-Injector.exe','inject/LegacySteam-InjectBridge.dll','templates/bridge-x64.dll','zstd/LegacySteam-ZstdModule.dll','zstd/steam_zstd.dll','zstd/vsza_hook.bin']}
files['LegacySteamFixer.exe']=stub.read_bytes()
files['README.txt']=b'test package'
def write(name, entries):
    with zipfile.ZipFile(output/(name+'.zip'),'w',zipfile.ZIP_DEFLATED) as z:
        for path,data in entries:z.writestr('LegacySteamFixer/'+path,data)
write('valid',files.items())
write('traversal',[('../escape.txt',b'bad')])
write('reserved',[('CON.txt',b'bad')])
write('duplicate',[('README.txt',b'a'),('readme.txt',b'b')])
write('protected',list(files.items())+[('profiles/game.json',b'overwrite')])
write('missing',[(p,v) for p,v in files.items() if p!='zstd/steam_zstd.dll'])
with zipfile.ZipFile(output/'crc.zip','w',zipfile.ZIP_STORED) as z:z.writestr('README.txt',b'unique payload')
p=output/'crc.zip';p.write_bytes(p.read_bytes().replace(b'unique payload',b'broken payload'))
