#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <endian.h>

#include "../w4on.c"
#include "wasm4/runtimes/native/src/apu.c"

#define SAMPLES_PER_TICK (SAMPLE_RATE / 60)
#define TICK_SAMPLE_CHUNK_SZ (SAMPLES_PER_TICK * CHANNELS * BYTES_PER_SAMPLE)
#define BYTES_PER_SAMPLE 2
#define CHANNELS 2

void tone(uint32_t frequency, uint32_t duration, uint32_t volume, uint32_t flags)
{
	w4_apuTone(frequency, duration, volume, flags);
}

void fwrite_i16(FILE *f, int16_t n)
{
	const int16_t le = htole16(n);
	assert(fwrite(&le, 2, 1, f));
}

void fwrite_i32(FILE *f, int32_t n)
{
	const int32_t le = htole32(n);
	assert(fwrite(&le, 4, 1, f));
}

void usage()
{
	fprintf(stderr, "w4on-wav [-l<loops>] <in.w4on> <out.wav>\n");
	exit(1);
}

int main(int argc, char **argv)
{
	size_t argi = 1;
	size_t loops = 0;
	while (argi < argc && argv[argi][0] == '-') {
		if (argv[argi][1] == 'l') {
			assert(sscanf(argv[argi] + 2, "%ld", &loops) == 1);
			assert(loops >= 1);
		} else {
			usage();
		}

		argi++;
	}
	if (argc - argi != 2) {
		usage();
	}

	loops++; // always plays at least once

	// Read w4on file
	FILE *w4onFile = fopen(argv[argi++], "rb");
	assert(w4onFile);
	fseek(w4onFile, 0, SEEK_END);
	const size_t w4onSz = ftell(w4onFile);
	fseek(w4onFile, 0, SEEK_SET);
	uint8_t *w4onData = malloc(w4onSz);
	assert(w4onData);
	assert(fread(w4onData, w4onSz, 1, w4onFile));
	fclose(w4onFile);

	// Init WASM-4 APU
	w4_apuInit();

	// Sample buffer
	size_t bufCap = 1 << 16;
	size_t bufSz = 0; // will be Subchunk2Size
	uint8_t *buf = malloc(bufCap);
	assert(buf);

	// Loop
	for (size_t loopI = 0; loopI < loops; loopI++) {
		w4on_seq_t seq;
		w4on_seq_init(&seq, w4onData);

		// Tick
		while (w4on_seq_tick(&seq)) {
			// Get new samples
			int16_t samples[SAMPLES_PER_TICK * CHANNELS];
			w4_apuWriteSamples(samples, SAMPLES_PER_TICK);

			// Swap to LE order
			for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
				samples[i] = htole16(samples[i]);
			}

			// Write
			if (bufSz + TICK_SAMPLE_CHUNK_SZ > bufCap) {
				bufCap <<= 1;
				buf = realloc(buf, bufCap);
				assert(buf);
			}
			memcpy(buf + bufSz, samples, TICK_SAMPLE_CHUNK_SZ);
			bufSz += TICK_SAMPLE_CHUNK_SZ;
		}
	}

	// Write wav file
	// http://soundfile.sapp.org/doc/WaveFormat/
	FILE *wavFile = fopen(argv[argi++], "wb");
	assert(wavFile);

	// - RIFF Header
	assert(fwrite("RIFF", 4, 1, wavFile));
	fwrite_i32(wavFile, 36 + bufSz);
	assert(fwrite("WAVE", 4, 1, wavFile));

	// - fmt chunk
	assert(fwrite("fmt ", 4, 1, wavFile));
	fwrite_i32(wavFile, 16); // Size
	fwrite_i16(wavFile, 1); // Format
	fwrite_i16(wavFile, 2); // Channels
	fwrite_i32(wavFile, SAMPLE_RATE);
	fwrite_i32(wavFile, SAMPLE_RATE * CHANNELS * BYTES_PER_SAMPLE); // Byte rate
	fwrite_i16(wavFile, CHANNELS * BYTES_PER_SAMPLE); // Block align
	fwrite_i16(wavFile, BYTES_PER_SAMPLE * 8); // Bits per sample

	// - data chunk
	assert(fwrite("data", 4, 1, wavFile));
	fwrite_i32(wavFile, bufSz);
	assert(fwrite(buf, bufSz, 1, wavFile));

	fclose(wavFile);

	return 0;
}
