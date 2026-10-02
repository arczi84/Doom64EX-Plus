/* Native relative mouse input: Classic SDL's GrabInput/WarpMouse are stubs. */
#include <exec/interrupts.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/minigl.h>
#include <stdio.h>
#include <string.h>
#include "amiga_mouse.h"

static struct MsgPort *port;
static struct IOStdReq *request;
static struct Interrupt handler;
static struct Window *native_window;
static volatile LONG dx, dy;
static volatile int captured;
static volatile int escape_pressed, escape_down;
static volatile int quit_pressed;
static int opened, installed;

/* input.device supplies the event chain in A0. The C handler uses the
   normal stack ABI and no OS calls, floating point or allocations. */
extern void Amiga_MouseEntry(void);
__asm__(".text\n.even\n.globl _Amiga_MouseEntry\n_Amiga_MouseEntry:\n"
        "movem.l %d1/%a0-%a1,-(%sp)\n"
        "move.l %a0,-(%sp)\n"
        "jsr _Amiga_MouseFilter\n"
        "addq.l #4,%sp\n"
        "movem.l (%sp)+,%d1/%a0-%a1\n"
        "rts\n");

struct InputEvent *Amiga_MouseFilter(struct InputEvent *events) {
    struct InputEvent *event;
    if (!native_window || !(native_window->Flags & WFLG_WINDOWACTIVE)) {
        escape_down = escape_pressed = 0;
        quit_pressed = 0;
        return events;
    }
    for (event=events; event; event=event->ie_NextEvent) {
        if (event->ie_Class == IECLASS_RAWKEY && event->ie_Code == 0x59 &&
            !(event->ie_Qualifier & IEQUALIFIER_REPEAT))
            quit_pressed = 1;
        /* Read Esc before SDL/Intuition, including while the menu releases
           mouse capture. Never call engine or OS functions in this handler. */
        if (event->ie_Class == IECLASS_RAWKEY &&
            (event->ie_Code & ~IECODE_UP_PREFIX) == 0x45) {
            if (event->ie_Code & IECODE_UP_PREFIX) escape_down = 0;
            else if (!escape_down && !(event->ie_Qualifier & IEQUALIFIER_REPEAT)) {
                escape_down = 1;
                if (escape_pressed < 8) ++escape_pressed;
            }
        }
        if (!captured) continue;
        if (event->ie_Class != IECLASS_RAWMOUSE ||
            !(event->ie_Qualifier & IEQUALIFIER_RELATIVEMOUSE) ||
            (event->ie_Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))) continue;
        dx += event->ie_X;
        dy += event->ie_Y;
        /* Keep buttons/qualifiers for Intuition and SDL, but do not move
           the desktop pointer. The game reads the original deltas above. */
        event->ie_X = event->ie_Y = 0;
    }
    return events;
}

int Amiga_ReadEscapePress(void) {
    int pressed;
    Disable();
    pressed = escape_pressed;
    escape_pressed = 0;
    Enable();
    return pressed;
}

int Amiga_ReadQuitPress(void) {
    int pressed;
    Disable();
    pressed = quit_pressed;
    quit_pressed = 0;
    Enable();
    return pressed;
}

static int write_event(struct InputEvent *event) {
    request->io_Command = IND_WRITEEVENT;
    request->io_Data = event;
    request->io_Length = sizeof(*event);
    return DoIO((struct IORequest *)request) == 0;
}

void Amiga_MouseShutdown(void) {
    captured = 0;
    quit_pressed = 0;
    escape_down = escape_pressed = 0;
    if (installed) {
        request->io_Command = IND_REMHANDLER;
        request->io_Data = &handler;
        DoIO((struct IORequest *)request);
        installed = 0;
    }
    if (opened) { CloseDevice((struct IORequest *)request); opened = 0; }
    if (request) { DeleteIORequest((struct IORequest *)request); request = NULL; }
    if (port) { DeleteMsgPort(port); port = NULL; }
    native_window = NULL;
}

