// AntiAliasing.h
#ifndef ANTI_ALIASING_H
#define ANTI_ALIASING_H

#include <cmath>
#include <vector>

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

#define BLOCK_SIZE 64

// Add oversampled processing
template<int OVERSAMPLE>
class OversampledOscillator {
public:
    void process(float* output, int numSamples) {
        float osBuffer[OVERSAMPLE * BLOCK_SIZE]; // Static allocation

        // Process at higher sample rate
        for(int i=0; i<OVERSAMPLE*numSamples; i++) {
            phase += phaseIncrementOS;
            if (phase >= 1.0f) phase -= 1.0f;
            osBuffer[i] = generateSample(phase);
        }

        // Better FIR decimation
        decimateFIR(osBuffer, output, numSamples);
    }

protected:
    virtual float generateSample(float phase) = 0;

private:
    void decimateFIR(float* in, float* out, int numSamples) {
        // Implement proper FIR filter here
        // Example coefficients for 4x oversampling
        static const float firCoeffs[16] = { 0.026, 0.054, 0.082, 0.109, 0.13, 0.148, 0.159, 0.165, 0.165, 0.159, 0.148, 0.13, 0.109, 0.082, 0.054, 0.026 };

        for (int i = 0; i < numSamples; i++) {
            float sum = 0;
            for (int j = 0; j < 16; j++) {
                sum += in[i*OVERSAMPLE + j] * firCoeffs[j];
            }
            out[i] = sum;
        }
    }

    float phase = 0.0f;
    float phaseIncrementOS = 0.0f;
};

#endif
