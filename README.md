# tiny-formant-speech

A tiny, portable, dependency-free **formant speech synthesiser** in C — as a **single header**.

It speaks a small word vocabulary using the classic *source-filter* technique: a glottal impulse
train (voiced) or pseudo-random noise (unvoiced) excites three parallel 2-pole **formant resonators**
tuned to F1/F2/F3 for each phoneme. Output is 8-bit unsigned PCM — robotic, but intelligible, in the
spirit of early speech chips.

- **One file**, no floating point, no allocation, no platform dependencies. Runs on tiny
  microcontrollers (written for a 48 MHz Cortex-M0+ driving a DAC).
- **Voice variants** via pitch (F0). ~8 kHz output.

## Use it

Include the header anywhere for the API; in **exactly one** `.c` file, define the implementation:

```c
#define FORMANT_SPEECH_IMPLEMENTATION
#include "formant_speech.h"
```

```c
int  fs_word_count(void);                 // words in the vocabulary
int  fs_voice_count(void);                // pitch/voice variants
const char *fs_word_text(int word);       // e.g. "HELLO"

// Render one word in one voice to 8-bit unsigned PCM @ FS_RATE (silence = 128).
int  fs_render(int word, int voice, unsigned char *out, int maxlen);

// Streaming variant for tiny RAM: calls emit(ctx, sample) once per sample, no buffer.
typedef void (*fs_emit_fn)(void *ctx, unsigned char sample);
void fs_render_stream(int word, int voice, fs_emit_fn emit, void *ctx);
```

On an MCU, feed the bytes to a DAC at `FS_RATE`. With only ~1 KB to spare, use `fs_render_stream`:
it emits one sample at a time through your callback (push each into a DAC ring/ISR), so no PCM buffer
is needed at all. `fs_render` (buffer) is the convenience form for hosts / when RAM is plentiful.

## Try it (host)

```
make run            # builds the example, renders the whole vocabulary to speech.wav
afplay speech.wav   # macOS  (or: aplay / play speech.wav)
```

## Vocabulary

HELLO, WORLD, ZERO, ONE, TWO, THREE, FOUR, FIVE, READY, GO, SAM, VIDEO — in two voices.

Edit words/phonemes in `gen_data.py`, run `make data`, and paste the regenerated block into the
implementation section of `formant_speech.h`.

## How it works

- **Source.** Voiced phonemes: an impulse once per pitch period (F0). Unvoiced: a 16-bit Galois LFSR.
- **Filter.** Three parallel 2-pole resonators `y[n] = (a1*y1 + a2*y2)>>14 + x[n]`, with `a1`/`a2`
  precomputed (Q14) from each phoneme's formant frequencies and a fixed pole radius (bandwidth).
- **Phonemes.** A small table of F1/F2/F3 per phoneme; words are `(phoneme, duration)` sequences.

Vowel formant frequencies are standard published acoustic-phonetics values (Peterson & Barney,
*Control Methods Used in a Study of the Vowels*, 1952). The synthesiser itself is original.

## Licence

MIT. See [LICENSE](LICENSE).
