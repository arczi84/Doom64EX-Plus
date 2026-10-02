/* AmigaOS 3 SDL 1.2 / shared MiniGL video backend. */
#include <intuition/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include "doomdef.h"
#include "doomstat.h"
#include "i_video.h"
#include "i_system.h"
#include "i_sdlinput.h"
#include "m_misc.h"
#include "d_main.h"
#include "gl_main.h"
#include "gl_texture.h"
#include "amiga_vertex_batch.h"
#include "amiga_mouse.h"

unsigned long __stack = 1024 * 1024;
static const char version_tag[] __attribute__((used)) = "$VER: Doom64EX-Plus MiniGL 0.1 (1.10.2026)";
SDL_Window *window;
SDL_Surface *screen;
int video_width, video_height;
float video_ratio, display_scale = 1.0f;
float mouse_x, mouse_y;
boolean window_focused;
CVAR(v_width, 640);
CVAR(v_height, 480);
CVAR(v_windowed, 1);
CVAR(v_windowborderless, 0);
CVAR_CMD(v_vsync, 1) {
    if(window && MiniGLDispatch) mglEnableSync(cvar->value != 0 ? GL_TRUE : GL_FALSE);
}
/* Windowed MiniGL only ClipBlits; it does not wait for vertical blank. */
void Amiga_FrameSync(void) {
    if(InWindow && v_vsync.value != 0 && GfxBase) WaitTOF();
}
int amiga_intro_finished;
/* Optional diagnostics: SDL storage format and the actual Workbench bitmap. */
void Amiga_VideoStats(void) {
    static int enabled=-1, previous_state=-1;
    static ULONG previous_depth,previous_bytes,previous_format;
    struct Window *native;
    struct BitMap *bitmap;
    ULONG depth,bytes,format;
    if(enabled<0) enabled=M_CheckParm("-amiga-videostats")!=0;
    if(!enabled || !window || !MiniGLDispatch || !CyberGfxBase) return;
    native=(struct Window *)mglGetWindowHandle();
    if(!native || !native->WScreen || !native->RPort) return;
    bitmap=native->RPort->BitMap;
    depth=GetCyberMapAttr(bitmap,CYBRMATTR_DEPTH);
    bytes=GetCyberMapAttr(bitmap,CYBRMATTR_BPPIX);
    format=GetCyberMapAttr(bitmap,CYBRMATTR_PIXFMT);
    if(previous_state!=(int)gamestate || previous_depth!=depth ||
       previous_bytes!=bytes || previous_format!=format) {
        I_Printf("Video format: state=%d map=%d SDL=%d bpp/%d bytes RTG=%lu bits/%lu bytes pixfmt=%lu screen=%dx%d window=%dx%d\n",
            (int)gamestate,gamemap,screen->format->BitsPerPixel,screen->format->BytesPerPixel,
            depth,bytes,format,native->WScreen->Width,native->WScreen->Height,
            video_width,video_height);
        previous_state=gamestate; previous_depth=depth; previous_bytes=bytes; previous_format=format;
    }
}

boolean Amiga_WindowFocused(void) {
    struct Window *native;
    if (!window || !MiniGLDispatch) return false;
    native = (struct Window *)mglGetWindowHandle();
    return native && (native->Flags & WFLG_WINDOWACTIVE);
}

boolean Amiga_MouseInClient(void) {
    struct Window *native;
    if (!window || !MiniGLDispatch) return false;
    native = (struct Window *)mglGetWindowHandle();
    return native && native->MouseX >= native->BorderLeft &&
        native->MouseX < native->Width - native->BorderRight &&
        native->MouseY >= native->BorderTop &&
        native->MouseY < native->Height - native->BorderBottom;
}

