/* SDL_mixer/AHI backend. Legacy FMOD_* names are engine entry points only. */
#include <SDL/SDL.h>
#include "SDL_mixer.h"
#include <proto/exec.h>
#include "doomdef.h"
#include "i_audio.h"
#include "i_system.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"
#include "con_console.h"
#include "amiga_music.h"

#define CHANNELS 32
static Mix_Chunk *samples[MAX_GAME_SFX];
static sndsrc_t *sources[CHANNELS];
static int sound_ids[CHANNELS];
static int audio_requested, pending_music = -1;
static int ready, count, loop_channel = -1, plasma_channel = -1;
static float sound_volume = 100;
static int music_initialized, profile_audio;
static unsigned long profile_calls, profile_ms, profile_max, profile_overruns;
static Uint32 profile_last_mix, profile_max_gap, profile_mix_start;
static unsigned long profile_total_ms, profile_total_max;
static int profile_task_priority;
static void profile_postmix(void *unused, Uint8 *stream, int length) {
    Uint32 now=SDL_GetTicks();
    (void)unused; (void)stream; (void)length;
    if(profile_last_mix && now-profile_last_mix>profile_max_gap)
        profile_max_gap=now-profile_last_mix;
    if(profile_mix_start) {
        Uint32 cost=now-profile_mix_start;
        profile_total_ms+=cost;
        if(cost>profile_total_max) profile_total_max=cost;
    }
    profile_last_mix=now;
    profile_task_priority=FindTask(NULL)->tc_Node.ln_Pri;
}
static void music_callback(void *unused, Uint8 *stream, int length) {
    Uint32 start=profile_audio?SDL_GetTicks():0;
    profile_mix_start=start;
    (void)unused; Amiga_MusicRender((short *)stream,length/4);
    if(profile_audio) {
        Uint32 elapsed=SDL_GetTicks()-start;
        ++profile_calls; profile_ms+=elapsed;
        if(elapsed>profile_max) profile_max=elapsed;
        if(elapsed>(Uint32)(length*1000/4/22050)) ++profile_overruns;
    }
}
CVAR_EXTERNAL(s_sfxvol);
CVAR_EXTERNAL(s_musvol);

static int clamp(int n, int max) { return n < 0 ? 0 : n > max ? max : n; }
void I_UpdateChannel(int c, int volume, int pan, fixed_t x, fixed_t y) {
    if (!ready || c < 0 || c >= CHANNELS) return;
    /* Engine attenuation is 0..127; menu volume is a percentage. */
    Mix_Volume(c, clamp((int)(volume * sound_volume / 100.0f), MIX_MAX_VOLUME));
    pan = clamp(pan, 255);
    Mix_SetPanning(c, 255-pan, pan);
}
void I_InitSequencer(void) { audio_requested = !M_CheckParm("-nosound"); }

