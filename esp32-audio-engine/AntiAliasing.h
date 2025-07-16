// AntiAliasing.h
#ifndef ANTI_ALIASING_H
#define ANTI_ALIASING_H

#include <cmath>

class AntiAliasing {
public:
    // PolyBLEP function to correct a discontinuity
    // t: phase position relative to the discontinuity (0 to 1)
    // dt: phase increment per sample
    static inline float poly_blep(float t, float dt) {
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        } else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        } else {
            return 0.0f;
        }
    }
};

#endif
