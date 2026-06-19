/* formant_speech.h - a tiny, portable, dependency-free formant speech synthesiser (single header).
 *
 * Source-filter model: a glottal impulse train (voiced) or LFSR noise (unvoiced) excites three
 * parallel 2-pole formant resonators tuned to F1/F2/F3 per phoneme. Renders 8-bit unsigned PCM at
 * FS_RATE Hz. No floating point, no allocation, no platform dependencies -- good for tiny MCUs.
 *
 * Single-header (stb-style): include this anywhere for the API; in EXACTLY ONE .c file do
 *     #define FORMANT_SPEECH_IMPLEMENTATION
 *     #include "formant_speech.h"
 * to compile the implementation + data.
 *
 * Licence: MIT. Copyright (c) 2026 Andrew Stuart.
 * Vowel formant frequencies are standard published values (Peterson & Barney, 1952).
 */
#ifndef FORMANT_SPEECH_H
#define FORMANT_SPEECH_H

#define FS_RATE 8000                 /* output sample rate (Hz) */

int  fs_word_count(void);            /* number of words in the built-in vocabulary */
int  fs_voice_count(void);           /* number of voice (pitch) variants */
const char *fs_word_text(int word);  /* display text for a word (e.g. "HELLO") */

/* Render one word in one voice to 8-bit unsigned PCM (silence = 128) at FS_RATE.
 * Writes up to maxlen samples into out; returns the number of samples written. */
int  fs_render(int word, int voice, unsigned char *out, int maxlen);

/* Streaming variant for memory-constrained targets: calls emit(ctx, sample) once per 8-bit sample,
 * in order, with no buffer. Feed each sample to your DAC/ring. */
typedef void (*fs_emit_fn)(void *ctx, unsigned char sample);
void fs_render_stream(int word, int voice, fs_emit_fn emit, void *ctx);

#endif /* FORMANT_SPEECH_H */

#ifdef FORMANT_SPEECH_IMPLEMENTATION
#include <stdint.h>

/* ---- generated data (gen_data.py): formant coefficients (Q14) + vocabulary ---- */
#define FS_SHIFT 8
typedef struct { int16_t a1[3], a2[3]; int16_t amp; uint8_t voiced; } fs_phon_t;
static const fs_phon_t fs_phon_[] = {
  {{0,0,0},{0,0,0},0,0}, /* SIL */
  {{31233,-7214,-22768},{-15575,-15575,-15575},1900,1}, /* IY */
  {{30385,2507,-13376},{-15575,-15575,-15575},1900,1}, /* IH */
  {{29221,4004,-11761},{-15575,-15575,-15575},1900,1}, /* EH */
  {{27752,7458,-9873},{-15575,-15575,-15575},1900,1}, /* AE */
  {{26840,20939,-10822},{-15575,-15575,-15575},1900,1}, /* AH */
  {{28800,25245,-10111},{-15575,-15575,-15575},1900,1}, /* AO */
  {{29974,25847,-9873},{-15575,-15575,-15575},1900,1}, /* OW */
  {{31066,24776,-5987},{-15575,-15575,-15575},1900,1}, /* UW */
  {{29612,15611,7702},{-15575,-15575,-15575},1900,1}, /* ER */
  {{28467,7458,-12226},{-15575,-15575,-15575},1900,1}, /* AY */
  {{30680,16693,-12226},{-15575,-15575,-15575},1700,1}, /* L */
  {{29612,15611,7702},{-15575,-15575,-15575},1700,1}, /* R */
  {{31066,28352,-4998},{-15575,-15575,-15575},1600,1}, /* W */
  {{31233,-4998,-22591},{-15575,-15575,-15575},1600,1}, /* Y */
  {{31335,20749,-4998},{-15575,-15575,-15575},1500,1}, /* M */
  {{31335,7458,-9873},{-15575,-15575,-15575},1500,1}, /* N */
  {{31066,7458,-14504},{-15575,-15575,-15575},1300,1}, /* D */
  {{31066,20749,-4998},{-15575,-15575,-15575},1300,1}, /* G */
  {{30749,20749,-9873},{-15575,-15575,-15575},900,1}, /* V */
  {{30882,7458,-14504},{-15575,-15575,-15575},900,1}, /* Z */
  {{29517,12226,-9873},{-15575,-15575,-15575},700,0}, /* HH */
  {{30385,20749,-9873},{-15575,-15575,-15575},750,0}, /* F */
  {{30385,7458,-14504},{-15575,-15575,-15575},700,0}, /* TH */
  {{30385,7458,-22591},{-15575,-15575,-15575},800,0}, /* S */
  {{30385,7458,-14504},{-15575,-15575,-15575},800,0}, /* T */
  {{30749,20749,-4998},{-15575,-15575,-15575},800,0}, /* K */
};
static const char *const fs_text_[] = {"HELLO","WORLD","ZERO","ONE","TWO","THREE","FOUR","FIVE","READY","GO","SAM","VIDEO"};
static const uint8_t  fs_seq_[] = {21,88,3,184,11,112,7,272,13,112,9,208,11,112,17,144,20,128,2,128,12,112,7,272,13,112,5,208,16,160,25,72,8,336,23,144,12,96,1,304,22,144,6,240,12,160,22,144,10,304,19,144,12,128,3,176,17,112,1,256,18,96,7,336,24,144,4,240,15,192,19,128,2,128,17,96,1,192,7,240};
static const uint16_t fs_off_[] = {0,8,16,24,30,34,40,46,52,60,64,70};
static const uint8_t  fs_len_[] = {4,4,4,3,2,3,3,3,4,2,3,5};
#define FS_WORDS_ 12
static const uint8_t fs_pitch_[] = {67,53};  /* voice variants ~120 Hz, ~150 Hz */
#define FS_VOICES_ 2

