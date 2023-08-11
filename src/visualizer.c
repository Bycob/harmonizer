#include "visualizer.h"

#include <math.h>

int visualizer_start(visualizer_t *visualizer) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Could not initialize SDL\n");
        return 1;
    }
    visualizer->window = SDL_CreateWindow(
        "Harmonizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        VISUALIZER_WIDTH, VISUALIZER_HEIGHT, SDL_WINDOW_SHOWN);

    if (visualizer->window == NULL) {
        fprintf(stderr, "Could not open window\n");
        return 1;
    }

    visualizer->renderer =
        SDL_CreateRenderer(visualizer->window, -1, SDL_RENDERER_ACCELERATED);
    if (visualizer->renderer == NULL) {
        fprintf(stderr, "Could not create renderer\n");
        return 1;
    }

    // Initialize rectangles
    visualizer->width = VISUALIZER_WIDTH;
    visualizer->rectangles =
        (SDL_Rect *)calloc(visualizer->width, sizeof(SDL_Rect));
    return 0;
}

void visualizer_set_data(visualizer_t *visualizer, fftwf_complex *samples,
                         int count) {
    int height = VISUALIZER_HEIGHT;
    int i;
    for (i = 0; i < visualizer->width; ++i) {
        SDL_Rect *rect = visualizer->rectangles + i;
        int t = (int)((float)i / visualizer->width * count);
        float re = samples[t][0];
        float im = samples[t][1];
        float mod = sqrtf(re * re + im * im);
        rect->h = (int)(mod / 32 * height);
        rect->x = i;
        rect->y = height - rect->h;
        rect->w = 1;
    }
}

int visualizer_refresh(visualizer_t *visualizer) {
    SDL_SetRenderDrawColor(visualizer->renderer, 0, 0, 0, 255);
    SDL_RenderClear(visualizer->renderer);
    SDL_SetRenderDrawColor(visualizer->renderer, 0, 255, 0, 255);

    SDL_RenderFillRects(visualizer->renderer, visualizer->rectangles,
                        visualizer->width);
    SDL_RenderPresent(visualizer->renderer);
}

int visualizer_destroy(visualizer_t *visualizer) {
    if (visualizer->renderer != NULL)
        SDL_DestroyRenderer(visualizer->renderer);
    if (visualizer->window != NULL)
        SDL_DestroyWindow(visualizer->window);
    SDL_Quit();
    return 0;
}
