// SineWave.h
#ifndef SINE_WAVE_H
#define SINE_WAVE_H

#include "AudioEngine.h"
#include "SmoothedParameter.h"
#include <cmath>

class SineWave : public AudioSource {
public:
    SineWave(float frequency = 440.0f, float amplitude = 0.0f)
        : frequency(frequency, 0.02f),
          baseAmplitude(amplitude),
          envelopeValue(0.0f), // Direct envelope value, no smoothing
          phase(0.0f), sampleRate(44100.0f) {}

    void setSampleRate(float sr) override {
        sampleRate = sr;
        frequency.setSampleRate(sr);
        // No need to set sample rate for envelopeValue since it's not smoothed
    }

    void renderBlock(float* buffer, size_t blockSize) override {
        // Replace sinf() with a more efficient alternative
        for (size_t i = 0; i < blockSize; i++) {
            phase += frequency.next() / sampleRate;
            if (phase >= 1.0f) phase -= 1.0f;
            
            // Fast sine approximation (5th order polynomial)
            buffer[i] = fastSin(phase * 2.0f * M_PI) * baseAmplitude * envelopeValue;
        }
    }

    void setFrequency(float freq, bool immediate = false) {
        if (immediate) {
            frequency.setTargetImmediate(freq);
        } else {
            frequency.setTarget(freq);
        }
    }

    void setAmplitude(float amp, bool immediate = false) {
        baseAmplitude = amp;
        // Don't modify modulatedAmplitude here - it's controlled by ADSR
    }

    float* getAmplitudePtr() { return &baseAmplitude; }

private:
    float fastSin(float x) {
        // Normalize to [0, 2π]
        x = fmodf(x, 2.0f * M_PI);
        if (x < 0) x += 2.0f * M_PI;

        // 5th order polynomial approximation
        const float B = 4.0f/M_PI;
        const float C = -4.0f/(M_PI*M_PI);
        const float P = 0.225f;

        float y = B * x + C * x * fabsf(x);
        return P * (y * fabsf(y) - y) + y;
    }

    SmoothedParameter frequency;
    float baseAmplitude;
    float envelopeValue; // Direct envelope value from ADSR
    float phase;
    float sampleRate;
};

#endif