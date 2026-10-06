# Doom64 Amiga source and build

The only active Doom64 Amiga source checkout is this repository:
`D:\dev\Doom64\Doom64EX-Plus` (`/mnt/d/dev/Doom64/Doom64EX-Plus`).
Do not create or build a second checkout under jazz2. Preserve the existing
committed fixes, especially save/load metadata caching, explicit MiniGL panel
outlines, native cursor/input handling, percentage audio volume, settings and
safe video-context recreation/animated palette allocations.

Build with `make -f Makefile.amiga`. V29 output is in `build-amiga-v29`.
MiniGL SDK: `D:\dev\Pistorm3D\MiniGL_Classic_v29.1\libraryroot`.
The matching corrected Classic library is in `MiniGL_Classic_v29.1`.

Copy tested game executables to `D:\dev\Doom64\Doom64EX-Plus-MiniGL`.
Classic libraries belong in `D:\dev\Pistorm3D\MiniGL_Classic_v29.1`.
Do not deliver Classic files to aaftp/_pistorm, aaftp/_classic or the main
Pistorm3D directory. Do not create ZIP/7z packages without an explicit request.

Compilation alone does not prove rendering/menu fixes. Inspect save and load
menu captures and verify the actual Doom video-menu reset path when changing
video modes. Keep vertical-blank waiting active in both SDL modes with vsync.
Music quality remains 22050 Hz stereo; do not reduce it to 11025 Hz.
