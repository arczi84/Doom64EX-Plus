#!/usr/bin/env python3
"""Package the built test executable and redistributable support files."""
from pathlib import Path
import hashlib
import zipfile

root = Path(__file__).resolve().parent.parent
files = {
    'Doom64EX-Plus-MiniGL': root / 'build-amiga-v29/Doom64EX-Plus-MiniGL',
    'doom64ex-plus.wad': root / 'doom64ex-plus.wad',
    'README.txt': root / 'amiga/README.txt',
    'Start-Doom64': root / 'amiga/Start-Doom64',
    'COPYING.SDL': root / 'amiga/vendor/COPYING.SDL',
    'VALIDATION.txt': root / 'amiga/VALIDATION.txt',
    'COPYING.SDL_mixer': root / 'amiga/vendor/SDL_mixer/COPYING',
    'doomsnd.sf2': root / 'doomsnd.sf2',
    'COPYING.TinySoundFont': root / 'amiga/vendor/TinySoundFont/LICENSE',
    **{name: root / name for name in ('COPYING', 'DOOMLIC', 'AUTHORS')},
}
for path in files.values():
    if not path.is_file():
        raise SystemExit(f'Missing package input: {path}')
dest = root / 'dist/Doom64EX-Plus-MiniGL-test.zip'
dest.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(dest, 'w', zipfile.ZIP_DEFLATED) as archive:
    checksums = []
    for name, path in files.items():
        data = path.read_bytes()
        archive.writestr('Doom64EX-Plus-MiniGL/' + name, data)
        checksums.append(hashlib.sha256(data).hexdigest() + '  ' + name)
    archive.writestr('Doom64EX-Plus-MiniGL/SHA256SUMS', '\n'.join(checksums) + '\n')
print(dest)
