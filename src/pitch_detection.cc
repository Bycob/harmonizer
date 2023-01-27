#include "pitch_detection.h"

#include <iostream>
#include <q/pitch/period_detector.hpp>

using namespace cycfi::q;

float probe_median_period(period_detector &detector, float *signal, int nframes,
                          int interval = 29) {
    std::vector<float> periods;
    periods.reserve(nframes / interval + 1);

    for (int i = 0; i < nframes; ++i) {
        detector(signal[i]);

        if (i % interval == 0) {
            periods.push_back(detector.predict_period());
        }
    }

    // compute median period
    std::sort(periods.begin(), periods.end());
    float median_period = periods.at(periods.size() / 2);
    return median_period;
}

// ===

float detect_period(float *signal, int nframes) {
    period_detector detector(94.0_Hz /* C2 */, 2093.0_Hz /* C7 */, 48000u,
                             -30.0_dB);
    return probe_median_period(detector, signal, nframes);
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
    return probe_median_period(detector, signal, nframes);
}