void I_InitScreen(void) {
    int p;
    static int initial_mode = 1;
    unsigned flags = SDL_OPENGL;
    InWindow = v_windowed.value != 0;
    if (initial_mode && M_CheckParm("-fullscreen")) InWindow = 0;
    if (initial_mode && M_CheckParm("-window")) InWindow = 1;
    video_width = (int)v_width.value;
    video_height = (int)v_height.value;
    p = M_CheckParm("-width");
    if (initial_mode && p && p < myargc-1) video_width = atoi(myargv[p+1]);
    p = M_CheckParm("-height");
    if (initial_mode && p && p < myargc-1) video_height = atoi(myargv[p+1]);
    if (video_width < 320 || video_height < 200) I_Error("Invalid video dimensions");
    video_ratio = (float)video_width / video_height;
    CON_CvarSetValue(v_width.name, video_width);
    CON_CvarSetValue(v_height.name, video_height);
    CON_CvarSetValue(v_windowed.name, InWindow);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    if (!InWindow) flags |= SDL_FULLSCREEN;
    if (!MiniGLOpen()) I_Error("Cannot open minigl.library");
    mglChooseVertexBufferSize(AMIGA_MGL_VERTEX_CAPACITY);
    screen = window = SDL_SetVideoMode(video_width, video_height, 32, flags);
    if (!screen) I_Error("MiniGL video: %s", SDL_GetError());
    SDL_WM_SetCaption("Doom64EX-Plus MiniGL", NULL);
    mglEnableSync(v_vsync.value != 0 ? GL_TRUE : GL_FALSE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();
    if (!Amiga_MouseInit()) I_Error("Cannot initialize native mouse capture");
    SDL_ShowCursor(SDL_ENABLE);
    window_focused = Amiga_WindowFocused();
    usingGL = false;
    initial_mode = 0;
}
void I_InitVideo(void) {
    if (window) {
        /* MiniGL cannot resize its windowed offscreen bitmap in place.
           Release texture names while the old context is still valid,
           then recreate SDL's window/context without stopping audio. */
        I_Printf("Video Apply: recreating MiniGL context (%dx%d -> %.0fx%.0f)\n",
            video_width, video_height, v_width.value, v_height.value);
        Amiga_MouseShutdown();
        SDL_WM_GrabInput(SDL_GRAB_OFF);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_GL_SwapBuffers();
        if (usingGL) {
            glFinish();
            GL_DumpTextures();
            usingGL = false;
        }
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        MiniGLClose();
        screen = window = NULL;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) I_Error("SDL: %s", SDL_GetError());
    I_InitScreen();
}
void I_ShutdownVideo(void) {
    Amiga_MouseShutdown();
    if (window) {
        I_Printf("Video shutdown: release input\n");
        SDL_WM_GrabInput(SDL_GRAB_OFF);
        I_Printf("Video shutdown: show cursor\n");
        SDL_ShowCursor(SDL_ENABLE);
        /* Readback/draw may leave a manual MiniGL display lock held. */
        I_Printf("Video shutdown: swap\n");
        SDL_GL_SwapBuffers();
        if (usingGL) {
            I_Printf("Video shutdown: textures\n");
            I_Printf("Video shutdown: glFinish begin\n");
            glFinish();
            I_Printf("Video shutdown: GL_DumpTextures begin\n");
            GL_DumpTextures();
            I_Printf("Video shutdown: textures done\n");
            usingGL = false;
        }
    }
    I_Printf("Video shutdown: timer\n");
    SDL_QuitSubSystem(SDL_INIT_TIMER);
    I_Printf("Video shutdown: video\n");
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    I_Printf("Video shutdown: SDL_Quit\n");
    SDL_Quit();
    MiniGLClose();
    I_Printf("Video shutdown: done\n");
    screen = window = NULL;
}
void V_RegisterCvars(void) {
    CON_CvarRegister(&v_width);
    CON_CvarRegister(&v_height);
    CON_CvarRegister(&v_windowed);
    CON_CvarRegister(&v_windowborderless);
    CON_CvarRegister(&v_vsync);
}

/* Round-trip four colors through the exact padded framebuffer-copy path
   used by fades. Compare RGB and RGBA destinations independently. */
static int Amiga_CopyTextureTest(vtx_t *v, GLuint source) {
    const unsigned char pattern[16]={255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,0,255};
    unsigned char reference[4][4],actual[4];
    int mode,i,c,fail[3]={0,0,0};
    GLuint snapshot;
    unsigned char *before=malloc(320*240*4),*after=malloc(320*240*4);
    if(!before || !after) I_Error("Copy test: out of memory");
    video_width=320;video_height=240;
    glBindTexture(GL_TEXTURE_2D,source);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,pattern);
    for(mode=0;mode<3;++mode) {
        glBindTexture(GL_TEXTURE_2D,source);
        v[1].tu=v[3].tu=1; v[2].tv=v[3].tv=1;
        glClear(GL_COLOR_BUFFER_BIT);
        dglSetVertex(v);dglTriangle(0,1,2);dglTriangle(1,3,2);dglDrawGeometry(4,v);
        glFinish();
        for(i=0;i<4;++i) glReadPixels(80+160*(i&1),60+120*(i>>1),1,1,GL_RGBA,GL_UNSIGNED_BYTE,reference[i]);
        glReadPixels(0,0,320,240,GL_RGBA,GL_UNSIGNED_BYTE,before);
        if(mode==2) snapshot=GL_ScreenToTexture();
        else {
        glGenTextures(1,&snapshot);glBindTexture(GL_TEXTURE_2D,snapshot);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D,0,mode?GL_RGBA:GL_RGB,512,256,0,mode?GL_RGBA:GL_RGB,GL_UNSIGNED_BYTE,NULL);
        video_width=320;video_height=240;
        glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,512,256);
        }
        v[1].tu=v[3].tu=320.0f/512;v[2].tv=v[3].tv=240.0f/256;
        glClear(GL_COLOR_BUFFER_BIT);
        dglSetVertex(v);dglTriangle(0,1,2);dglTriangle(1,3,2);dglDrawGeometry(4,v);
        glFinish();
        for(i=0;i<4;++i) {
            glReadPixels(80+160*(i&1),60+120*(i>>1),1,1,GL_RGBA,GL_UNSIGNED_BYTE,actual);
            for(c=0;c<3;++c) if(abs((int)actual[c]-reference[i][c])>4) ++fail[mode];
            printf("Copy %s quadrant %d: expected %u,%u,%u got %u,%u,%u\n",
                mode==2?"engine":mode?"RGBA":"RGB",i,reference[i][0],reference[i][1],reference[i][2],actual[0],actual[1],actual[2]);
        }
        glReadPixels(0,0,320,240,GL_RGBA,GL_UNSIGNED_BYTE,after);
        for(i=0;i<320*240;++i) for(c=0;c<3;++c)
            if(abs((int)after[i*4+c]-before[i*4+c])>4) ++fail[mode];
        printf("Copy %s: mismatches=%d GL error=%u\n",mode==2?"engine":mode?"RGBA":"RGB",fail[mode],glGetError());
        glDeleteTextures(1,&snapshot);
    }
    free(before);free(after);
    return fail[1]==0 && fail[2]==0; /* RGBA and the engine path must preserve all colors. */
}

