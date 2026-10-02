// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    SDL Input
//
//-----------------------------------------------------------------------------

#include "i_sdlinput.h"
#include "doomdef.h"
#include "m_misc.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_sdlinput.h"
#include "i_video.h"
#include "d_main.h"
#ifdef AMIGA_MINIGL
#include "g_controls.h"
#include "amiga_mouse.h"
#include "m_menu.h"
#endif

CVAR(v_msensitivityx, 5);
CVAR(v_msensitivityy, 5);
CVAR(v_macceleration, 0);
CVAR(v_mlook, 0);
CVAR(v_mlookinvert, 0);
CVAR(v_yaxismove, 0);
CVAR(v_xaxismove, 0);
CVAR_EXTERNAL(m_menumouse);

float mouse_accelfactor;

int         UseJoystick;
boolean    DigiJoy;
int         DualMouse;

boolean    MouseMode;//false=microsoft, true=mouse systems
boolean window_mouse;
#ifdef AMIGA_MINIGL
static boolean amiga_discard_mouse;
static byte amiga_keys[NUMKEYS];
static boolean amiga_sdl_escape_press;
static boolean amiga_pending_quit, amiga_escape_seen;
static Uint32 amiga_last_escape;
extern boolean Amiga_WindowFocused(void);

static void Amiga_SyncFocus(void) {
    boolean focused = Amiga_WindowFocused();
    event_t ev = {0};
    int key;
    if (focused == window_focused) return;
    window_focused = focused;
    I_Printf("Input focus: %s\n", focused ? "active" : "inactive");
    amiga_discard_mouse = true;
    SDL_GetRelativeMouseState(NULL, NULL);
    if (!focused) {
        ev.type = ev_mouseup;
        D_PostEvent(&ev);
        for (key = 0; key < NUMKEYS; ++key) {
            if (!amiga_keys[key]) continue;
            ev.type = ev_keyup;
            ev.data1 = key;
            D_PostEvent(&ev);
            amiga_keys[key] = 0;
        }
    }
    I_UpdateGrab();
}
#endif

//
// I_TranslateKey
//

static int I_TranslateKey(const int key) {
	int rc = 0;

	switch (key) {
	case SDLK_LEFT:
		rc = KEY_LEFTARROW;
		break;
	case SDLK_RIGHT:
		rc = KEY_RIGHTARROW;
		break;
	case SDLK_DOWN:
		rc = KEY_DOWNARROW;
		break;
	case SDLK_UP:
		rc = KEY_UPARROW;
		break;
	case SDLK_ESCAPE:
		rc = KEY_ESCAPE;
		break;
	case SDLK_RETURN:
		rc = KEY_ENTER;
		break;
	case SDLK_TAB:
		rc = KEY_TAB;
		break;
	case SDLK_F1:
		rc = KEY_F1;
		break;
	case SDLK_F2:
		rc = KEY_F2;
		break;
	case SDLK_F3:
		rc = KEY_F3;
		break;
	case SDLK_F4:
		rc = KEY_F4;
		break;
	case SDLK_F5:
		rc = KEY_F5;
		break;
	case SDLK_F6:
		rc = KEY_F6;
		break;
	case SDLK_F7:
		rc = KEY_F7;
		break;
	case SDLK_F8:
		rc = KEY_F8;
		break;
	case SDLK_F9:
		rc = KEY_F9;
		break;
	case SDLK_F10:
		rc = KEY_F10;
		break;
	case SDLK_F11:
		rc = KEY_F11;
		break;
	case SDLK_F12:
		rc = KEY_F12;
		break;
	case SDLK_BACKSPACE:
		rc = KEY_BACKSPACE;
		break;
	case SDLK_DELETE:
		rc = KEY_DEL;
		break;
	case SDLK_INSERT:
		rc = KEY_INSERT;
		break;
	case SDLK_PAGEUP:
		rc = KEY_PAGEUP;
		break;
	case SDLK_PAGEDOWN:
		rc = KEY_PAGEDOWN;
		break;
	case SDLK_HOME:
		rc = KEY_HOME;
		break;
	case SDLK_END:
		rc = KEY_END;
		break;
	case SDLK_PAUSE:
		rc = KEY_PAUSE;
		break;
	case SDLK_EQUALS:
		rc = KEY_EQUALS;
		break;
	case SDLK_MINUS:
		rc = KEY_MINUS;
		break;
	case SDLK_KP_0:
		rc = KEY_KEYPAD0;
		break;
	case SDLK_KP_1:
		rc = KEY_KEYPAD1;
		break;
	case SDLK_KP_2:
		rc = KEY_KEYPAD2;
		break;
	case SDLK_KP_3:
		rc = KEY_KEYPAD3;
		break;
	case SDLK_KP_4:
		rc = KEY_KEYPAD4;
		break;
	case SDLK_KP_5:
		rc = KEY_KEYPAD5;
		break;
	case SDLK_KP_6:
		rc = KEY_KEYPAD6;
		break;
	case SDLK_KP_7:
		rc = KEY_KEYPAD7;
		break;
	case SDLK_KP_8:
		rc = KEY_KEYPAD8;
		break;
	case SDLK_KP_9:
		rc = KEY_KEYPAD9;
		break;
	case SDLK_KP_PLUS:
		rc = KEY_KEYPADPLUS;
		break;
	case SDLK_KP_MINUS:
		rc = KEY_KEYPADMINUS;
		break;
	case SDLK_KP_DIVIDE:
		rc = KEY_KEYPADDIVIDE;
		break;
	case SDLK_KP_MULTIPLY:
		rc = KEY_KEYPADMULTIPLY;
		break;
	case SDLK_KP_ENTER:
		rc = KEY_KEYPADENTER;
		break;
	case SDLK_KP_PERIOD:
		rc = KEY_KEYPADPERIOD;
		break;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		rc = KEY_RSHIFT;
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		rc = KEY_RCTRL;
		break;
	case SDLK_LALT:

	case SDLK_RALT:
		rc = KEY_RALT;
		break;
	default:
		rc = key;
		break;
	}

	return rc;
}