/* Like OpenLara, create the AHI task only after MiniGL presents its first frame. */
void Amiga_StartAudio(void) {
    int i, start, end;
    if (!audio_requested) return;
    audio_requested = 0;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0 || Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 2048) < 0) {
        I_Printf("Audio unavailable: %s\n", SDL_GetError());
        return;
    }
    /* SDL/AHI assigns its audio task priority 11. Do not lower it: mixing
       must preempt rendering to meet the hardware buffer deadline. */
    ready = 1;
    profile_audio=M_CheckParm("-amiga-audiostats")!=0;
    if(profile_audio) Mix_SetPostMix(profile_postmix,NULL);
    Mix_AllocateChannels(CHANNELS);
    start = W_GetNumForName("DS_START") + 1;
    end = W_GetNumForName("DS_END");
    count = end - start;
    if (count > MAX_GAME_SFX) count = MAX_GAME_SFX;
    for (i = 0; i < count; ++i) {
        void *data = W_CacheLumpNum(start+i, PU_STATIC);
        SDL_RWops *rw = SDL_RWFromMem(data, W_LumpLength(start+i));
        /* Remaster slot 0 (NOSOUND) is an empty WAV, not a decode error. */
        if (i == 0) { if (rw) SDL_FreeRW(rw); Z_Free(data); continue; }
        samples[i] = rw ? Mix_LoadWAV_RW(rw, 1) : NULL;
        if (!samples[i]) I_Printf("SFX %d: %s\n", i, SDL_GetError());
        Z_Free(data);
    }
    I_SetSoundVolume(s_sfxvol.value);
    I_SetMusicVolume(s_musvol.value);
    I_Printf("SDL_mixer/AHI: %d effects, 22050 Hz stereo\n", count);
    I_Printf("Audio: percent-volume v1, SFX %.0f/100, music %.0f/100\n",
        sound_volume, s_musvol.value);
    if (pending_music >= 0) FMOD_StartMusic(pending_music);
    /* Opt-in worst-case test, independent of map ambience/player input. */
    if(M_CheckParm("-amiga-audiostress")) {
        int channel=0;
        for(i=1;i<count && channel<16;++i) if(samples[i]) {
            Mix_PlayChannel(channel,samples[i],-1);
            I_UpdateChannel(channel,192,channel*17,0,0);
            ++channel;
        }
        I_Printf("Audio stress: %d looping SFX with stereo panning\n",channel);
    }
}
int I_GetMaxChannels(void) { return ready ? CHANNELS : 0; }
int I_GetVoiceCount(void) { return ready ? Mix_Playing(-1) : 0; }
sndsrc_t *I_GetSoundSource(int c) {
    return ready && c >= 0 && c < CHANNELS && Mix_Playing(c) ? sources[c] : NULL;
}
void I_RemoveSoundSource(int c) {
    if (ready && c >= 0 && c < CHANNELS) { Mix_HaltChannel(c); sources[c] = NULL; }
}
static int play(int id, sndsrc_t *origin, int vol, int pan, int loops) {
    int c;
    if (!ready || id < 0 || id >= count || !samples[id]) return -1;
    c = Mix_PlayChannel(-1, samples[id], loops);
    if (c >= 0) { sources[c] = origin; sound_ids[c] = id; I_UpdateChannel(c, vol, pan, 0, 0); }
    return c;
}
int FMOD_StartSound(int id, sndsrc_t *origin, int vol, int pan) { return play(id, origin, vol, pan, 0); }
void FMOD_StopSound(sndsrc_t *origin, int id) {
    int c;
    for (c=0;c<I_GetMaxChannels();++c) if (sources[c]==origin && sound_ids[c]==id) I_RemoveSoundSource(c);
}
int FMOD_StopSFXLoop(void) { I_RemoveSoundSource(loop_channel); loop_channel=-1; return 0; }
void FMOD_StopPlasmaLoop(void) { I_RemoveSoundSource(plasma_channel); plasma_channel=-1; }
int FMOD_StartSFXLoop(int id, int vol) { FMOD_StopSFXLoop(); return loop_channel=play(id,NULL,vol,128,-1); }
int FMOD_StartPlasmaLoop(int id, int vol) { FMOD_StopPlasmaLoop(); return plasma_channel=play(id,NULL,vol,128,-1); }
int FMOD_StartSoundPlasma(int id) { return play(id,NULL,255,128,0); }
void FMOD_StopMusic(sndsrc_t *origin, int id) {
    if (!ready) return;
    Mix_HookMusic(NULL,NULL);
    Amiga_MusicStop();
}
int FMOD_StartMusic(int id) {
    char *path;
    void *data;
    int start, end, ok;
    pending_music = id;
    if (!ready || M_CheckParm("-nomusic")) return -1;
    FMOD_StopMusic(NULL,0);
    if (!music_initialized) {
        path=I_FindDataFile("doomsnd.sf2");
        if (!path) { I_Printf("Music: doomsnd.sf2 missing\n"); return -1; }
        I_Printf("Music: loading SF2 bank %s\n", path);
        music_initialized=Amiga_MusicInit(path);
        free(path);
        if (!music_initialized) { I_Printf("Music: cannot load SF2 bank\n"); return -1; }
        I_Printf("Music: SF2 bank in RAM, synth 22050 Hz stereo / 32 voices\n");
    }
    start=W_GetNumForName("DM_START")+1;
    end=W_GetNumForName("DM_END");
    if(id<0 || id>=end-start) return -1;
    data=W_CacheLumpNum(start+id,PU_STATIC);
    ok=Amiga_MusicStart(data,W_LumpLength(start+id));
    Z_Free(data);
    if(!ok) { I_Printf("Music: invalid MIDI %.8s\n",lumpinfo[start+id].name); return -1; }
    Amiga_MusicVolume(s_musvol.value/100.0f);
    Mix_HookMusic(music_callback,NULL);
    I_Printf("Music: %.8s MIDI from RAM\n",lumpinfo[start+id].name);
    return id;
}
void I_SetMusicVolume(float v) {
    if(ready) { SDL_LockAudio(); Amiga_MusicVolume(v/100.0f); SDL_UnlockAudio(); }
}
void I_SetSoundVolume(float v) { sound_volume=clamp((int)v,100); }
void Chan_SetMusicVolume(float v) { I_SetMusicVolume(v); }
void Chan_SetSoundVolume(float v) { I_SetSoundVolume(v); }
void FMOD_PauseMusic(void) { if (ready) { SDL_LockAudio(); Amiga_MusicPause(1); SDL_UnlockAudio(); } }
void FMOD_ResumeMusic(void) { if (ready) { SDL_LockAudio(); Amiga_MusicPause(0); SDL_UnlockAudio(); } }
void FMOD_PauseSFXLoop(void) { if (ready && loop_channel>=0) Mix_Pause(loop_channel); }
void FMOD_ResumeSFXLoop(void) { if (ready && loop_channel>=0) Mix_Resume(loop_channel); }
void I_PauseSound(void) { if (ready) { Mix_Pause(-1); FMOD_PauseMusic(); } }
void I_ResumeSound(void) { if (ready) { Mix_Resume(-1); FMOD_ResumeMusic(); } }
void I_ResetSound(void) { if (ready) { Mix_HaltChannel(-1); memset(sources,0,sizeof(sources)); loop_channel=plasma_channel=-1; } }
void I_ShutdownSound(void) {
    int i;
    if (!ready) return;
    I_Printf("Audio shutdown: halt channels\n");
    I_ResetSound(); FMOD_StopMusic(NULL,0);
    Amiga_MusicClose(); music_initialized=0;
    if(profile_audio && profile_calls) I_Printf("Music CPU: %lu callbacks, avg %lu ms, max %lu ms, synthesis over budget %lu\n",
        profile_calls,profile_ms/profile_calls,profile_max,profile_overruns);
    if(profile_audio && profile_calls) I_Printf("Full mixer CPU: avg %lu ms, max %lu ms\n",
        profile_total_ms/profile_calls,profile_total_max);
    if(profile_audio) I_Printf("Audio scheduling: task priority %d, max buffer interval %lu ms\n",
        profile_task_priority,(unsigned long)profile_max_gap);
    Mix_SetPostMix(NULL,NULL);
    I_Printf("Audio shutdown: free samples\n");
    for(i=0;i<count;++i) { if (samples[i]) Mix_FreeChunk(samples[i]); samples[i]=NULL; }
    I_Printf("Audio shutdown: close device\n");
    Mix_CloseAudio(); ready=0;
    I_Printf("Audio shutdown: done\n");
}
/* Reverb/low-pass gain is not provided by the SDL mixer. */
void I_SetGain(float v) { (void)v; }
void Seq_SetGain(float v) { (void)v; }
void I_UpdateListenerPosition(fixed_t x,fixed_t y,fixed_t z,angle_t a) { /* S_UpdateSounds handles attenuation/pan. */ }
void I_StopSound(sndsrc_t *s,int id) { FMOD_StopSound(s,id); }
void I_StartMusic(int id) { FMOD_StartMusic(id); }
