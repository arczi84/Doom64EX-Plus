/* Run on the host and on m68k: real SF2, RAM MIDI, pause, resume, restart. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "amiga_music.h"
static unsigned char *read_file(const char *name,int *length) {
    FILE *f=fopen(name,"rb"); unsigned char *p;
    if(!f) return NULL;
    fseek(f,0,SEEK_END);*length=ftell(f);rewind(f);
    p=malloc(*length);
    if(!p || fread(p,1,*length,f)!=(size_t)*length) { free(p);p=NULL; }
    fclose(f);return p;
}
int main(int argc,char **argv) {
    unsigned char *midi; int length,i,j,peak=0; unsigned long nonzero=0;
    struct timeval begin,end; unsigned long elapsed;
    short pcm[512],first[512],after[512];
    if(argc!=3) return 20;
    midi=read_file(argv[2],&length);
    if(!midi || !Amiga_MusicInit(argv[1]) || !Amiga_MusicStart(midi,length)) { puts("FAIL load");return 20; }
    /* Destroy input; parsed events and the bank must own all rendering data. */
    memset(midi,0,length);free(midi);
    Amiga_MusicRender(first,256);
    gettimeofday(&begin,NULL);
    for(i=0;i<22050*8/256;++i) {
        Amiga_MusicRender(pcm,256);
        for(j=0;j<512;++j) { int v=abs(pcm[j]); if(v) ++nonzero; if(v>peak) peak=v; }
    }
    gettimeofday(&end,NULL);
    elapsed=(end.tv_sec-begin.tv_sec)*1000+(end.tv_usec-begin.tv_usec)/1000;
    printf("Synthesis: 8 seconds audio in %lu ms\n",elapsed);
    Amiga_MusicPause(1);Amiga_MusicRender(pcm,256);
    for(j=0;j<512;++j) if(pcm[j]) { puts("FAIL pause");return 20; }
    Amiga_MusicPause(0);Amiga_MusicRender(after,256);
    Amiga_MusicStop();Amiga_MusicRender(pcm,256);
    for(j=0;j<512;++j) if(pcm[j]) { puts("FAIL stop");return 20; }
    midi=read_file(argv[2],&length);
    if(!midi || !Amiga_MusicStart(midi,length)) return 20;
    free(midi);Amiga_MusicRender(pcm,256);
    if(memcmp(first,pcm,sizeof(first))) { puts("FAIL restart");return 20; }
    Amiga_MusicVolume(0);Amiga_MusicRender(pcm,256);
    for(j=0;j<512;++j) if(pcm[j]) { puts("FAIL mute");return 20; }
    Amiga_MusicClose();
    printf("%s RAM MIDI: nonzero=%lu peak=%d; pause/stop/restart/mute\n",nonzero && peak?"PASS":"FAIL",nonzero,peak);
    return nonzero && peak?0:20;
}
