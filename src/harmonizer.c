#include <math.h>

#include "harmonizer.h"

harmonizer_data_t _harmonizer_data;

#define HANN_SIZE 1024
float *hann_window;

float hann(float t) {
    float s = sin(M_PI * t);
    return s * s;
    // return fminf(1, s * s * 50);
}

/** Linear interpolation
 *
 * n size of "values". Is float for an experiment
 * */
float lerp(float *values, float t, float n) {
    int xa = (int)fminf(n - 2, fmaxf(0, floor(t * n)));
    float a = values[xa];
    float b = values[xa + 1];
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
    for (i = 0; i < 2; ++i) {
        _harmonizer_data.prev_buf[i] = (float *)calloc(1024, sizeof(float));
        int j;
        for (j = 0; j < 1024; ++j) {
            _harmonizer_data.prev_buf[i][j] = 0;
        }
        _harmonizer_data.prev_ratio[i] = 1;
        _harmonizer_data.prev_period[i] = 1;
        _harmonizer_data.prev_offset[i] = 1024;
    }
}

// DEBUG
void print_array(float *array, int nframes) {
    fprintf(stderr, "[");
    int i;
    for (i = 0; i < nframes; ++i) {
        if (i != 0) {
            fprintf(stderr, ", ");
        }
        fprintf(stderr, "%f", array[i]);
    }
    fprintf(stderr, "]\n");
}

void wave(float *in, float *out, float in_nframes, int out_a, int out_b,
          float ta, float tb) {
    int i;
    const int out_count = out_b - out_a;
    fprintf(stderr, "in_nframes %f out_a %d out_b %d ta %f tb %f\n", in_nframes,
            out_a, out_b, ta, tb);

    for (i = 0; i < out_count; ++i) {
        const float t = (float)i / out_count * (tb - ta) + ta;
        float h = t < 0.5 ? hann(t) : 1 - hann(t);
        out[out_a + i] += lerp(in, t, in_nframes) * hann(t);
        if (i == 100) {
            fprintf(stderr,
                    "%d: %f, frames = %f, hann = %f, in = %f, lerp = %f\n", i,
                    t, in_nframes, hann(t), in[(int)(t * in_nframes)],
                    lerp(in, t, in_nframes));
        }
    }
}

/**
 * Shift signal with phase alignment
 *
 * \param nframes number of frames in each buffer
 *
 * \param ratio how much the frequency should be divided / multiplied
 *
 * \param period the original (precomputed) period of the original signal for
 * phase alignment, in samples.
 *
 * \param prev_buf the previous buffer
 *
 * \param prev_offset index of the start of the previous "wave" in the previous
 * sample. Will be updated with the new offset.
 */
void shift_signal(float *in, float *out, int nframes, float ratio, float period,
                  float *prev_buf, float *prev_offset, float *prev_ratio,
                  float *prev_period) {
    print_array(out + 512, 20);
    float prev_whole_nframes;
    // float frame_shift = modf(nframes / (*prev_period), &whole_nframes) *
    // (*prev_period);
    modff(nframes / (*prev_period) / 2, &prev_whole_nframes);
    prev_whole_nframes *= (*prev_period) * 2;
    // float tshift = frame_shift / nframes * 2;
    // float neg_tshift = tshift - (*prev_period) / nframes * 2;

    // first wave with old frequency
    float out_nframes = prev_whole_nframes * (*prev_ratio) / 2;
    // number of frames to fill up from prev_buffer
    // TODO if ratio > 2, out_fillup exceeds number of frames available
    float out_fillup = out_nframes - (nframes - *prev_offset);
    // starting t in input buffer
    float tstart = out_fillup / out_nframes / 2;

    if (out_fillup >= 1) {
        wave(prev_buf, out, prev_whole_nframes, 0, (int)out_fillup,
             0.5 + tstart, 1);
        wave(prev_buf, out, prev_whole_nframes, 0, (int)out_fillup, tstart,
             0.5);
        fprintf(stderr, "FILL UP!\n");
    }
    fprintf(stderr,
            "whole %f out_nframes %f out_fillup %f tstart %f prev_ratio %f "
            "prev_offset %f\n",
            prev_whole_nframes, out_nframes, out_fillup, tstart, *prev_ratio,
            *prev_offset);

    // repeat new frequency
    float whole_nframes;
    // frame_shift = modf(nframes / period, &integral) * period;
    modff(nframes / period / 2, &whole_nframes);
    whole_nframes *= period * 2;
    // tshift = frame_shift / nframes * 2;
    // neg_tshift = tshift - period / nframes * 2;
    out_nframes = whole_nframes * ratio / 2;

    float out_start = out_fillup;
    *prev_offset = out_start;
    float out_end = out_fillup + out_nframes;
    float *from_buf = prev_buf;

    while (out_start < nframes) {
        float real_out_end = fminf(out_end, nframes);
        float tend = (real_out_end - out_start) / (out_end - out_start) / 2;

        wave(from_buf, out, whole_nframes, (int)out_start, (int)real_out_end,
             0.5, 0.5 + tend);
        wave(in, out, whole_nframes, (int)out_start, (int)real_out_end, 0,
             tend);
        fprintf(stderr,
                "whole %f out_nframes %f ratio %f out_start %f "
                "real_out_end %f\n",
                whole_nframes, out_nframes, ratio, out_start, real_out_end);

        *prev_offset = out_start;
        out_start = out_end;
        out_end = out_start + out_nframes;
        from_buf = in;
    }
    print_array(prev_buf + 512, 20);
    print_array(out, 20);

    *prev_ratio = ratio;
    *prev_period = period;
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

        memset(out, 0.f, nframes * sizeof(jack_default_audio_sample_t));
        shift_signal(in, out, nframes, 0.6666, 1, _harmonizer_data.prev_buf[i],
                     &_harmonizer_data.prev_offset[i],
                     &_harmonizer_data.prev_ratio[i],
                     &_harmonizer_data.prev_period[i]);

        memcpy(_harmonizer_data.prev_buf[i], in,
               nframes * sizeof(jack_default_audio_sample_t));
    }
    return 0;
}