//
// I_SDLtoDoomMouseState
//

static int I_SDLtoDoomMouseState(Uint8 buttonstate) {
	return 0
		| (buttonstate & SDL_BUTTON_MASK(SDL_BUTTON_LEFT) ? 1 : 0)
		| (buttonstate & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE) ? 2 : 0)
		| (buttonstate & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT) ? 4 : 0);
}

//
// I_ReadMouse
//

void I_ReadMouse(void) {
	float x, y;
	Uint8 btn;
	event_t ev;
	static Uint8 lastmbtn = 0;

	SDL_GetRelativeMouseState(&x, &y);
#ifdef AMIGA_MINIGL
    {
        float rawx, rawy;
        if (Amiga_MouseRead(&rawx, &rawy)) { x = rawx; y = rawy; }
    }
    if (!window_focused || amiga_discard_mouse) {
        lastmbtn = 0;
        amiga_discard_mouse = false;
        return;
    }
#endif
	btn = SDL_GetMouseState(&mouse_x, &mouse_y);

	if (x != 0 || y != 0 || btn || (lastmbtn != btn)) {
		ev.type = ev_mouse;
		ev.data1 = I_SDLtoDoomMouseState(btn);
		ev.data2 = x * 32.0;
		ev.data3 = -y * 32.0;
		ev.data4 = 0;
		D_PostEvent(&ev);
	}

	lastmbtn = btn;
}

void I_CenterMouse(void) {
#ifdef AMIGA_MINIGL
    if (!Amiga_WindowFocused()) return;
#endif
	// Warp the the screen center
	SDL_WarpMouseInWindow(window, (unsigned short)(video_width / 2), (unsigned short)(video_height / 2));

	// Clear any relative movement caused by warping
	SDL_PumpEvents();
	SDL_GetRelativeMouseState(NULL, NULL);
}

//
// I_MouseAccelChange
//

void I_MouseAccelChange(void) {
	mouse_accelfactor = v_macceleration.value / 200.0f + 1.0f;
}

//
// I_MouseAccel
//

float I_MouseAccel(float val) {
	if (!v_macceleration.value) {
		return val;
	}

	if (val < 0) {
		return -I_MouseAccel(-val);
	}

	return (float)(pow((double)val, (double)mouse_accelfactor));
}

//
// I_UpdateGrab
//

boolean I_UpdateGrab(void) {
	static boolean currently_grabbed = false;
	boolean grab;

	grab = /*window_mouse &&*/ !menuactive
		&& (gamestate == GS_LEVEL)
		&& !demoplayback;
#ifdef AMIGA_MINIGL
    grab = grab && window_focused;
#endif

#ifdef AMIGA_MINIGL
    Amiga_MouseGrab(grab);
#endif
	if (grab && !currently_grabbed) {
		SDL_SetWindowRelativeMouseMode(window, 1);
		SDL_SetWindowMouseGrab(window, 1);
		SDL_SetWindowKeyboardGrab(window, 1);
	}

	if (!grab && currently_grabbed) {
		SDL_SetWindowRelativeMouseMode(window, 0);
		SDL_SetWindowMouseGrab(window, 0);
		SDL_SetWindowKeyboardGrab(window, 0);
	}

	currently_grabbed = grab;
#ifdef AMIGA_MINIGL
    /* Menu mouse is free to leave the window. Hide the Amiga pointer only
       where the game's cursor is drawn; gameplay capture also hides it. */
    SDL_ShowCursor(grab || (window_focused && menuactive &&
        m_menumouse.value && Amiga_MouseInClient()) ? SDL_DISABLE : SDL_ENABLE);
#endif

	return currently_grabbed;
}

