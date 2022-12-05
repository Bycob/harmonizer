#ifndef HARMONIZER_APP_H
#define HARMONIZER_APP_H

#include <stdbool.h>

#include "tinywav.h"

#include "harmonizer_dsp.h"
#include "jack_backend.h"

typedef struct {
    bool use_jack_in;
    bool use_jack_out;
    bool use_midi_in;
    char *wav_in_fname;
    char *midi_in_fname;
    char *wav_out_fname;
    // in case of dynamic input, they can be saved in a file for later debugging
    char *midi_input_out_fname;
    char *wav_input_out_fname;
} harmonizer_app_params_t;

typedef struct {
    harmonizer_app_params_t params;
    harmonizer_dsp_t dsp;

    jack_backend_t jack;

    // files to write to / read from
    TinyWav wav_out;
    /** save the input file */
    TinyWav wav_input_out;
} harmonizer_app_t;

extern harmonizer_app_t _harmonizer_app;

void init_harmonizer_app(int argc, char **argv);

void run_harmonizer_app();

int harmonizer_jack_process(jack_nframes_t nframes, void *arg);

void destroy_harmonizer_app();

#endif // HARMONIZER_APP_H
