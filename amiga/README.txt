Doom64EX-Plus MiniGL / AmigaOS 3 - initial test port
==================================================
Based on BSzili/Doom64EX-Plus stable, commit 1336e1b.
Target: 68060 + FPU, RTG, working Warp3D/PiStorm3D driver, AHI.
Use the local MiniGL Classic v27 runtime (including its readback fixes).
The program uses the v12 shared dispatch ABI, also available in v27.
Before creating the SDL context it reserves 4096 MiniGL scratch vertices;
indexed triangles are rebased into batches of at most 192 vertices.
No SDL3 or FMOD installation is needed. SDL 1.2 and SDL_mixer are linked in.

INSTALL / RUN
Copy Doom64EX-Plus-MiniGL and doom64ex-plus.wad into a writable directory.
Add DOOM64.WAD from your own 2020 Doom 64 remaster. Legacy ROM-converted
Doom64EX IWADs are not compatible. Use your installed LIBS:minigl.library;
this port does not replace system libraries.

From an Amiga Shell in that directory:
  Stack 2000000
  SetEnv MINIGL_MAX_TEXTURE_SIZE 1024
  Doom64EX-Plus-MiniGL -window -width 640 -height 480
or:
  Doom64EX-Plus-MiniGL -fullscreen -width 640 -height 480
Start-Doom64 is an Execute script with these defaults.
MINIGL_MAX_TEXTURE_SIZE limits texture dimensions; it does not control the
vertex scratch buffer. Use -nosound to disable audio, -nomusic to disable music.
v_vsync defaults to 1. Windowed MiniGL explicitly waits for vertical blank
at frame presentation, yielding the CPU rather than drawing unlimited menu
frames. Fullscreen uses MiniGL's synchronization setting.
Save/config paths resolve to PROGDIR:.
Mouse movement is captured through input.device while the game window is
active. Escape opens the menu and releases the pointer. Losing focus also
releases capture; held Amiga keys allow system pointer shortcuts.
In WinUAE disable Magic Mouse, which bypasses the game pointer capture.

TEST WITHOUT GAME DATA
  Stack 2000000
  Doom64EX-Plus-MiniGL -amiga-smoketest > smoke.log
This opens a 320x240 window and verifies the engine's indexed vertex/color/UV
arrays, sized RGBA texture upload, GLboolean ABI, and framebuffer readback.
It prints PASS/FAIL and exits with 0/20. It does not test the game itself.
Add -amiga-copytest to reproduce the RGB framebuffer-copy corruption and
verify the RGBA snapshot used by the engine over the entire test image.
With an IWAD, -amiga-wipetest checks the real legal-screen fade at the
selected resolution, allowing the observed ARGB4444 color conversion,
and exits after that fade. These readback tests run only when requested.
With an IWAD, -warp 1 -amiga-esctest sends native Esc down/up through
input.device, checks that the menu opens, then checks that a second Esc
resumes gameplay. It exits after the test. -amiga-inputstats logs SDL key
events, raw codes, focus and menu state; SDL_QUIT is always logged.
With an IWAD, -amiga-introtest runs the full MAP33 intro plus ten seconds
of the title menu, reports invalid target references and exits. Add
-amiga-audiostats to report mixer timing at shutdown.
With an IWAD, -warp 1 -amiga-testframes 90 waits at least three seconds of
level time, renders 90 additional frames, writes amiga-frame.ppm, logs GL
errors and exits through the normal settings/audio/video shutdown path.

AUDIO / MUSIC
Sound effects load directly from the IWAD's DS_START/DS_END WAV lumps.
SDL_mixer outputs 22050 Hz, signed 16-bit stereo through SDL/AHI (32 voices).
FMOD reverb and low-pass effects are unavailable.

Music reads MIDI directly from the active WAD and synthesizes it from the
included doomsnd.sf2 bank using TinySoundFont/TinyMidiLoader. Place doomsnd.sf2
beside the executable. The bank and current MIDI load into RAM before playback;
the audio callback performs no disk I/O. No music conversion or music/ WAV
folder is needed. The synthesizer uses up to 32 voices at 22050 Hz stereo.
Tracks currently loop as whole songs with a two-second release tail;
original per-track sequencer loop markers, FMOD reverb and SF2 modulators
are not reproduced. Sound quality and CPU cost still need gameplay testing.
Use -nomusic to compare performance with SFX alone.

RENDERING
Screen fades copy the framebuffer into an RGBA texture: MiniGL corrupts
RGBA sub-uploads into RGB textures, producing vertical colored stripes.
The MiniGL build locks r_texturecombiner to 0 and uses one texture unit.
Sector additive texture stages, RGB_SCALE and anisotropic filtering are
unavailable, so lighting can differ from the desktop renderer. Colored
vertex lighting, fog, alpha blending and screen flash overlays remain in
the draw path. Wireframe debug rendering is not supported by MiniGL.
Mirrored world textures are expanded into cached mirrored tiles with scaled
UVs, rather than sending unsupported GL_MIRRORED_REPEAT to the library.
Readback uses GLReadPixels plus texture sub-upload for screen/wipe textures.

BUILD (this workstation)
  make -f Makefile.amiga -j8
  make -f Makefile.amiga check

Override AMIGA_ROOT, DEV_ROOT, MGL_SDK, SDL_ROOT, and SDL_LIB for
another machine. Defaults match the local OpenLara toolchain/libraries:
  /opt/amiga16-copy: GCC 16, libnix, 68060 hard-float, -O2, no fast-math
  /mnt/d/dev/Pistorm3D/Pistorm3D_v12: MiniGL shared dispatch SDK
  /mnt/d/dev/Pistorm3D/tests/sdl-fullscreen-fallback: SDL MiniGL build
  amiga/vendor/SDL_mixer: pinned SDL 1.2 mixer sources, WAV sound-effect loader
libpng and zlib come from the selected compiler. SDL_mixer is rebuilt with
the same compiler; an older prebuilt archive crashed during Mix_CloseAudio.
TinySoundFont/TinyMidiLoader: pinned sources under amiga/vendor/TinySoundFont,
with a local big-endian SF2 loading fix. See README.local and LICENSE there.
The original Makefile and SDL3/FMOD backend remain available for desktops.

VALIDATION / LIMITS
See VALIDATION.txt for the checks actually performed.
Long gameplay sessions, save/load, mouse focus, sound quality and real
hardware still require testing. Network play is not a
supported feature of this initial Amiga build. This is a test port, not a
hardware-validated release. No speed/FPS claim is made.

Licensing: see COPYING, DOOMLIC and AUTHORS from the upstream project.
The commercial IWAD is not included.
