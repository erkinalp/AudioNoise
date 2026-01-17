#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SAMPLES_PER_SEC (48000.0)

// Core utility functions and helpers
#include "util.h"
#include "lfo.h"
#include "effect.h"
#include "biquad.h"
#include "process.h"

// Effects
#include "flanger.h"
#include "echo.h"
#include "fm.h"
#include "am.h"
#include "phaser.h"
#include "discont.h"
#include "distortion.h"

static void magnitude_describe(float pot[4]) { fprintf(stderr, "\n"); }
static void magnitude_init(float pot[4]) {}
static float magnitude_step(float in) { return u32_to_fraction(magnitude); }

#define EFF(x) { #x, x##_describe, x##_init, x##_step }
struct effect {
	const char *name;
	void (*describe)(float[4]);
	void (*init)(float[4]);
	float (*step)(float);
} effects[] = {
	EFF(discont),
	EFF(distortion),
	EFF(echo),
	EFF(flanger),
	EFF(phaser),

	/* "Helper" effects */
	EFF(am),
	EFF(fm),
	EFF(magnitude),
};

#define UPDATE(x) x += 0.001 * (target_##x - x)

#define BLOCKSIZE 200
static inline int make_one_noise(int in, int out, struct effect *eff)
{
	s32 input[BLOCKSIZE], output[BLOCKSIZE];
	int nr = read(in, input, sizeof(input));
	if (nr <= 0)
		return nr;

	nr /= 4;
	for (int i = 0; i < nr; i++) {
		UPDATE(effect_delay);

		float val = process_input(input[i]);

		val = eff->step(val);

		output[i] = process_output(val);
	}
	write(out, output, nr * 4);
	return nr * 4;
}

int main(int argc, char **argv)
{
	float pot[4];
	struct effect *eff = &effects[0];

	if (argc < 6)
		return 1;

	const char *name = argv[1];

	for (int i = 0; i < ARRAY_SIZE(effects); i++) {
		if (!strcmp(name, effects[i].name))
			eff = effects+i;
	}

	for (int i = 0; i < 4; i++)
		pot[i] = atof(argv[2+i]);

	fprintf(stderr, "Playing %s: ",	eff->name);
	eff->describe(pot);

	for (;;) {
		eff->init(pot);
		if (make_one_noise(0, 1, eff) <= 0)
			break;
	}
	return 0;
}
