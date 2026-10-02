#ifndef AMIGA_MUSIC_H
#define AMIGA_MUSIC_H
/* Call from main task with the audio callback detached, or under audio lock. */
int Amiga_MusicInit(const char *soundfont);
int Amiga_MusicStart(const void *midi, int length);
void Amiga_MusicStop(void);
void Amiga_MusicClose(void);
void Amiga_MusicVolume(float volume);
void Amiga_MusicPause(int paused);
void Amiga_MusicRender(short *pcm, int frames);
#endif