//
// I_GetEvent
//

#ifdef AMIGA_MINIGL
#define DOOM_EVENT_KEY(e) ((e)->key.keysym.sym)
#else
#define DOOM_EVENT_KEY(e) ((e)->key.key)
#endif

void I_GetEvent(SDL_Event* Event) {
	event_t event;
	unsigned int mwheeluptic = 0, mwheeldowntic = 0;
	unsigned int tic = gametic;
#ifdef AMIGA_MINIGL
    {
        static int inputstats=-1;
        if(inputstats<0) inputstats=M_CheckParm("-amiga-inputstats")!=0;
        if(inputstats && (Event->type==SDL_KEYDOWN || Event->type==SDL_KEYUP))
            I_Printf("Input: type=%u key=%d raw=%u focus=%d menu=%d state=%d\n",
                Event->type,Event->key.keysym.sym,Event->key.keysym.scancode,
                window_focused,menuactive,gamestate);
        if(Event->type==SDL_QUIT)
            I_Printf("Input: SDL_QUIT received, focus=%d menu=%d state=%d\n",
                window_focused,menuactive,gamestate);
    }
#endif

	switch (Event->type) {
	case SDL_EVENT_KEY_DOWN:
#ifdef AMIGA_MINIGL
        if (DOOM_EVENT_KEY(Event) == SDLK_F10 && Amiga_WindowFocused()) {
            I_Quit();
            break;
        }
        if (DOOM_EVENT_KEY(Event) == SDLK_ESCAPE) {
            I_Printf("Input: SDL Esc focus=%d menu=%d state=%d\n",
                window_focused, menuactive, gamestate);
            amiga_sdl_escape_press = true;
        }
        if (!window_focused) break;
#endif
		#ifndef AMIGA_MINIGL
		if (Event->key.repeat)
			break;
#endif
		event.type = ev_keydown;
		event.data1 = I_TranslateKey(DOOM_EVENT_KEY(Event));
#ifdef AMIGA_MINIGL
        if (event.data1 >= 0 && event.data1 < NUMKEYS)
            amiga_keys[event.data1] = Event->type == SDL_KEYDOWN;
#endif
		D_PostEvent(&event);
		break;

	case SDL_EVENT_KEY_UP:
		event.type = ev_keyup;
		event.data1 = I_TranslateKey(DOOM_EVENT_KEY(Event));
#ifdef AMIGA_MINIGL
        if (event.data1 >= 0 && event.data1 < NUMKEYS)
            amiga_keys[event.data1] = Event->type == SDL_KEYDOWN;
#endif
		D_PostEvent(&event);
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
#ifdef AMIGA_MINIGL
        if (!window_focused || amiga_discard_mouse) break;
        if (Event->button.button == SDL_BUTTON_WHEELUP || Event->button.button == SDL_BUTTON_WHEELDOWN) {
            event.type = Event->type == SDL_MOUSEBUTTONDOWN ? ev_keydown : ev_keyup;
            event.data1 = Event->button.button == SDL_BUTTON_WHEELUP ? KEY_MWHEELUP : KEY_MWHEELDOWN;
            event.data2 = event.data3 = 0;
            D_PostEvent(&event);
            break;
        }
#endif
		if (!window_focused)
			break;

		event.type = (Event->type == SDL_EVENT_MOUSE_BUTTON_UP) ? ev_mouseup : ev_mousedown;
		event.data1 =
			I_SDLtoDoomMouseState(SDL_GetMouseState(NULL, NULL));
		event.data2 = event.data3 = 0;

		D_PostEvent(&event);
		break;

#ifdef AMIGA_MINIGL
    case SDL_ACTIVEEVENT:
        /* Classic SDL reports IDCMP activation as APPMOUSEFOCUS, not
           APPINPUTFOCUS. Query the actual Intuition window, not that bit. */
        Amiga_SyncFocus();
        amiga_discard_mouse = true;
        SDL_GetRelativeMouseState(NULL, NULL);
        if (Event->active.state & SDL_APPMOUSEFOCUS) window_mouse = Event->active.gain;
        break;
#else
	case SDL_EVENT_MOUSE_WHEEL:
		if (Event->wheel.y > 0) {
			event.type = ev_keydown;
			event.data1 = KEY_MWHEELUP;
			mwheeluptic = tic;
		}
		else if (Event->wheel.y < 0) {
			event.type = ev_keydown;
			event.data1 = KEY_MWHEELDOWN;
			mwheeldowntic = tic;
		}
		else
			break;

		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		break;

	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		window_focused = true;
		break;

	case SDL_EVENT_WINDOW_FOCUS_LOST:
		window_focused = false;
		break;

	case SDL_EVENT_WINDOW_MOUSE_ENTER:
		window_mouse = true;
		break;

	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		window_mouse = false;
		break;

#endif
	case SDL_EVENT_QUIT:
#ifdef AMIGA_MINIGL
        /* Decide after polling: Esc and a close request can arrive together,
           in either order, on the Amiga SDL window backend. */
        amiga_pending_quit = true;
#else
		I_Quit();
#endif
		break;

	default:
		break;
	}

	if (mwheeluptic && mwheeluptic + 1 < tic) {
		event.type = ev_keyup;
		event.data1 = KEY_MWHEELUP;
		D_PostEvent(&event);
		mwheeluptic = 0;
	}

	if (mwheeldowntic && mwheeldowntic + 1 < tic) {
		event.type = ev_keyup;
		event.data1 = KEY_MWHEELDOWN;
		D_PostEvent(&event);
		mwheeldowntic = 0;
	}
}

