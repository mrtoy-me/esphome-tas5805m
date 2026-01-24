//
// Created by Michael on 22.01.2026.
//

#ifndef BIQUAD_H
#define BIQUAD_H
#include <cstdint>


struct Biquad527 {
    uint32_t b0;
    uint32_t b1;
    uint32_t b2;
    uint32_t a1;
    uint32_t a2;
};


static inline int32_t float_to_5_27(double x)
{
    constexpr int    FRAC_BITS = 27;
    constexpr double SCALE     = double(1u << FRAC_BITS);

    // Valid 9.23 range
    constexpr double MAX_VAL =  256.0 - 1.0 / SCALE;
    constexpr double MIN_VAL = -256.0;

    if (x > MAX_VAL) x = MAX_VAL;
    if (x < MIN_VAL) x = MIN_VAL;

    // Scale
    double scaled = x * SCALE;
    long long q = std::llround(scaled);

    // Saturate to 32-bit
    if (q >  std::numeric_limits<int32_t>::max()) q =  std::numeric_limits<int32_t>::max();
    if (q <  std::numeric_limits<int32_t>::min()) q =  std::numeric_limits<int32_t>::min();

    return static_cast<int32_t>(q);
}

#endif //BIQUAD_H
