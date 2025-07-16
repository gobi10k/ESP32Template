// SmoothedParameter.h
#ifndef SMOOTHED_PARAMETER_H
#define SMOOTHED_PARAMETER_H

#include <cmath>

class SmoothedParameter {
public:
  SmoothedParameter(float value = 0.0f, float smoothingTime = 0.05f)
      : current(value), target(value), smoothingFactor(0.0f), 
        smoothingTime(smoothingTime), sampleRate(44100.0f) {
    setSmoothingTime(smoothingTime);
  }

  void setSampleRate(float sr) {
    sampleRate = sr;
    setSmoothingTime(smoothingTime);
  }

  void setSmoothingTime(float timeInSeconds) {
    smoothingTime = timeInSeconds;
    if (sampleRate > 0 && timeInSeconds > 0) {
      smoothingFactor = 1.0f - expf(-1.0f / (smoothingTime * sampleRate));
    } else {
      smoothingFactor = 1.0f; // Immediate change
    }
  }

  void setTarget(float newTarget) {
    target = newTarget;
  }

  void setTargetImmediate(float newTarget) {
    target = newTarget;
    current = newTarget;
  }

  float next() {
    if (current != target) {
      current += (target - current) * smoothingFactor;
      // Snap to target when close enough to prevent denormals
      if (fabsf(target - current) < 1e-6f) {
        current = target;
      }
    }
    return current;
  }

  float getCurrent() const {
    return current;
  }

  float getTarget() const {
    return target;
  }

  float* getPtr() {
      return &current;
  }

private:
  float current;
  float target;
  float smoothingFactor;
  float smoothingTime;
  float sampleRate;
};

#endif