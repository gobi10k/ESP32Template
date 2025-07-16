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
        for (size_t i = 0; i < blockSize; i++) {
            phase += 2.0f * M_PI * frequency.next() / sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
            
            // Use the direct envelope value from ADSR (no smoothing)
            buffer[i] = sinf(phase) * baseAmplitude * envelopeValue;
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

    float* getAmplitudePtr() { return &envelopeValue; }

private:
    SmoothedParameter frequency;
    float baseAmplitude;
    float envelopeValue; // Direct envelope value from ADSR
    float phase;
    float sampleRate;
};

#endif