int Amiga_MouseInit(void) {
    if (installed) return 1;
    native_window = (struct Window *)mglGetWindowHandle();
    port = CreateMsgPort();
    if (!port) goto fail;
    request = (struct IOStdReq *)CreateIORequest(port,sizeof(*request));
    if (!request || OpenDevice("input.device",0,(struct IORequest *)request,0)) goto fail;
    opened = 1;
    memset(&handler,0,sizeof(handler));
    handler.is_Node.ln_Type = NT_INTERRUPT;
    handler.is_Node.ln_Pri = 100; /* Before Intuition (priority 50). */
    handler.is_Node.ln_Name = "Doom64 relative mouse";
    handler.is_Code = Amiga_MouseEntry;
    request->io_Command = IND_ADDHANDLER;
    request->io_Data = &handler;
    if (DoIO((struct IORequest *)request)) goto fail;
    installed = 1;
    return 1;
fail:
    Amiga_MouseShutdown();
    return 0;
}

void Amiga_MouseGrab(int enable) {
    struct IEPointerPixel pixel;
    struct InputEvent event;
    if (!installed) return;
    if (!!enable == !!captured) return;
    captured = 0;
    if (enable && native_window && (native_window->Flags & WFLG_WINDOWACTIVE)) {
        /* SDL's warp is also a stub. Place the real pointer inside the
           client area so button events reach this window while captured. */
        memset(&event,0,sizeof(event));
        pixel.iepp_Screen = native_window->WScreen;
        pixel.iepp_Position.X = native_window->LeftEdge + native_window->Width/2;
        pixel.iepp_Position.Y = native_window->TopEdge + native_window->Height/2;
        event.ie_Class = IECLASS_NEWPOINTERPOS;
        event.ie_SubClass = IESUBCLASS_PIXEL;
        event.ie_Code = IECODE_NOBUTTON;
        event.ie_EventAddress = &pixel;
        write_event(&event);
        Disable();
        dx = dy = 0;
        captured = 1;
        Enable();
    } else {
        Disable(); dx = dy = 0; Enable();
    }
}

int Amiga_MouseRead(float *x, float *y) {
    LONG xval, yval;
    int active;
    Disable();
    xval = dx; yval = dy; dx = dy = 0;
    active = captured;
    Enable();
    *x = xval; *y = yval;
    return active;
}

/* On-target device test: the same motion is captured while grabbed and
   passes to Intuition after release. No physical mouse required. */
int Amiga_MouseTest(void) {
    struct InputEvent event;
    float x,y;
    int ok;
    Amiga_MouseGrab(1);
    memset(&event,0,sizeof(event));
    event.ie_Class = IECLASS_RAWMOUSE;
    event.ie_Code = IECODE_NOBUTTON;
    event.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
    event.ie_X = 23; event.ie_Y = -7;
    ok = captured && write_event(&event);
    ok = Amiga_MouseRead(&x,&y) && ok;
    printf("Mouse captured=%d result=%d delta=%ld,%ld event=%d,%d flags=%lx\n",captured,ok,(long)x,(long)y,event.ie_X,event.ie_Y,(unsigned long)native_window->Flags);
    ok = ok && x == 23 && y == -7 && event.ie_X == 0 && event.ie_Y == 0;
    Amiga_MouseGrab(0);
    event.ie_X = 11; event.ie_Y = 3;
    ok = write_event(&event) && ok;
    ok = !Amiga_MouseRead(&x,&y) && ok;
    printf("Mouse released=%d delta=%ld,%ld event=%d,%d\n",!captured,(long)x,(long)y,event.ie_X,event.ie_Y);
    /* Intuition may convert the released relative event to absolute screen
       coordinates in place; unlike captured input it must remain nonzero. */
    ok = ok && x == 0 && y == 0 && (event.ie_X != 0 || event.ie_Y != 0);
    printf("Native mouse capture/release: %s\n",ok?"PASS":"FAIL");
    return ok;
}

/* Exercise the actual Intuition/SDL key path in opt-in input tests. */
int Amiga_TestRawKey(unsigned short code) {
    struct InputEvent event;
    if(!opened) return 0;
    memset(&event,0,sizeof(event));
    event.ie_Class=IECLASS_RAWKEY;
    event.ie_Code=code;
    return write_event(&event);
}
