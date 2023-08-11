#ifndef HARMONIZER_VISUALIZER
#define HARMONIZER_VISUALIZER

#include <SDL2/SDL.h>
#include <fftw3.h>

#include "harmonizer_dsp.h" // for sample_t

#define VISUALIZER_WIDTH 640
#define VISUALIZER_HEIGHT 480

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;

    int width;
    SDL_Rect *rectangles;
} visualizer_t;

/** Create visualizer window & init visualizer */
int visualizer_start(visualizer_t *visualizer);

/** Update visualizer data */
void visualizer_set_data(visualizer_t *visualizer, fftwf_complex *samples,
                         int count);

/** Refresh visualizer window */
int visualizer_refresh(visualizer_t *visualizer);

/** Destroy visualizer window and free resources */
int visualizer_destroy(visualizer_t *visualizer);

#ifdef __cplusplus
}
#endif

#endif // HARMONIZER_VISUALIZER
