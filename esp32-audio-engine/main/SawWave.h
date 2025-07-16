// SawWave.h
#ifndef SAW_WAVE_H
#define SAW_WAVE_H

#include "AudioEngine.h"
#include "SmoothedParameter.h"
#include "AntiAliasing.h"
#include <cmath>

class SawWave : public AudioSource {
public:
    SawWave(float frequency = 440.0f, float amplitude = 0.0f)
        : frequency(frequency, 0.02f),
          baseAmplitude(amplitude),
          envelopeValue(0.0f),
          phase(0.0f), sampleRate(44100.0f) {}

    void setSampleRate(float sr) override {
        sampleRate = sr;
        frequency.setSampleRate(sr);
    }

    void renderBlock(float* buffer, size_t blockSize) override {
        for (size_t i = 0; i < blockSize; i++) {
            float freq = frequency.next();
            freq = std::min(freq, sampleRate * 0.49f); // Nyquist limit

            float dt = freq / sampleRate;
            phase = fmodf(phase + dt, 1.0f);

            float raw_saw = 2.0f * phase - 1.0f;
            float correction = AntiAliasing::poly_blep(phase, dt);

            buffer[i] = (raw_saw - correction) * baseAmplitude * envelopeValue;
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
    }

    float* getAmplitudePtr() { return &envelopeValue; }


private:
    SmoothedParameter frequency;
    float baseAmplitude;
    float envelopeValue;
    float phase;
    float sampleRate;
};

#endif
