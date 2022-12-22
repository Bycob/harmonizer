#include "harmonizer_app.h"

#include <signal.h>
#include <stdio.h>

harmonizer_app_t _harmonizer_app;

#define HANDLE_ERR(Line)                                                       \
    if (Line != 0)                                                             \
    exit(1)
// TODO add error message

static void signal_handler(int sig) {
    destroy_harmonizer_app();
    fprintf(stderr, "signal received, exiting ...\n");
    exit(0);
}

void init_harmonizer_app(int argc, char **argv) {
    harmonizer_app_params_t params;
    // TODO parse arguments
    params.use_jack_in = true;
    params.wav_input_out_fname = "harmonizer_input.wav";
    params.wav_out_fname = "harmonizer_output.wav";

    _harmonizer_app.params = params;

    if (params.wav_out_fname != NULL) {
        HANDLE_ERR(tinywav_open_write(&_harmonizer_app.wav_out, 2, 48000,
                                      TW_FLOAT32, TW_SPLIT,
                                      params.wav_out_fname));
        // fprintf(stderr, "Output file %s could not be opened.", filename)
    }
    if (params.wav_input_out_fname != NULL) {
        HANDLE_ERR(tinywav_open_write(&_harmonizer_app.wav_input_out, 2, 48000,
                                      TW_FLOAT32, TW_SPLIT,
                                      params.wav_input_out_fname));
    }
    if (params.use_jack_in) {

        HANDLE_ERR(init_jack(&_harmonizer_app.jack, "client", NULL));

        /* tell the JACK server to call `process()' whenever
           there is work to be done.
        */

        jack_set_process_callback(_harmonizer_app.jack.client,
                                  harmonizer_jack_process, 0);
    }

    harmonizer_dsp_init(&_harmonizer_app.dsp);
    // TODO remove & use parameters
    harmonizer_dsp_log_pitch(&_harmonizer_app.dsp, "pitch_log.txt");
}

void run_harmonizer_app() {
    if (_harmonizer_app.params.use_jack_in) {
        HANDLE_ERR(init_io(&_harmonizer_app.jack));
        HANDLE_ERR(start_jack(&_harmonizer_app.jack));

        /* install a signal handler to properly quits jack client */
#ifdef WIN32
        signal(SIGINT, signal_handler);
        signal(SIGABRT, signal_handler);
        signal(SIGTERM, signal_handler);
#else
        signal(SIGQUIT, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGHUP, signal_handler);
        signal(SIGINT, signal_handler);
#endif

        /* keep running until the transport stops */
        while (1) {
#ifdef WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
        }
    }
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 */
int harmonizer_jack_process(jack_nframes_t nframes, void *arg) {
    int i;
    // TODO dynamic allocation to support mono & stereo in input & output
    sample_t *in[HARMONIZER_CHANNELS], *out[HARMONIZER_CHANNELS];

    for (i = 0; i < HARMONIZER_CHANNELS; i++) {
        int input_chan = i;
        if (!_harmonizer_app.jack.stereo_input) {
            input_chan = 0;
        }

        in[i] = jack_port_get_buffer(
            _harmonizer_app.jack.input_ports[input_chan], nframes);
        out[i] =
            jack_port_get_buffer(_harmonizer_app.jack.output_ports[i], nframes);
    }

    if (_harmonizer_app.params.wav_input_out_fname) {
        tinywav_write_f(&_harmonizer_app.wav_input_out, in, nframes);
    }

    harmonizer_dsp_process(&_harmonizer_app.dsp, nframes, in, out);

    // save as needed
    if (_harmonizer_app.params.wav_out_fname) {
        tinywav_write_f(&_harmonizer_app.wav_out, out, nframes);
    }

    return 0;
}

void destroy_harmonizer_app() {
    // stop jack
    if (_harmonizer_app.params.use_jack_in) {
        jack_client_close(_harmonizer_app.jack.client);
    }

    // close all files
    if (_harmonizer_app.params.wav_input_out_fname) {
        tinywav_close_write(&_harmonizer_app.wav_input_out);
    }
    if (_harmonizer_app.params.wav_out_fname) {
        tinywav_close_write(&_harmonizer_app.wav_out);
    }

    // remove dsp
    harmonizer_dsp_destroy(&_harmonizer_app.dsp);
}

