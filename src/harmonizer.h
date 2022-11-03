#ifndef HARMONIZER_H
#define HARMONIZER_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jack_backend.h"

// TODO use jack audio type to switch easily between double & floats

typedef struct {
    jack_backend_t *jack;
    /** rolling buffer of previous data */
    // XXX currently used as single buffer
    float *prev_buf;
    /** id of next write in rolling buffer */
    int prev_idx;
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

int
harmonizer_process( jack_nframes_t nframes, void *arg );

#endif // HARMONIZER_H
