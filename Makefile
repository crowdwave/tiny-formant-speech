# Tiny formant speech synthesiser (single-header)
example: example_wav.c formant_speech.h
	cc -O2 -Wall -o example example_wav.c

# regenerate the embedded data block (paste between the markers in formant_speech.h, or keep for ref)
data:
	python3 gen_data.py

run: example
	./example

clean:
	rm -f example speech.wav
.PHONY: data run clean
