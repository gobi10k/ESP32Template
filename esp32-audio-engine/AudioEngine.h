// AudioEngine.h
#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <cstddef>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cmath>
#include "ModulationEngine.h"

#if defined(ESP32)
#include <xtensa/hal.h>
#define GET_CYCLE_COUNT() xthal_get_ccount()
#else
#define GET_CYCLE_COUNT() 0
#endif

#define BLOCK_SIZE 64
#define CYCLES_PER_SAMPLE (F_CPU / 44100.0f)

class AudioSource {
public:
  virtual ~AudioSource() {}
  virtual void renderBlock(float* buffer, size_t blockSize) = 0;
  virtual void setSampleRate(float sampleRate) = 0;
};

class AudioEffect {
public:
  virtual ~AudioEffect() {}
  virtual void processBlock(float* buffer, size_t blockSize) = 0;
  virtual void setSampleRate(float sampleRate) = 0;
};

class AudioOutput {
public:
    virtual ~AudioOutput() {}
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void writeBlock(float* buffer) = 0;
};

class AudioEngine {
public:
  AudioEngine(AudioOutput& output) : output(output), sampleRate(44100.0f),
                                    active(false), peakLevel(0.0f), rmsLevel(0.0f), cpuUsage(0.0f) {}

  ModulationEngine& getModulationEngine() { return modEngine; }

  void start() {
    std::lock_guard<std::mutex> lock(mutex);
    output.start();
    active = true;
    peakLevel = 0.0f;
    rmsLevel = 0.0f;
  }

  void stop() {
    std::lock_guard<std::mutex> lock(mutex);
    active = false;
    output.stop();
  }

  void renderBlock() {
    if (!active) return;

    uint32_t start = GET_CYCLE_COUNT();

    // Update modulation engine
    modEngine.update((float)BLOCK_SIZE / sampleRate);

    float mixBuffer[BLOCK_SIZE] = {0.0f};

    // Render sources
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (size_t i = 0; i < numSources; ++i) {
        float sourceBuffer[BLOCK_SIZE];
        sources[i]->renderBlock(sourceBuffer, BLOCK_SIZE);
        for (size_t j = 0; j < BLOCK_SIZE; j++) {
          mixBuffer[j] += sourceBuffer[j];
        }
      }
    }

    // Apply effects
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (size_t i = 0; i < numEffects; ++i) {
        effects[i]->processBlock(mixBuffer, BLOCK_SIZE);
      }
    }

    // Calculate statistics
    float peak = 0.0f;
    float sumSquares = 0.0f;
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
      float absVal = fabsf(mixBuffer[i]);
      if (absVal > peak) peak = absVal;
      sumSquares += mixBuffer[i] * mixBuffer[i];
    }
    peakLevel = peak;
    rmsLevel = sqrtf(sumSquares / BLOCK_SIZE);

    // Write to output
    output.writeBlock(mixBuffer);

    uint32_t end = GET_CYCLE_COUNT();
    cpuUsage = (end - start) / (BLOCK_SIZE * CYCLES_PER_SAMPLE);
  }

  void addSource(AudioSource* src) {
    std::lock_guard<std::mutex> lock(mutex);
    if (numSources < MAX_SOURCES) {
        src->setSampleRate(sampleRate);
        sources[numSources++] = src;
    }
  }

  void removeSource(AudioSource* src) {
    std::lock_guard<std::mutex> lock(mutex);
    for (size_t i = 0; i < numSources; ++i) {
        if (sources[i] == src) {
            // Shift remaining elements down
            for (size_t j = i; j < numSources - 1; ++j) {
                sources[j] = sources[j + 1];
            }
            numSources--;
            return;
        }
    }
  }

  void addEffect(AudioEffect* fx) {
    std::lock_guard<std::mutex> lock(mutex);
    if (numEffects < MAX_EFFECTS) {
        fx->setSampleRate(sampleRate);
        effects[numEffects++] = fx;
    }
  }

  void removeEffect(AudioEffect* fx) {
    std::lock_guard<std::mutex> lock(mutex);
    for (size_t i = 0; i < numEffects; ++i) {
        if (effects[i] == fx) {
            // Shift remaining elements down
            for (size_t j = i; j < numEffects - 1; ++j) {
                effects[j] = effects[j + 1];
            }
            numEffects--;
            return;
        }
    }
  }

  void setSampleRate(float sr) {
    std::lock_guard<std::mutex> lock(mutex);
    sampleRate = sr;
    for (size_t i = 0; i < numSources; ++i) {
        sources[i]->setSampleRate(sr);
    }
    for (size_t i = 0; i < numEffects; ++i) {
        effects[i]->setSampleRate(sr);
    }
  }

  float getPeakLevel() const { return peakLevel; }
  float getRMSLevel() const { return rmsLevel; }
  float getCpuUsage() const { return cpuUsage; }

private:
    // Replace vectors with fixed-size arrays if you have a known maximum
    static constexpr size_t MAX_SOURCES = 8;
    static constexpr size_t MAX_EFFECTS = 4;

    AudioSource* sources[MAX_SOURCES];
    size_t numSources = 0;

    AudioEffect* effects[MAX_EFFECTS];
    size_t numEffects = 0;

  AudioOutput& output;
  ModulationEngine modEngine;
  std::mutex mutex;
  float sampleRate;
  bool active;
  float peakLevel;
  float rmsLevel;
  float cpuUsage;
};

#endif