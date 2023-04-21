#include "fourier.h"

#include <math.h>

#include "math_utils.h"

void fft_allocate(harmonizer_dsp_t *dsp) {
    const int win_size = 512;

    for (int i = 0; i < HARMONIZER_CHANNELS; i++) {
        dsp->fft_in_buf[i] = (float *)calloc(win_size, sizeof(float));
        dsp->fft_out_buf[i] =  (float *)calloc(win_size, sizeof(float));
        dsp->fft_buf[i] = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * win_size);
    }
}

// Process the signal by decomposing it into windows, performing fft on the windows and then recomposing the signal afterwards
int fft_process(harmonizer_dsp_t *dsp, count_t nframes,
                           sample_t **in_stereo, sample_t **out_stereo) {
    int i;
    sample_t *in, *out;

    for (i = 0; i < HARMONIZER_CHANNELS; i++) {
        in = in_stereo[i];
        out = out_stereo[i];

        float* fft_in_buf = dsp->fft_in_buf[i];
        float* fft_out_buf = dsp->fft_out_buf[i];
        fftwf_complex* fft_buf = dsp->fft_buf[i];

        memcpy(rbuf_next(&dsp->sample_buf[i]), in, nframes * sizeof(sample_t));
        memset(out, 0, nframes * sizeof(sample_t));

        const int win_size = 512;
        // hsize = half size
        const int win_hsize = win_size / 2;
        // TODO fields somewhere, not hardcoded
        // start of the current window
        int offset = - win_hsize;

        printf("start_loop !\n");
        // copy the remaining part of the previous frame
        // TODO don't hardcode win_hsize
        // TODO check that printf corresponds to the actual operation
        printf("copy fft_out [%d, %d] to out [%d, %d]\n", win_hsize, win_hsize + win_hsize , 0, win_hsize);
        memcpy(out, fft_out_buf + win_hsize, win_hsize * sizeof(sample_t));
        printf("fft_out[511] = %f\n", fft_out_buf[511]);
        // printf("fft_out[256] = %f\n", fft_out_buf[256]);
        printf("out[255] = %f\n", out[255]);

        while (true) {
            // copy window in fft_buf
            int in_start = max(0, offset);
            int cpy_size;
            sample_t *cpy_dst;

            // Il y a un problème dans fft_in, les deux parties de fft_in sont pas connectées entre elles
            // TODO à vérifier en priorité
            if (offset < 0) {
                cpy_size = win_size + offset;
                cpy_dst = fft_in_buf + (-offset);
            }
            else {
                cpy_size = min(win_size, nframes - offset);
                cpy_dst = fft_in_buf;
            }
            memcpy(cpy_dst, rbuf_get(&dsp->sample_buf[i], 0) + in_start, cpy_size * sizeof(sample_t));
            printf("copy in [%d, %d] to fft_in [%d, %d]\n", max(0, offset), max(0, offset) + cpy_size, offset < 0 ? -offset : 0, (offset < 0 ? -offset : 0) + cpy_size);
            
            if (nframes - offset < win_size) {
                // frame is incomplete, will be processed at the next timestep
                break;
            }

            // hamming
            for (int i = 0; i < win_size; ++i) {
                double t = (double)i / win_size;
                fft_in_buf[i] *= hann(t);
            }
            
            // TODO remove
            memcpy(fft_out_buf, fft_in_buf, win_size * sizeof(sample_t));
            printf("debug copy fft_in to fft_out\n");
 
            // fft
            fft(fft_in_buf, fft_buf, win_size);

            // low pass filter (0 half of the spectrum)
            for (int i = win_hsize; i < win_size; ++i) {
                // set to 0
                fft_buf[i][0] = 0;
                fft_buf[i][1] = 0;
            }

            // ifft
            // ifft(fft_buf, fft_out_buf, win_size);
            
            // copy buffer to output with an offset
            int out_off = offset + win_hsize;
            // out_off should always be >= 0
            // TODO (later) phase alignement?
            cpy_size = min(win_size, nframes - out_off);

            if (cpy_size != 0) {
                printf("copy fft_out [%d, %d] to out [%d, %d]\n", 0, cpy_size, out_off, out_off + cpy_size);
                for (int i = 0; i < cpy_size; ++i) {
                    out[out_off + i] += fft_out_buf[i];
                    if (out_off + i == 255 || out_off + i == 256) {
                        printf("out[%d] = %f\n", out_off + i, out[out_off + i]);
                        printf("fft_out[%d] = %f\n", i, fft_out_buf[i]);
                    }
                }
            }
            // TODO if cpy_size != win_size, next iteration we should copy the remaining part of fft_out_buf to the output

            offset += win_hsize;
        }

        printf("out[255] = %f\n", out[255]);
        printf("out[256] = %f\n", out[256]);
    }
    return 0;
}
