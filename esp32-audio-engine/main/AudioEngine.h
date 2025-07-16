// AudioEngine.h
#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <cstddef>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cmath>
#include "ModulationEngine.h"

#define BLOCK_SIZE 64

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
                                    active(false), peakLevel(0.0f), rmsLevel(0.0f) {}

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

    // Update modulation engine
    modEngine.update((float)BLOCK_SIZE / sampleRate);

    float mixBuffer[BLOCK_SIZE] = {0.0f};

    // Render sources
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (auto source : sources) {
        float sourceBuffer[BLOCK_SIZE];
        source->renderBlock(sourceBuffer, BLOCK_SIZE);
        for (size_t i = 0; i < BLOCK_SIZE; i++) {
          mixBuffer[i] += sourceBuffer[i];
        }
      }
    }

    // Apply effects
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (auto effect : effects) {
        effect->processBlock(mixBuffer, BLOCK_SIZE);
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
  }

  void addSource(AudioSource* src) {
    std::lock_guard<std::mutex> lock(mutex);
    src->setSampleRate(sampleRate);
    sources.push_back(src);
  }

  void removeSource(AudioSource* src) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = std::find(sources.begin(), sources.end(), src);
    if (it != sources.end()) {
      sources.erase(it);
    }
  }

  void addEffect(AudioEffect* fx) {
    std::lock_guard<std::mutex> lock(mutex);
    fx->setSampleRate(sampleRate);
    effects.push_back(fx);
  }

  void removeEffect(AudioEffect* fx) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = std::find(effects.begin(), effects.end(), fx);
    if (it != effects.end()) {
      effects.erase(it);
    }
  }

  void setSampleRate(float sr) {
    std::lock_guard<std::mutex> lock(mutex);
    sampleRate = sr;
    for (auto source : sources) source->setSampleRate(sr);
    for (auto effect : effects) effect->setSampleRate(sr);
  }

  float getPeakLevel() const { return peakLevel; }
  float getRMSLevel() const { return rmsLevel; }

private:
  AudioOutput& output;
  std::vector<AudioSource*> sources;
  std::vector<AudioEffect*> effects;
  ModulationEngine modEngine;
  std::mutex mutex;
  float sampleRate;
  bool active;
  float peakLevel;
  float rmsLevel;
};

#endif