int fs_word_count(void){ return FS_WORDS_; }
int fs_voice_count(void){ return FS_VOICES_; }
const char *fs_word_text(int w){ return (w >= 0 && w < FS_WORDS_) ? fs_text_[w] : ""; }

/* core synth: emits 8-bit unsigned PCM, one sample at a time, via emit(ctx, s) */
static void fs_run_(int word, int voice, fs_emit_fn emit, void *ctx){
    int pitch = fs_pitch_[voice];
    long y1[3] = {0,0,0}, y2[3] = {0,0,0};
    int  pc = 0; unsigned lf = 0xACE1u;
    const unsigned char *seq = &fs_seq_[fs_off_[word]];
    int npairs = fs_len_[word];
    for(int i = 0; i < npairs; i++){
        const fs_phon_t *p = &fs_phon_[seq[2*i]];
        int ns = (int)seq[2*i+1] * FS_RATE / 1000;
        for(int n = 0; n < ns; n++){
            int src;
            if(p->voiced){
                if(pc <= 0){ src = p->amp; pc = pitch; } else src = 0;
                pc--;
            } else {
                lf = ((lf >> 1) ^ (-(int)(lf & 1u) & 0xB400u)) & 0xFFFFu;
                src = (((int)(lf & 0xFF) - 128) * p->amp) >> 7;
            }
            long o = 0;
            for(int k = 0; k < 3; k++){
                long y = ((p->a1[k]*y1[k] + p->a2[k]*y2[k]) >> 14) + src;
                if(y > 100000) y = 100000; else if(y < -100000) y = -100000;
                y2[k] = y1[k]; y1[k] = y; o += y;
            }
            o >>= FS_SHIFT;
            if(o > 127) o = 127; else if(o < -127) o = -127;
            emit(ctx, (unsigned char)(o + 128));
        }
    }
}

void fs_render_stream(int word, int voice, fs_emit_fn emit, void *ctx){
    if(word < 0 || word >= FS_WORDS_ || !emit) return;
    if(voice < 0 || voice >= FS_VOICES_) voice = 0;
    fs_run_(word, voice, emit, ctx);
}

struct fs_buf_ { unsigned char *out; int max, n; };
static void fs_buf_emit_(void *ctx, unsigned char s){
    struct fs_buf_ *b = (struct fs_buf_ *)ctx;
    if(b->n < b->max) b->out[b->n++] = s;
}
int fs_render(int word, int voice, unsigned char *out, int maxlen){
    if(word < 0 || word >= FS_WORDS_ || !out || maxlen <= 0) return 0;
    if(voice < 0 || voice >= FS_VOICES_) voice = 0;
    struct fs_buf_ b; b.out = out; b.max = maxlen; b.n = 0;
    fs_run_(word, voice, fs_buf_emit_, &b);
    return b.n;
}
#endif /* FORMANT_SPEECH_IMPLEMENTATION */
