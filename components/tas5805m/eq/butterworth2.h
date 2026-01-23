//
// Created by Michael on 22.01.2026.
//

#include <cmath>
#include <cstdint>
#include <array>
#include <string>
#include <stdexcept>


#include <cstdint>
#include <cmath>
#include <limits>

#include "biquad.h"


enum Butterworth2Type {
    BUTTERWORTH2_HIGHPASS = 0,
    BUTTERWORTH2_LOWPASS = 1,
};

Biquad527 butterworth2_tas5805m(double fs, double fc, Butterworth2Type type)
{
    const double Q     = 1.0 / std::sqrt(2.0);
    const double w0    = 2.0 * M_PI * fc / fs;
    const double alpha = std::sin(w0) / (2.0 * Q);
    const double cosw0 = std::cos(w0);

    double b0, b1, b2, a0, a1, a2;

    if (type == BUTTERWORTH2_LOWPASS) {
        b0 = (1.0 - cosw0) * 0.5;
        b1 = 1.0 - cosw0;
        b2 = (1.0 - cosw0) * 0.5;
    } else if (type == BUTTERWORTH2_HIGHPASS) {
        b0 = (1.0 + cosw0) * 0.5;
        b1 = -(1.0 + cosw0);
        b2 = (1.0 + cosw0) * 0.5;
    }

    a0 = 1.0 + alpha;
    a1 = -2.0 * cosw0;
    a2 = 1.0 - alpha;

    // Normalize
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;

    Biquad527 out{};
    out.b0 = float_to_5_27(b0);
    out.b1 = float_to_5_27(b1);
    out.b2 = float_to_5_27(b2);
    out.a1 = float_to_5_27(-a1);
    out.a2 = float_to_5_27(-a2);

    return out;
}
