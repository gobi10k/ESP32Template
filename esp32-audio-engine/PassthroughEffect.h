// PassthroughEffect.h
#ifndef PASSTHROUGH_EFFECT_H
#define PASSTHROUGH_EFFECT_H

#include "AudioEngine.h"

class PassthroughEffect : public AudioEffect {
public:
  void processBlock(float* buffer, size_t blockSize) override {
    // Optional: Add clipping protection here instead of in engine
    // for (size_t i = 0; i < blockSize; i++) {
    //   buffer[i] = std::max(-1.0f, std::min(1.0f, buffer[i]));
    // }
  }

  void setSampleRate(float /*sampleRate*/) override {}
};

#endif