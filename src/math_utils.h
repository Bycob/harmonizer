#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <fftw3.h>

int min(int a, int b);

int max(int a, int b);

float hann(float t);

/** function to call at the beggining of the program to precompute the window
 * and accelerate the computations later */
void precompute_hann();

void fft(float *in, fftwf_complex *out, int nframes);

void ifft(fftwf_complex *in, float *out, int nframes);

/** Returns the fundamental period, in sample*/
float fundamental_period(fftwf_complex *fourier, int nframes);

#endif // MATH_UTILS_H
