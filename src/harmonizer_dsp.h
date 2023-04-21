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

#ifdef __cplusplus
extern "C" {
#endif

// Rolling buffer

typedef struct {
    /** rolling buffer of past data */
    sample_t *buf;
    /** offset of last write in rolling buffer */
    int curr_offset;

    int size;
    int step;
} rolling_buffer_t;

void rbuf_init(rolling_buffer_t *rbuf, int size, int step);

/** Get current buffer. Use prev = 1, 2, 3 to get a previous buffer. */
sample_t *rbuf_get(rolling_buffer_t *rbuf, int prev);

/** Advance to next buffer and return it. */
sample_t *rbuf_next(rolling_buffer_t *rbuf);

void rbuf_destroy(rolling_buffer_t *rbuf);

// Midi

typedef struct {
    double stamp;
    bool note_on;
    int pitch;
    int velocity;
} harmonizer_midi_event_t;

// Harmonizer

#define MAX_HARMONIZER_VOICES 8
#define HARMONIZER_CHANNELS 2

typedef struct {
    bool active;
    int midi_pitch;
    float target_period;

    float prev_ratio[HARMONIZER_CHANNELS];
    float prev_offset[HARMONIZER_CHANNELS];
    sample_t prev_period[HARMONIZER_CHANNELS];

    sample_t *starting_buf[HARMONIZER_CHANNELS];
    sample_t *ending_buf[HARMONIZER_CHANNELS];
} harmonizer_voice_t;

/** Harmonizer "Digital Signal Processor" */
typedef struct {
    harmonizer_voice_t voices[MAX_HARMONIZER_VOICES];
    rolling_buffer_t sample_buf[HARMONIZER_CHANNELS];
    sample_t prev_period[HARMONIZER_CHANNELS];

    pitch_detection_data pitch_detect[HARMONIZER_CHANNELS];
    float pitch_alpha;

    /** preallocated buffers to compute the fft */
    fftwf_complex *fft_buf[HARMONIZER_CHANNELS];
    float *fft_in_buf[HARMONIZER_CHANNELS];
    float *fft_out_buf[HARMONIZER_CHANNELS];

    /** file to log result of pitch detection for debugging */
    FILE *pitch_log_file;
} harmonizer_dsp_t;

void harmonizer_dsp_init(harmonizer_dsp_t *dsp);

void harmonizer_dsp_log_pitch(harmonizer_dsp_t *dsp, char *filename);

/** Make harmonizer react to midi event */
void harmonizer_dsp_event(harmonizer_dsp_t *dsp, harmonizer_midi_event_t *evt);

/**
 * Main fonction for harmonizer processing. Takes a number of frames, input
 * samples * number of channels, output samples * number of channels.
 * */
int harmonizer_dsp_process(harmonizer_dsp_t *dsp, count_t nframes,
                           sample_t **in_stereo, sample_t **out_stereo);

void harmonizer_dsp_destroy(harmonizer_dsp_t *dsp);

#ifdef __cplusplus
}
#endif

#endif // HARMONIZER_DSP_H
