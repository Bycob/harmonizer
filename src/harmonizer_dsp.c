#include "harmonizer_dsp.h"

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

#define HANN_SIZE 1024
float *hann_window;

float hann(float t) {
    float s = sin(M_PI * t);
    return s * s;
    // return fminf(1, s * s * 50);
}

/** function to call at the beggining of the program to precompute the window
 * and accelerate the computations later */
void precompute_hann() {
    hann_window = (float *)calloc(HANN_SIZE, sizeof(float));
    int i;
    for (i = 0; i < HANN_SIZE; ++i) {
        float t = i / (float)HANN_SIZE;
        hann_window[i] = hann(t);
    }
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
sample_t lerp(sample_t *values, float t, float n) {
    int xa = (int)fminf(n - 2, fmaxf(0, floor(t * n)));
    sample_t a = values[xa];
    sample_t b = values[xa + 1];
    float xt = t * n - xa;
    return a * (1 - xt) + b * xt;
}

/** Generate one resampled window of the input signal, between out_a & out_b,
 * with ta & tb as ratio of input to be taken (between 0 and 1) */
void wave(sample_t *in, sample_t *out, float in_nframes, int out_a, int out_b,
          float ta, float tb) {
    int i;
    const int out_count = out_b - out_a;
    /*
    fprintf(stderr, "in_nframes %f out_a %d out_b %d ta %f tb %f\n", in_nframes,
            out_a, out_b, ta, tb);
            */

    for (i = 0; i < out_count; ++i) {
        const float t = (float)i / out_count * (tb - ta) + ta;
        float h = t < 0.5 ? hann(t) : hann(1 - t);
        out[out_a + i] += lerp(in, t, in_nframes) * h;
        if (i == 0) {
            fprintf(stderr, "Start with %f at t = %f\n",
                    lerp(in, t, in_nframes) * h, t);
        } else if (i == out_count - 1) {
            fprintf(stderr, "Finish with %f at t = %f\n",
                    lerp(in, t, in_nframes) * h, t);
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
void shift_signal(sample_t *in, sample_t *out, int nframes, float ratio,
                  float period, sample_t *prev_buf, float *prev_offset,
                  float *prev_ratio, float *prev_period) {
    float prev_whole_nframes;
    modff(nframes / (*prev_period) / 2, &prev_whole_nframes);
    prev_whole_nframes *= *prev_period * 2;

    // first wave with old frequency
    float out_nframes = prev_whole_nframes * (*prev_ratio) / 2;
    // number of frames filled from prev_buffer
    float out_filled = nframes - *prev_offset;
    // number of frames to fill up from prev_buffer
    float out_fillup = out_nframes - out_filled;
    // starting t in input buffer
    float tstart = out_filled / out_nframes / 2;

    if (out_fillup >= nframes) {
        // if ratio > 2, fillup exceeds the number of frames and can overlap
        // on multiple buffers.
        float tend = (1 - (out_fillup - nframes) / out_nframes) / 2;
        fprintf(stderr,
                "prev_ratio %f out_nframes %f prev_period %f out_fillup %f "
                "prev_offset %f tstart %f tend %f\n",
                *prev_ratio, out_nframes, *prev_period, out_fillup,
                *prev_offset, tstart, tend);
        wave(prev_buf, out, prev_whole_nframes, 0, nframes, 0.5 + tstart,
             0.5 + tend);
        wave(prev_buf, out, prev_whole_nframes, 0, nframes, tstart, tend);

        *prev_offset -= nframes;
        return;
    } else if (out_fillup >= 1) {
        fprintf(stderr,
                "whole %f out_nframes %f out_fillup %f tstart %f prev_ratio %f "
                "prev_offset %f\n",
                prev_whole_nframes, out_nframes, out_fillup, tstart,
                *prev_ratio, *prev_offset);
        wave(prev_buf, out, prev_whole_nframes, 0, (int)out_fillup,
             0.5 + tstart, 1);
        wave(prev_buf, out, prev_whole_nframes, 0, (int)out_fillup, tstart,
             0.5);
        fprintf(stderr, "FILL UP!\n");
    }

    // repeat new frequency
    float whole_nframes;
    modff(nframes / period / 2, &whole_nframes);
    whole_nframes *= period * 2;
    out_nframes = whole_nframes * ratio / 2;

    float out_start = out_fillup;
    *prev_offset = out_start;
    float out_end = out_fillup + out_nframes;
    float *from_buf = prev_buf;
    int from_whole_nframes = prev_whole_nframes;

    while (out_start < nframes) {
        float real_out_end = fminf(out_end, nframes);
        float tend = (real_out_end - out_start) / (out_end - out_start) / 2;

        fprintf(stderr,
                "whole %f out_nframes %f ratio %f out_start %f "
                "real_out_end %f tend %f/%f\n",
                whole_nframes, out_nframes, ratio, out_start, real_out_end,
                tend, 0.5 + tend);
        wave(from_buf, out, from_whole_nframes, (int)out_start,
             (int)real_out_end, 0.5, 0.5 + tend);
        wave(in, out, whole_nframes, (int)out_start, (int)real_out_end, 0,
             tend);

        *prev_offset = out_start;
        out_start = out_end;
        out_end = out_start + out_nframes;
        from_buf = in;
        from_whole_nframes = whole_nframes;
    }

    *prev_ratio = ratio;
    *prev_period = period;
}

void harmonizer_dsp_init(harmonizer_dsp_t *dsp) {
    // Init buffers
    int buf_size = 1024;
    int i;

    for (i = 0; i < HARMONIZER_CHANNELS; ++i) {
        dsp->prev_buf[i] = (sample_t *)calloc(buf_size, sizeof(sample_t));
        dsp->prev_period[i] = 1.0;
        dsp->fft[i] =
            (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * buf_size);

        dsp->pitch_detect[i] = alloc_pitch_detection_data();

        int v_id;
        for (v_id = 0; v_id < MAX_HARMONIZER_VOICES; ++v_id) {
            harmonizer_voice_t *voice = &dsp->voices[v_id];
            voice->active = false;

            voice->prev_ratio[i] = 1;
            voice->prev_offset[i] = 1024;
            voice->prev_period[i] = 1;
        }
    }

    // Setup voices
    // TODO use midi input
    dsp->voices[0].active = false;
    dsp->voices[0].target_period = 184 / 2;
    dsp->voices[1].active = true;
    dsp->voices[1].target_period = 184 / 1.25;
    dsp->voices[2].active = false;
    dsp->voices[2].target_period = 184 / 1.5;

    // Debug pitch detection
    dsp->pitch_log_file = NULL;
}

void harmonizer_dsp_log_pitch(harmonizer_dsp_t *dsp, char *filename) {
    dsp->pitch_log_file = fopen(filename, "w");
    // TODO manage errors
}

int harmonizer_dsp_process(harmonizer_dsp_t *dsp, count_t nframes,
                           sample_t **in_stereo, sample_t **out_stereo) {
    int i;
    sample_t *in, *out;

    for (i = 0; i < HARMONIZER_CHANNELS; i++) {
        in = in_stereo[i];
        out = out_stereo[i];

        memset(out, 0, nframes * sizeof(sample_t));
        // fft(in, dsp->fft[i], nframes);
        // float period = fundamental_period(dsp->fft[i],
        // nframes);
        // pitch detection
        float period =
            detect_period_continuous(dsp->pitch_detect[i], in, nframes);
        fprintf(stderr, "period = %f\n", period);
        if (dsp->pitch_log_file != NULL) {
            fprintf(dsp->pitch_log_file, "%0.2f\n", period);
        }

        if (period < 1 || period > 511)
            period = dsp->prev_period[i];
        // frequency stabilisation
        if (fabs(period - 2 * dsp->prev_period[i]) <
            dsp->prev_period[i] * 0.01) {
            period = dsp->prev_period[i];
        }
        fprintf(stderr, "retained period = %f\n", period);
        dsp->prev_period[i] = period;

        int v_id;
        for (v_id = 0; v_id < MAX_HARMONIZER_VOICES; ++v_id) {
            harmonizer_voice_t *voice = &dsp->voices[v_id];

            if (!voice->active) {
                continue;
            }
            float ratio = voice->target_period / period;
            // shield against very high ratio. should never be higher than
            // 3 octaves I guess (ratio = 8)
            if (ratio > 8) {
                ratio = 1;
            }

            shift_signal(in, out, nframes, ratio, period, dsp->prev_buf[i],
                         &voice->prev_offset[i], &voice->prev_ratio[i],
                         &voice->prev_period[i]);
        }

        // add original signal * 2
        int u;
        for (u = 0; u < nframes; ++u) {
            out[u] += in[u] * 0.;
            out[u] /= 2;
        }

        memcpy(dsp->prev_buf[i], in,
               nframes * sizeof(jack_default_audio_sample_t));
    }
    return 0;
}

void harmonizer_dsp_destroy(harmonizer_dsp_t *dsp) {
    if (dsp->pitch_log_file != NULL) {
        fclose(dsp->pitch_log_file);
    }
}
