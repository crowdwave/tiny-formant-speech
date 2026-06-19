/* example_wav.c - render the whole vocabulary (every word in every voice) to speech.wav.
 * Build: cc -O2 -o example example_wav.c && ./example && afplay speech.wav  (or aplay/play) */
#include <stdio.h>
#define FORMANT_SPEECH_IMPLEMENTATION
#include "formant_speech.h"

static void put32(FILE*f,unsigned v){fputc(v,f);fputc(v>>8,f);fputc(v>>16,f);fputc(v>>24,f);}
static void put16(FILE*f,unsigned v){fputc(v,f);fputc(v>>8,f);}

int main(void){
    static unsigned char pcm[FS_RATE*2];
    static unsigned char all[FS_RATE*40];
    int len = 0;
    for(int v = 0; v < fs_voice_count(); v++)
        for(int w = 0; w < fs_word_count(); w++){
            int n = fs_render(w, v, pcm, sizeof pcm);
            for(int i = 0; i < n && len < (int)sizeof all; i++) all[len++] = pcm[i];
            for(int i = 0; i < FS_RATE*2/5 && len < (int)sizeof all; i++) all[len++] = 128; /* 0.4s gap */
            printf("voice %d: %s\n", v+1, fs_word_text(w));
        }
    FILE *f = fopen("speech.wav","wb");
    fwrite("RIFF",1,4,f); put32(f,36+len); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); put32(f,16); put16(f,1); put16(f,1);
    put32(f,FS_RATE); put32(f,FS_RATE); put16(f,1); put16(f,8);
    fwrite("data",1,4,f); put32(f,len); fwrite(all,1,len,f); fclose(f);
    printf("wrote speech.wav (%d samples, %.1fs)\n", len, (double)len/FS_RATE);
    return 0;
}
