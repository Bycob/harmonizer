#include "harmonizer_app.h"

#include <getopt.h>
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

static void print_help() {
    printf("Harmonizer options:\n"
           "\n I/O:"
           "\n\t--audio_input_file Set audio file (wav) as input. If not "
           "specified, the input is the mic."
           "\n\t--midi_input_file Set midi file as input. If not specified, "
           "the input is the midi keyboard."
           "\n\t--loop Loop the audio/midi input when it's finished"
           "\n\t--save_audio_output Specify a file to save the audio output of "
           "the harmonizer."
           "\n\t--save_audio_input Specify a file to save the audio input, "
           "typically from the mic"
           "\n\t--save_midi_input Specify a file to save the midi input, "
           "typically from the midi keyboard."
           "\n\t--no_play_audio Do not play output audio. Output audio can "
           "still be saved in a file"
           "\n"
           "\n Debug:"
           "\n\t--pitch_log_file Log the detected pitch to a file"
           "\n"
           "\n Other:"
           "\n\t--help print this message and exit"
           "\n");
}

void init_harmonizer_app(int argc, char **argv) {
    harmonizer_app_params_t params;
    memset(&params, 0, sizeof(harmonizer_app_params_t));

    static struct option long_options[] = {
        {"audio_input_file", required_argument, 0, 'a'},
        {"midi_input_file", required_argument, 0, 'm'},
        {"loop", no_argument, 0, 'l'},
        {"save_audio_output", required_argument, 0, 'o'},
        {"save_audio_input", required_argument, 0, 's'},
        {"save_midi_input", required_argument, 0, 'i'},
        {"no_play_audio", no_argument, 0, 'p'},
        {"pitch_log_file", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    params.use_jack_in = true;
    params.use_jack_out = true;
    params.use_midi_in = true;

    int arg;
    int long_index = 0;
    while ((arg = getopt_long(argc, argv, "a:m:o:s:i:p:h:", long_options,
                              &long_index)) != -1) {
        switch (arg) {
        case 'a':
            params.use_jack_in = false;
            params.wav_in_fname = strdup(optarg);
            break;
        case 'm':
            params.use_midi_in = false;
            params.midi_in_fname = strdup(optarg);
            break;
        case 'l':
            params.loop = true;
            break;
        case 'o':
            params.wav_out_fname = strdup(optarg);
            break;
        case 's':
            params.wav_input_out_fname = strdup(optarg);
            break;
        case 'i':
            params.midi_input_out_fname = strdup(optarg);
            break;
        case 'p':
            params.use_jack_out = false;
            break;
        case 'd':
            params.pitch_log_fname = strdup(optarg);
            break;
        case 'h':
            print_help();
            exit(0);
        default:
            fprintf(stderr, "Unknwon argument!\n\n");
            print_help();
            exit(0);
        }
    }

    _harmonizer_app.params = params;

    _harmonizer_app.finished = false;
    _harmonizer_app.use_jack = params.use_jack_in || params.use_jack_out;
    int i;
    for (i = 0; i < HARMONIZER_CHANNELS; ++i) {
        _harmonizer_app.in[i] = NULL;
        _harmonizer_app.out[i] = NULL;
    }

    if (!_harmonizer_app.use_jack) {
        // can't use midi in offline process
        _harmonizer_app.params.use_midi_in = false;
    }

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
    if (params.wav_in_fname != NULL) {
        HANDLE_ERR(tinywav_open_read(&_harmonizer_app.wav_in,
                                     params.wav_in_fname, TW_SPLIT));
        _harmonizer_app.wav_in_start = ftell(_harmonizer_app.wav_in.f);
    }

    // Jack I/O
    if (_harmonizer_app.use_jack) {

        HANDLE_ERR(init_jack(&_harmonizer_app.jack, "client", NULL));

        /* tell the JACK server to call `process()' whenever
           there is work to be done.
        */

        jack_set_process_callback(_harmonizer_app.jack.client,
                                  harmonizer_jack_process, 0);
    }

    harmonizer_dsp_init(&_harmonizer_app.dsp);

    // Debug
    if (params.pitch_log_fname != NULL) {
        harmonizer_dsp_log_pitch(&_harmonizer_app.dsp, params.pitch_log_fname);
    }
}

void run_harmonizer_app() {
    if (_harmonizer_app.use_jack) {
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
        while (!_harmonizer_app.finished) {
#ifdef WIN32
            Sleep(50);
#else
            usleep(50000);
#endif
        }
    } else {
        // offline mode
        // TODO process input file until the end, write result to output
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
        if (_harmonizer_app.params.use_jack_in) {
            int input_chan = i;
            if (!_harmonizer_app.jack.stereo_input) {
                input_chan = 0;
            }

            in[i] = jack_port_get_buffer(
                _harmonizer_app.jack.input_ports[input_chan], nframes);
        } else {
            if (_harmonizer_app.in[i] == NULL) {
                _harmonizer_app.in[i] = calloc(nframes, sizeof(sample_t));
            }
            in[i] = _harmonizer_app.in[i];
        }

        if (_harmonizer_app.params.use_jack_out) {
            out[i] = jack_port_get_buffer(_harmonizer_app.jack.output_ports[i],
                                          nframes);
        } else {
            if (_harmonizer_app.out[i] == NULL) {
                _harmonizer_app.out[i] = calloc(nframes, sizeof(sample_t));
            }
            out[i] = _harmonizer_app.out[i];
        }
    }

    if (_harmonizer_app.params.wav_in_fname) {
        int read = tinywav_read_f(&_harmonizer_app.wav_in, in, nframes);
        if (read == 0) {
            if (_harmonizer_app.params.loop) {
                // rewind file
                fseek(_harmonizer_app.wav_in.f, _harmonizer_app.wav_in_start,
                      SEEK_SET);
                read = tinywav_read_f(&_harmonizer_app.wav_in, in, nframes);
            } else {
                fprintf(stderr, "End of file\n");
                _harmonizer_app.finished = true;
            }
        }
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

