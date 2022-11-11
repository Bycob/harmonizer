#include "pitch_detection.h"

#include <q/pitch/period_detector.hpp>

using namespace cycfi::q;

float detect_period(float *signal, int nframes) {
    period_detector detector(94.0_Hz /* C2 */, 2093.0_Hz /* C7 */, 48000u,
                             -30.0_dB);

    for (int i = 0; i < nframes; ++i) {
        detector(signal[i]);
    }
    return detector.predict_period();
}

float detect_period_continuous(float *signal, int nframes) { return 0.0; }
