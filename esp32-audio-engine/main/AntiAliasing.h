// AntiAliasing.h
#ifndef ANTI_ALIASING_H
#define ANTI_ALIASING_H

#include <cmath>

class AntiAliasing {
public:
    // Polynomial Band-Limited stEP function
    // t: normalized phase position (0-1) relative to discontinuity
    // dt: normalized phase increment (frequency/sampleRate)
    // Returns correction factor to subtract from naive waveform
    static inline float poly_blep(float t, float dt) {
        if (t < dt) {
            t /= dt;
            // 2t - t^2 - 1
            return t + t - t * t - 1.0f;
        } else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            // -t^2 - 2t - 1
            return t * t + t + t + 1.0f;
        } else {
            // No correction needed
            return 0.0f;
        }
    }
};

#endif
