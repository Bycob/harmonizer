#include <math.h>

#include "harmonizer.h"
#include "pitch_detection.h"

harmonizer_data_t _harmonizer_data;

#define HANN_SIZE 1024
float *hann_window;

float hann(float t) {
    float s = sin(M_PI * t);
    return s * s;
    // return fminf(1, s * s * 50);
}

void fft(float *in, fftwf_complex *out, int nframes) {
    fftwf_plan p = fftwf_plan_dft_r2c_1d(nframes, in, out, FFTW_ESTIMATE);
    fftwf_execute(p);
    fftwf_destroy_plan(p);
}

/** Returns the fundamental period, in sample*/
float fundamental_period(fftwf_complex *fourier, int nframes) {
    int i;
    float max_val = 0;
    float max_idx = 0;
    const int ignore_edges = 64;

    for (i = ignore_edges; i < nframes / 2 - ignore_edges; ++i) {
        float val =
            fourier[i][0] * fourier[i][0] + fourier[i][1] * fourier[i][1];
        if (val > max_val) {
            max_val = val;
            max_idx = i;
        }
    }
    return max_idx == 0 ? 0 : (float)nframes / max_idx * 2;
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
    int buf_size = 1024;
    for (i = 0; i < 2; ++i) {
        _harmonizer_data.prev_buf[i] = (float *)calloc(buf_size, sizeof(float));
        _harmonizer_data.prev_period[i] = 1;
        _harmonizer_data.fft[i] =
            (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * buf_size);

        _harmonizer_data.pitch_detect[i] = alloc_pitch_detection_data();

        int v_id;
        for (v_id = 0; v_id < MAX_HARMONIZER_VOICES; ++v_id) {
            harmonizer_voice_t *voice = &_harmonizer_data.voices[v_id];
            voice->active = false;

            voice->prev_ratio[i] = 1;
            voice->prev_offset[i] = 1024;
        }
    }

    // Setup voices
    _harmonizer_data.voices[0].active = true;
    _harmonizer_data.voices[0].target_period = 184;
    _harmonizer_data.voices[1].active = true;
    _harmonizer_data.voices[1].target_period = 184 / 1.25;
    _harmonizer_data.voices[2].active = true;
    _harmonizer_data.voices[2].target_period = 184 / 1.5;
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
    /*
    fprintf(stderr, "in_nframes %f out_a %d out_b %d ta %f tb %f\n", in_nframes,
            out_a, out_b, ta, tb);
            */

    for (i = 0; i < out_count; ++i) {
        const float t = (float)i / out_count * (tb - ta) + ta;
        float h = t < 0.5 ? hann(t) : 1 - hann(t);
        out[out_a + i] += lerp(in, t, in_nframes) * hann(t);
        /*
        if (i == 100) {
            fprintf(stderr,
                    "%d: %f, frames = %f, hann = %f, in = %f, lerp = %f\n", i,
                    t, in_nframes, hann(t), in[(int)(t * in_nframes)],
                    lerp(in, t, in_nframes));
        }
        */
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
                  float prev_period) {
    float prev_whole_nframes;
    // float frame_shift = modf(nframes / (*prev_period), &whole_nframes) *
    // (*prev_period);
    modff(nframes / prev_period / 2, &prev_whole_nframes);
    prev_whole_nframes *= prev_period * 2;
    // float tshift = frame_shift / nframes * 2;
    // float neg_tshift = tshift - (*prev_period) / nframes * 2;

    // first wave with old frequency
    float out_nframes = prev_whole_nframes * (*prev_ratio) / 2;
    // number of frames filled from prev_buffer
    float out_filled = nframes - *prev_offset;
    // number of frames to fill up from prev_buffer
    // TODO if ratio > 2, out_fillup exceeds number of frames available
    float out_fillup = out_nframes - out_filled;
    // starting t in input buffer
    float tstart = out_filled / out_nframes / 2;

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
    int from_whole_nframes = prev_whole_nframes;

    while (out_start < nframes) {
        float real_out_end = fminf(out_end, nframes);
        float tend = (real_out_end - out_start) / (out_end - out_start) / 2;

        wave(from_buf, out, from_whole_nframes, (int)out_start,
             (int)real_out_end, 0.5, 0.5 + tend);
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
        from_whole_nframes = whole_nframes;
    }

    *prev_ratio = ratio;
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
        // fft(in, _harmonizer_data.fft[i], nframes);
        // float period = fundamental_period(_harmonizer_data.fft[i],
        // nframes);
        float period = detect_period_continuous(
            _harmonizer_data.pitch_detect[i], in, nframes);
        fprintf(stderr, "period = %f\n", period);
        if (period < 1 || period > 511)
            period = _harmonizer_data.prev_period[i];
        // frequency stabilisation
        if (fabs(period - 2 * _harmonizer_data.prev_period[i]) <
            _harmonizer_data.prev_period[i] * 0.01) {
            period = _harmonizer_data.prev_period[i];
        }
        fprintf(stderr, "retained period = %f\n", period);

        int v_id;
        for (v_id = 0; v_id < MAX_HARMONIZER_VOICES; ++v_id) {
            harmonizer_voice_t *voice = &_harmonizer_data.voices[v_id];

            if (!voice->active) {
                continue;
            }
            float ratio = voice->target_period / period;
            if (ratio > 2)
                ratio = 2;
            shift_signal(in, out, nframes, ratio, period,
                         _harmonizer_data.prev_buf[i], &voice->prev_offset[i],
                         &voice->prev_ratio[i],
                         _harmonizer_data.prev_period[i]);
        }
        _harmonizer_data.prev_period[i] = period;
        memcpy(_harmonizer_data.prev_buf[i], in,
               nframes * sizeof(jack_default_audio_sample_t));
    }
    return 0;
}
