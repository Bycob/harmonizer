#include <math.h>

#include "harmonizer.h"

harmonizer_data_t _harmonizer_data;

#define HANN_SIZE 1024
float *hann_window;

float hann(float t) {
    float s = sin(M_PI * t);
    //    return s * s;
    return fminf(1, s * s * 50);
}

float lerp(float *values, float t, int n) {
    float xa = fmaxf(0, floor(t * n));
    float a = values[(int)xa];
    float b = values[(int)fminf(n, ceil(t * n))];
    float xt = t * n - xa;
    return a * (1 - xt) + b * xt;
}

void precompute() {
    hann_window = (float *)calloc(HANN_SIZE, sizeof(float));
    int i;
    for (i = 0; i < HANN_SIZE; ++i) {
        float t = i / (float)HANN_SIZE;
        hann_window[i] = hann(t);
    }

    // Init buffers
    _harmonizer_data.prev_buf = (float *)calloc(1024, sizeof(float));
}

int harmonizer_process(jack_nframes_t nframes, void *arg) {
    int i;
    jack_default_audio_sample_t *in, *out;

    for (i = 0; i < 2; i++) {
        int input_chan = i;
        if (!_harmonizer_data.jack->stereo_input) {
            input_chan = 0;
        }

        in = jack_port_get_buffer(
            _harmonizer_data.jack->input_ports[input_chan], nframes);
        out = jack_port_get_buffer(_harmonizer_data.jack->output_ports[i],
                                   nframes);

        int j;
        for (j = 0; j < nframes; ++j) {
            float t = (float)j / nframes;
            // 1 octave higher
            const int p = nframes / 4;
            int k = j / p;
            float *b1 = k == 0 ? _harmonizer_data.prev_buf : in;
            float *b2 = in;
            int jp = j - k * p;
            float h1 = hann((float)jp / p);
            out[j] = (h1 * b1[jp * 2] + (1 - h1) * b2[p + jp * 2]) * 2;
            // 1 octave lower
            // TODO align phase so there is no interference
            float h2 = hann(t / 2);
            out[j] +=
                (in[j / 2] * h2 +
                 _harmonizer_data.prev_buf[(nframes + j) / 2] * (1 - h2)) *
                2;
        }
        // TODO save 2 buffers
        if (i == 1)
            memcpy(_harmonizer_data.prev_buf, in,
                   nframes * sizeof(jack_default_audio_sample_t));
    }
    return 0;
}
