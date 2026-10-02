/* Narrow SDL3 source adapter for the engine's SDL 1.2 Amiga backend. */
#ifndef DOOM_AMIGA_SDL_H
#define DOOM_AMIGA_SDL_H
#include <SDL/SDL.h>
#include <stdlib.h>
typedef SDL_Surface SDL_Window;
#define SDL_BUTTON_MASK SDL_BUTTON
#define SDL_GetBasePath() "PROGDIR:"
#define SDL_free free
#define SDL_GL_SwapWindow(w) SDL_GL_SwapBuffers()
#define SDL_WarpMouseInWindow(w,x,y) SDL_WarpMouse(x,y)
#define SDL_SetWindowRelativeMouseMode(w,on) SDL_ShowCursor(!(on))
#define SDL_SetWindowMouseGrab(w,on) SDL_WM_GrabInput((on)?SDL_GRAB_ON:SDL_GRAB_OFF)
#define SDL_SetWindowKeyboardGrab(w,on) SDL_WM_GrabInput((on)?SDL_GRAB_ON:SDL_GRAB_OFF)
static inline Uint8 Amiga_MouseState(float *x, float *y, int relative) {
    int ix, iy;
    Uint8 buttons = relative ? SDL_GetRelativeMouseState(&ix,&iy) : SDL_GetMouseState(&ix,&iy);
    if (x) *x=ix;
    if (y) *y=iy;
    return buttons;
}
#define SDL_GetMouseState(x,y) Amiga_MouseState(x,y,0)
#define SDL_GetRelativeMouseState(x,y) Amiga_MouseState(x,y,1)
#define SDL_EVENT_KEY_DOWN SDL_KEYDOWN
#define SDL_EVENT_KEY_UP SDL_KEYUP
#define SDL_EVENT_MOUSE_BUTTON_DOWN SDL_MOUSEBUTTONDOWN
#define SDL_EVENT_MOUSE_BUTTON_UP SDL_MOUSEBUTTONUP
#define SDL_EVENT_QUIT SDL_QUIT
#define SDLK_KP_0 SDLK_KP0
#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_NUMLOCKCLEAR SDLK_NUMLOCK
#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
#endif