//
// I_ShutdownWait
//

int I_ShutdownWait(void) {
	static SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT ||
			(event.type == SDL_EVENT_KEY_DOWN && DOOM_EVENT_KEY(&event) == SDLK_ESCAPE)) {
			I_ShutdownVideo();
			return 1;
		}
	}

	return 0;
}

//
// I_StartTic
//

void I_StartTic(void) {
	SDL_Event Event;
#ifdef AMIGA_MINIGL
    amiga_sdl_escape_press = false;
    amiga_pending_quit = false;
    Amiga_SyncFocus();
#endif

	while (SDL_PollEvent(&Event)) {
		I_GetEvent(&Event);
	}
#ifdef AMIGA_MINIGL
    {
        if (Amiga_ReadQuitPress() && Amiga_WindowFocused())
            I_Quit();
        int presses = Amiga_ReadEscapePress();
        event_t escape = {0};
        if (amiga_sdl_escape_press || presses) {
            amiga_last_escape = SDL_GetTicks();
            amiga_escape_seen = true;
        }
        if (amiga_pending_quit) {
            if (amiga_escape_seen && SDL_GetTicks() - amiga_last_escape < 250) {
                I_Printf("Input: ignoring SDL_QUIT accompanying Esc\n");
            } else {
                I_Printf("Input: window Close -> quit\n");
                I_Quit();
            }
        }
        if (presses)
            I_Printf("Input: native Esc presses=%d SDL=%d focus=%d menu=%d state=%d\n",
                presses, amiga_sdl_escape_press, window_focused, menuactive, gamestate);
        /* Prefer SDL delivery; native input is only a fallback. */
        if (amiga_sdl_escape_press) presses = 0;
        if (!Amiga_WindowFocused()) presses = 0;
        while (presses-- > 0) {
            escape.type = ev_keydown;
            escape.data1 = KEY_ESCAPE;
            D_PostEvent(&escape);
            escape.type = ev_keyup;
            D_PostEvent(&escape);
        }
    }
#endif

#if defined(_WIN32) && defined(USE_XINPUT)
	I_XInputPollEvent();
#endif
	I_InitInputs();
#ifdef AMIGA_MINIGL
    Amiga_SyncFocus();
    I_UpdateGrab();
#endif
	I_ReadMouse();
}

//
// I_FinishUpdate
//

void I_FinishUpdate(void) {
#ifdef AMIGA_MINIGL
    extern void Amiga_TestFrame(void);
    extern void Amiga_VideoStats(void);
    Amiga_VideoStats();
    Amiga_TestFrame();
#endif
	I_UpdateGrab();
	SDL_GL_SwapWindow(window);
#ifdef AMIGA_MINIGL
    {
        extern void Amiga_StartAudio(void);
        extern void Amiga_FrameSync(void);
        Amiga_StartAudio();
        Amiga_FrameSync();
    }
#endif

	BusyDisk = false;
}

//
// I_InitInputs
//

void I_InitInputs(void) {
	SDL_PumpEvents();
	I_MouseAccelChange();

#if defined(_WIN32) && defined(USE_XINPUT)
	I_XInputInit();
#endif
}

//
// ISDL_RegisterCvars
//

void ISDL_RegisterKeyCvars(void) {
	CON_CvarRegister(&v_msensitivityx);
	CON_CvarRegister(&v_msensitivityy);
	CON_CvarRegister(&v_macceleration);
	CON_CvarRegister(&v_mlook);
	CON_CvarRegister(&v_mlookinvert);
	CON_CvarRegister(&v_yaxismove);
	CON_CvarRegister(&v_xaxismove);
}
