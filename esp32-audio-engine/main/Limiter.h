// Limiter.h
#ifndef LIMITER_H
#define LIMITER_H

#include "AudioEngine.h"
#include <cmath>

class Limiter : public AudioEffect {
public:
  Limiter(float threshold = 0.95f, float releaseTime = 0.01f) 
      : threshold(threshold), releaseTime(releaseTime), envelope(0.0f), sampleRate(44100.0f) {
    setReleaseTime(releaseTime);
  }

  void setSampleRate(float sr) override {
    sampleRate = sr;
    setReleaseTime(releaseTime);
  }

  void setReleaseTime(float timeInSeconds) {
    releaseTime = timeInSeconds;
    releaseCoeff = expf(-1.0f / (releaseTime * sampleRate));
  }

  void processBlock(float* buffer, size_t blockSize) override {
    for (size_t i = 0; i < blockSize; i++) {
      float level = fabsf(buffer[i]);
      
      // Envelope follower with different attack/release
      if (level > envelope) {
        // Fast attack
        envelope = level;
      } else {
        // Smooth release
        envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * level;
      }

      // Apply limiting if above threshold
      if (envelope > threshold) {
        buffer[i] = buffer[i] * (threshold / envelope);
      }
    }
  }

private:
  float threshold;
  float releaseTime;
  float releaseCoeff;
  float envelope;
  float sampleRate;
};

#endif