/* Data-free on-target ABI/readback test, using the engine's indexed draw path. */
int Amiga_SmokeTest(void) {
    GLuint texture;
    unsigned char pixels[16] = {255,0,0,255, 255,0,0,255, 255,0,0,255, 255,0,0,255};
    unsigned char result[4] = {0,0,0,0};
    GLboolean enabled = GL_FALSE;
    GLenum error;
    int ok;
    vtx_t vertices[4] = {
        {-1,-1,0, 0,0, 255,255,255,255}, {1,-1,0, 1,0, 255,255,255,255},
        {-1,1,0, 0,1, 255,255,255,255}, {1,1,0, 1,1, 255,255,255,255}
    };
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) { fprintf(stderr,"SDL: %s\n",SDL_GetError()); return 20; }
    if (!MiniGLOpen()) { SDL_Quit(); return 20; }
    mglChooseVertexBufferSize(AMIGA_MGL_VERTEX_CAPACITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    if (!SDL_SetVideoMode(320,240,32,SDL_OPENGL)) {
        fprintf(stderr,"Video: %s\n",SDL_GetError()); SDL_Quit(); return 20;
    }
    printf("MiniGL smoke: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
    glViewport(0,0,320,240);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_CULL_FACE); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glGetBooleanv(GL_DEPTH_WRITEMASK,&enabled);
    glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    dglSetVertex(vertices); dglTriangle(0,1,2); dglTriangle(1,3,2); dglDrawGeometry(4,vertices);
    glReadPixels(160,120,1,1,GL_RGBA,GL_UNSIGNED_BYTE,result);
    error=glGetError();
    ok=enabled && result[0]>240 && result[1]<16 && result[2]<16 && error==GL_NO_ERROR;
    printf("MiniGL smoke %s: pixel=%u,%u,%u,%u texture=%u error=%u\n",
        ok?"PASS":"FAIL",result[0],result[1],result[2],result[3],enabled,error);
    if(M_CheckParm("-amiga-copytest")) ok=Amiga_CopyTextureTest(vertices,texture) && ok;
    SDL_GL_SwapBuffers(); SDL_Delay(500);
    if (M_CheckParm("-amiga-mousetest")) {
        if (!Amiga_MouseInit() || !Amiga_MouseTest()) ok = 0;
        Amiga_MouseShutdown();
    }
    glDeleteTextures(1,&texture); SDL_Quit();
    return ok?0:20;
}

