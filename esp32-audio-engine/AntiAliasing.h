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

// Add oversampled processing
template<int OVERSAMPLE>
class OversampledOscillator {
public:
    void process(float* output, int numSamples) {
        std::vector<float> osBuffer(OVERSAMPLE * numSamples);
        // Process at higher sample rate
        for(int i=0; i<OVERSAMPLE*numSamples; i++) {
            osBuffer[i] = generateSample();
        }
        // Decimate with FIR filter
        decimate(osBuffer.data(), output, numSamples);
    }

    // These would be implemented in a derived class
    virtual float generateSample() { return 0.0f; }
    virtual void decimate(float* in, float* out, int numSamples) {
        // Simple boxcar decimation for now, replace with proper FIR
        for (int i = 0; i < numSamples; ++i) {
            float sum = 0;
            for (int j = 0; j < OVERSAMPLE; ++j) {
                sum += in[i * OVERSAMPLE + j];
            }
            out[i] = sum / OVERSAMPLE;
        }
    }
};

#endif
