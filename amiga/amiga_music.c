/* In-memory SF2/MIDI music. Rendering performs no file I/O or allocation. */
#include <stdint.h>
#include <string.h>
#define TSF_IMPLEMENTATION
#include "vendor/TinySoundFont/tsf.h"
#define TML_IMPLEMENTATION
#define TML_NO_STDIO
#include "vendor/TinySoundFont/tml.h"
#include "amiga_music.h"
#define MUSIC_RATE 22050
static tsf *bank;
static tml_message *song, *next;
static uint64_t position, duration;
static int paused;
static float volume = 0.5f;
static void reset_channels(void) {
    int i;
    for (i=0;i<bank->voiceNum;++i) bank->voices[i].playingPreset=-1;
    for (i=0;i<16;++i) {
        tsf_channel_sounds_off_all(bank,i);
        tsf_channel_midi_control(bank,i,121,0);
        tsf_channel_set_sustain(bank,i,0);
        tsf_channel_set_pitchwheel(bank,i,8192);
        tsf_channel_set_presetnumber(bank,i,0,i==9);
    }
}
int Amiga_MusicInit(const char *path) {
    int i;
    if (bank) return 1;
    bank=tsf_load_filename(path);
    if (!bank) return 0;
    tsf_set_output(bank,TSF_STEREO_INTERLEAVED,MUSIC_RATE,0);
    if (!tsf_set_max_voices(bank,32)) { Amiga_MusicClose(); return 0; }
    /* Allocate all channels now, never from AHI's rendering task. */
    for(i=0;i<16;++i) if (!tsf_channel_set_presetnumber(bank,i,0,i==9)) {
        Amiga_MusicClose(); return 0;
    }
    Amiga_MusicVolume(volume);
    return 1;
}
void Amiga_MusicStop(void) {
    if(bank) reset_channels();
    if(song) tml_free(song);
    song=next=NULL; position=duration=0;
}
void Amiga_MusicClose(void) {
    Amiga_MusicStop();
    if(bank) tsf_close(bank);
    bank=NULL;
}
int Amiga_MusicStart(const void *midi,int length) {
    tml_message *m;
    Amiga_MusicStop();
    if(!bank || length<14) return 0;
    song=tml_load_memory(midi,length);
    if(!song) return 0;
    for(m=song;m;m=m->next) duration=m->time;
    /* Whole-track looping, with two seconds for release tails. */
    duration=(duration+2000)*MUSIC_RATE/1000;
    next=song; paused=0;
    return 1;
}
void Amiga_MusicVolume(float v) {
    volume=v<0?0:v>1?1:v;
    if(bank) {
        /* Avoid upstream 1/volume at zero on the 68k FPU. */
        if(volume==0) bank->globalGainDB=-100.0f;
        else tsf_set_volume(bank,volume);
    }
}
void Amiga_MusicPause(int p) { paused=p; }
static void event(const tml_message *m) {
    switch(m->type) {
    case TML_NOTE_ON: tsf_channel_note_on(bank,m->channel,m->key,m->velocity/127.0f);break;
    case TML_NOTE_OFF: tsf_channel_note_off(bank,m->channel,m->key);break;
    case TML_PROGRAM_CHANGE: tsf_channel_set_presetnumber(bank,m->channel,(unsigned char)m->program,m->channel==9);break;
    case TML_CONTROL_CHANGE: tsf_channel_midi_control(bank,m->channel,m->control,m->control_value);break;
    case TML_PITCH_BEND: tsf_channel_set_pitchwheel(bank,m->channel,m->pitch_bend);break;
    }
}
void Amiga_MusicRender(short *pcm,int frames) {
    memset(pcm,0,frames*2*sizeof(*pcm));
    if(!bank || !song || paused) return;
    while(frames>0) {
        uint64_t boundary;
        int block;
        if(position>=duration) { reset_channels(); position=0; next=song; }
        while(next && (uint64_t)next->time*MUSIC_RATE/1000<=position) {
            event(next); next=next->next;
        }
        boundary=next?(uint64_t)next->time*MUSIC_RATE/1000:duration;
        block=frames;
        if((uint64_t)block>boundary-position) block=(int)(boundary-position);
        if(block>64) block=64;
        tsf_render_short(bank,pcm,block,0);
        position+=block; pcm+=block*2; frames-=block;
    }
}