/* Optional reproducible map-start check: capture after N gameplay frames. */
void Amiga_TestFrame(void) {
    static int limit = -1, frames;
    static int turn;
    int p;
    if(M_CheckParm("-amiga-esctest") && gamestate==GS_LEVEL && leveltime>=3*TICRATE) {
        static int phase;
        static Uint32 sent;
        Uint32 now=SDL_GetTicks();
        if(!phase) {
            if(menuactive || !Amiga_WindowFocused()) I_Error("Esc test: invalid initial state");
            I_Printf("Esc test: sending native Esc down\n");
            if(!Amiga_TestRawKey(0x45)) I_Error("Esc test: key injection failed");
            sent=now;phase=1;
        } else if((phase==1 || phase==3) && now-sent>=100) {
            if(!Amiga_TestRawKey(0xc5)) I_Error("Esc test: release failed");
            ++phase;
        } else if(phase==2 && now-sent>=1000) {
            I_Printf("Esc test: menu after first Esc=%d\n",menuactive);
            if(!menuactive) I_Error("Esc did not open menu");
            if(!Amiga_TestRawKey(0x45)) I_Error("Esc test: second press failed");
            phase=3;sent=now;
        } else if(phase==4 && now-sent>=1000) {
            I_Printf("Esc test: menu after second Esc=%d\n",menuactive);
            if(menuactive) I_Error("Esc did not resume game");
            I_Printf("Esc test PASS: native key opens menu and resumes gameplay\n");
            I_Quit();
        }
    }
    if(M_CheckParm("-amiga-introtest") && amiga_intro_finished) {
        static Uint32 menu_start;
        static unsigned menu_frames;
        Uint32 now=SDL_GetTicks();
        if(!menu_start) menu_start=now;
        ++menu_frames;
        if(now-menu_start>=10000) {
            extern int amiga_target_errors;
            I_Printf("Amiga menu test: %u frames in %lu ms, target errors=%d\n",
                menu_frames,(unsigned long)(now-menu_start),amiga_target_errors);
            if(amiga_target_errors) I_Error("Intro target validation failed");
            I_Quit();
        }
    }
    if (limit == -1) {
        turn=M_CheckParm("-amiga-testturn");
        p=M_CheckParm("-amiga-testframes");
        limit=(p && p<myargc-1)?atoi(myargv[p+1]):0;
    }
    if (limit <= 0 || gamestate != GS_LEVEL || leveltime < 3*TICRATE) return;
    if (++frames < limit) {
        if (turn && players[consoleplayer].mo)
            players[consoleplayer].mo->angle += ANG45 / 32;
        return;
    }
    {
        byte *rgb;
        FILE *out;
        GLenum render_error=glGetError();
        rgb=GL_GetScreenBuffer(0,0,video_width,video_height);
        out=fopen("amiga-frame.ppm","wb");
        if (!out) I_Error("Cannot write amiga-frame.ppm");
        fprintf(out,"P6\n%d %d\n255\n",video_width,video_height);
        if (fwrite(rgb,3,(size_t)video_width*video_height,out)!=(size_t)video_width*video_height)
            I_Error("Cannot finish amiga-frame.ppm");
        fclose(out);
        I_Printf("Amiga map test: %d frames, render GL error=%u, readback GL error=%u\n",
                 frames,render_error,glGetError());
        /* I_Quit frees the process-owned zone allocation on exit. */
        I_Quit();
    }
}
