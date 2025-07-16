// WavetableSynth.h
#ifndef WAVETABLE_SYNTH_H
#define WAVETABLE_SYNTH_H

#include "AudioEngine.h"
#include "SmoothedParameter.h"
#include <vector>
#include <cmath>

#define WAVETABLE_SIZE 256

class WavetableSynth : public AudioSource {
public:
    WavetableSynth(float frequency = 440.0f)
        : frequency(frequency, 0.02f),
          position(0.0f, 0.02f),
          morph(0.0f, 0.02f),
          gain(1.0f),
          phase(0.0f),
          sampleRate(44100.0f) {}

    // Waveform management
    void addWavetable(const std::vector<float>& newWavetable) {
        wavetables.push_back(newWavetable);
    }

    void generateSineWavetable() {
        std::vector<float> sineTable(WAVETABLE_SIZE);
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            sineTable[i] = sinf(2.0f * M_PI * i / WAVETABLE_SIZE);
        }
        addWavetable(sineTable);
    }

    void generateSawWavetable() {
        std::vector<float> sawTable(WAVETABLE_SIZE);
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            sawTable[i] = 2.0f * (float(i) / WAVETABLE_SIZE) - 1.0f;
        }
        addWavetable(sawTable);
    }

    void generateSquareWavetable() {
        std::vector<float> squareTable(WAVETABLE_SIZE);
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            squareTable[i] = (i < WAVETABLE_SIZE / 2) ? 1.0f : -1.0f;
        }
        addWavetable(squareTable);
    }

    void generateTriangleWavetable() {
        std::vector<float> triangleTable(WAVETABLE_SIZE);
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float val = 2.0f * (float(i) / WAVETABLE_SIZE) - 1.0f;
            triangleTable[i] = 2.0f * (fabs(val) - 0.5f);
        }
        addWavetable(triangleTable);
    }

    void generateComplexWaveform(int harmonicProfile) {
        // Simple additive synth for demonstration.
        // For true anti-aliasing, harmonics should be removed if they exceed sampleRate / 2.
        // This is a simplified version for demonstration.
        std::vector<float> complexTable(WAVETABLE_SIZE, 0.0f);
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            for (int h = 1; h <= 16; ++h) { // Up to 16 harmonics
                if (harmonicProfile & (1 << (h-1))) {
                    complexTable[i] += (1.0f/h) * sinf(2.0f * M_PI * h * i / WAVETABLE_SIZE);
                }
            }
        }
        // Normalize
        float maxVal = 0.0;
        for(float val : complexTable) maxVal = fmax(maxVal, fabs(val));
        if (maxVal > 0.0) {
            for(float& val : complexTable) val /= maxVal;
        }
        addWavetable(complexTable);
    }


    // Playback control
    void setPosition(float pos) { position.setTarget(pos); }
    void setMorph(float m) { morph.setTarget(m); }
    void setFrequency(float freq) { frequency.setTarget(freq); }

    // Modulation targets
    float* getPositionPtr() { return position.getTargetPtr(); }
    float* getMorphPtr() { return morph.getTargetPtr(); }
    float* getFrequencyPtr() { return frequency.getTargetPtr(); }
    float* getGainPtr() { return &gain; }

    // AudioSource implementation
    void setSampleRate(float sr) override {
        sampleRate = sr;
        frequency.setSampleRate(sr);
        position.setSampleRate(sr);
        morph.setSampleRate(sr);
    }

    void renderBlock(float* buffer, size_t blockSize) override {
        if (wavetables.empty()) {
            for(size_t i = 0; i < blockSize; ++i) buffer[i] = 0.0f;
            return;
        }

        for (size_t i = 0; i < blockSize; ++i) {
            float currentPos = position.next();
            float currentMorph = morph.next();
            float currentFreq = frequency.next();

            // Determine which wavetables to use
            float tableIdxFloat = currentPos * (wavetables.size() - 1);
            int tableIdx1 = static_cast<int>(tableIdxFloat);
            int tableIdx2 = std::min(tableIdx1 + 1, (int)wavetables.size() - 1);
            float tableFrac = tableIdxFloat - tableIdx1;

            // Get pointers to the tables
            const auto& table1 = wavetables[tableIdx1];
            const auto& table2 = wavetables[tableIdx2];

            // Calculate read position in the wavetable
            float phaseInc = WAVETABLE_SIZE * currentFreq / sampleRate;
            phase += phaseInc;
            if (phase >= WAVETABLE_SIZE) phase -= WAVETABLE_SIZE;

            // Linear interpolation within each wavetable
            int sampleIdx1 = static_cast<int>(phase);
            int sampleIdx2 = (sampleIdx1 + 1) % WAVETABLE_SIZE;
            float sampleFrac = phase - sampleIdx1;

            float val1 = table1[sampleIdx1] + sampleFrac * (table1[sampleIdx2] - table1[sampleIdx1]);
            float val2 = table2[sampleIdx1] + sampleFrac * (table2[sampleIdx2] - table2[sampleIdx1]);

            // Linear interpolation between the two wavetables and apply gain
            buffer[i] = (val1 + tableFrac * (val2 - val1)) * gain;
        }
    }


private:
    std::vector<std::vector<float>> wavetables;
    SmoothedParameter frequency;
    SmoothedParameter position;
    SmoothedParameter morph;
    float gain;
    float phase;
    float sampleRate;
};

#endif // WAVETABLE_SYNTH_H
