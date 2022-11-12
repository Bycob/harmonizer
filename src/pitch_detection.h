#ifndef PITCH_DETECTION_H
#define PITCH_DETECTION_H

#ifdef __cplusplus
extern "C" {
#endif

/** Detect the period of the signal */
float detect_period(float *signal, int nframes);

typedef struct {
    void *period_detector;
} pitch_detection_data;

pitch_detection_data alloc_pitch_detection_data();

/** Detect period retaining the data provided by precedent calls of
 * `detect_period_continuous` */
float detect_period_continuous(pitch_detection_data data, float *signal,
                               int nframes);
#ifdef __cplusplus
}
#endif

#endif // PITCH_DETECTION_H
