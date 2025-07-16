// SquareWave.h
#ifndef SQUARE_WAVE_H
#define SQUARE_WAVE_H

#include "AudioEngine.h"
#include "SmoothedParameter.h"
#include "AntiAliasing.h"
#include <cmath>

class SquareWave : public AudioSource {
public:
    SquareWave(float frequency = 440.0f, float amplitude = 0.0f)
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

            float dt = freq / sampleRate;
            phase += dt;
            if (phase >= 1.0f) phase -= 1.0f;

            float raw_sqr = phase < 0.5f ? 1.0f : -1.0f;

            float correction = AntiAliasing::poly_blep(phase, dt);
            float correction2 = AntiAliasing::poly_blep(fmodf(phase + 0.5f, 1.0f), dt);

            buffer[i] = (raw_sqr - correction + correction2) * baseAmplitude * envelopeValue;
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
