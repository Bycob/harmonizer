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

pitch_detection_data alloc_pitch_detection_data() {
    pitch_detection_data wrapper;
    wrapper.period_detector = new period_detector(
        20.0_Hz /* C2 */, 2093.0_Hz /* C7 */, 48000u, -40.0_dB);
    return wrapper;
}

float detect_period_continuous(pitch_detection_data data, float *signal,
                               int nframes) {
    period_detector &detector =
        *static_cast<period_detector *>(data.period_detector);

    for (int i = 0; i < nframes; ++i) {
        detector(signal[i]);
    }
    return detector.predict_period();
}
