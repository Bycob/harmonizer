#include "math_utils.h"

#include <math.h>
#include <stdlib.h>

int min(int a, int b) { return a < b ? a : b; }

int max(int a, int b) { return a > b ? a : b; }

#define HANN_SIZE 1024
float *hann_window;

float hann(float t) {
    float s = sin(M_PI * t);
    return s * s;
    // return fminf(1, s * s * 50);
}

// TODO use precomputed version to go faaast
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

void ifft(fftwf_complex *in, float *out, int nframes) {
    fftwf_plan p =
        fftwf_plan_dft_c2r_1d(nframes, in, out, FFTW_ESTIMATE | FFTW_BACKWARD);
    fftwf_execute(p);
    fftwf_destroy_plan(p);
}

/** Returns the fundamental period, in sample*/
// Does not work
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
