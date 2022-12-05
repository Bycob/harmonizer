#ifndef HARMONIZER_DSP_H
#define HARMONIZER_DSP_H

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fftw3.h>

#include "pitch_detection.h"

// TODO independent of jack if needed
#include <jack/jack.h>
typedef jack_default_audio_sample_t sample_t;
typedef jack_nframes_t count_t;

#define MAX_HARMONIZER_VOICES 8
#define HARMONIZER_CHANNELS 2

typedef struct {
    bool active;
    float target_period;

    float prev_ratio[2];
    float prev_offset[2];
} harmonizer_voice_t;

/** Harmonizer "Digital Signal Processor" */
typedef struct {
    harmonizer_voice_t voices[MAX_HARMONIZER_VOICES];

    sample_t prev_period[HARMONIZER_CHANNELS];
    /** rolling buffer of previous data */
    // XXX currently used as single buffer
    sample_t *prev_buf[HARMONIZER_CHANNELS];
    /** id of next write in rolling buffer */
    // int prev_idx[HARMONIZER_CHANNELS];

    /** preallocated buffers to compute the fft */
    fftwf_complex *fft[HARMONIZER_CHANNELS];
    pitch_detection_data pitch_detect[HARMONIZER_CHANNELS];
} harmonizer_dsp_t;

void harmonizer_dsp_init(harmonizer_dsp_t *dsp);

/**
 * Main fonction for harmonizer processing. Takes a number of frames, input
 * samples * number of channels, output samples * number of channels.
 * */
int harmonizer_dsp_process(harmonizer_dsp_t *dsp, count_t nframes,
                           sample_t **in_stereo, sample_t **out_stereo);

void harmonizer_dsp_destroy(harmonizer_dsp_t *dsp);

#endif // HARMONIZER_DSP_H
