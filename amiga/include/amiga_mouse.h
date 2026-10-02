#ifndef DOOM_AMIGA_MOUSE_H
#define DOOM_AMIGA_MOUSE_H
int Amiga_MouseInit(void);
void Amiga_MouseShutdown(void);
void Amiga_MouseGrab(int enable);
int Amiga_MouseRead(float *x, float *y);
int Amiga_ReadEscapePress(void);
int Amiga_ReadQuitPress(void);
int Amiga_MouseTest(void);
int Amiga_TestRawKey(unsigned short code);
#endif
