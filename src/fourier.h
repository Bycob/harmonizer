#ifndef FOURIER_H
#define FOURIER_H

#include <fftw3.h>

#include "harmonizer_dsp.h"

// allocate
void fft_allocate(harmonizer_dsp_t *dsp);

// Process the signal by decomposing it into windows, performing fft on the windows and then recomposing the signal afterwards
int fft_process(harmonizer_dsp_t *dsp, count_t nframes,
                           sample_t **in_stereo, sample_t **out_stereo);

#endif // FOURIER_H
