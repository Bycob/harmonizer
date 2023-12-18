#ifndef HARMONIZER_APP_H
#define HARMONIZER_APP_H

#include <stdbool.h>

#include "MidiFile.h"
#include "tinywav.h"

#include "harmonizer_dsp.h"
#include "harmonizer_midi.h"
#include "jack_backend.h"
#ifdef VISUALIZER
#include "visualizer.h"
#endif // VISUALIZER

typedef struct {
    bool use_jack_in;
    bool use_jack_out;
    bool use_midi_in;
    char *wav_in_fname;
    char *midi_in_fname;
    /// Whether the input audio / midi should be looped
    bool loop;
    char *wav_out_fname;
    // in case of dynamic input, they can be saved in a file for later debugging
    // or testing
    char *midi_input_out_fname;
    char *wav_input_out_fname;
    /// midi parameters
    harmonizer_midi_params_t midi;
    // PROCESSING
    bool use_fft;
#ifdef VISUALIZER
    // TODO visualizer_params?
    bool run_visualizer;
#endif // VISUALIZER
    // debug
    char *pitch_log_fname;
} harmonizer_app_params_t;

typedef struct {
    harmonizer_app_params_t params;
    harmonizer_dsp_t dsp;

    bool use_jack;
    jack_backend_t jack;
    int64_t jack_time;

#ifdef VISUALIZER
    visualizer_t visualizer;
#endif // VISUALIZER

    /// flag used to signal main thread that the app has finished reading the
    /// input file
    bool finished;
    // These buffers are used if jack is disabled
    sample_t *in[HARMONIZER_CHANNELS];
    sample_t *out[HARMONIZER_CHANNELS];

    // files to write to / read from
    TinyWav wav_in;
    long int wav_in_start;
    TinyWav wav_out;
    smf::MidiFile midi_in;
    /// cursor indicating where we are in the midi file
    int midi_in_index;
    /** save the input audio to a file */
    TinyWav wav_input_out;
    smf::MidiFile midi_input_out;
} harmonizer_app_t;

extern harmonizer_app_t _harmonizer_app;

void init_harmonizer_app(int argc, char **argv);

void run_harmonizer_app();

int harmonizer_jack_process(jack_nframes_t nframes, void *arg);

void destroy_harmonizer_app();

#endif // HARMONIZER_APP_H
