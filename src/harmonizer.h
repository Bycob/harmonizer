#ifndef HARMONIZER_H
#define HARMONIZER_H

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fftw3.h>

#include "jack_backend.h"
#include "pitch_detection.h"

// TODO use jack audio type to switch easily between double & floats

typedef struct {
    jack_backend_t *jack;
    /** rolling buffer of previous data */
    // XXX currently used as single buffer
    float *prev_buf[2];
    /** id of next write in rolling buffer */
    // int prev_idx[2];
    float prev_ratio[2];
    float prev_period[2];
    float prev_offset[2];

    /** preallocated buffers to compute the fft */
    fftwf_complex *fft[2];
    pitch_detection_data pitch_detect[2];
} harmonizer_data_t;

extern harmonizer_data_t _harmonizer_data;

void precompute();

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */

int harmonizer_process(jack_nframes_t nframes, void *arg);

#endif // HARMONIZER_H
