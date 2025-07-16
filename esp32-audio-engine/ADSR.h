// ADSR.h
#ifndef ADSR_H
#define ADSR_H

#include "ModulationEngine.h"

class ADSR : public ModulationSource {
public:
    enum State {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    ADSR() : state(IDLE), value(0.0f), attackTime(0.1f), decayTime(0.1f), 
         sustainLevel(0.7f), releaseTime(0.5f), sampleRate(44100.0f) {
    // Force initial silent state
    value = 0.0f;
    state = IDLE;
}

    void setAttack(float time) { attackTime = time; }
    void setDecay(float time) { decayTime = time; }
    void setSustain(float level) { sustainLevel = level; }
    void setRelease(float time) { releaseTime = time; }

    void noteOn() {
        if (state == IDLE || state == RELEASE) {
            state = ATTACK;
        }
    }

    void noteOff() {
        state = RELEASE;
    }

    void setSampleRate(float sr) {
        sampleRate = sr;
    }

    void update(float dt) override {
        switch (state) {
            case ATTACK: {
                float progress = value / 1.0f;
                float curveFactor = 1.0f + (attackCurve * 3.0f * (1.0f - progress));
                value += (dt / attackTime) * curveFactor;
                if (value >= 1.0f) {
                    value = 1.0f;
                    state = DECAY;
                }
                break;
            }
            case DECAY: {
                float progress = (value - sustainLevel) / (1.0f - sustainLevel);
                float curveFactor = 1.0f + (decayCurve * 3.0f * progress);
                value -= (dt / decayTime) * curveFactor;
                if (value <= sustainLevel) {
                    value = sustainLevel;
                    state = SUSTAIN;
                }
                break;
            }
            case SUSTAIN:
                // Do nothing, value stays at sustainLevel
                break;
            case RELEASE: {
                float progress = value / sustainLevel;
                float curveFactor = 1.0f + (releaseCurve * 3.0f * progress);
                value -= (dt / releaseTime) * curveFactor;
                if (value <= 0.0f) {
                    value = 0.0f;
                    state = IDLE;
                }
                break;
            }
            case IDLE:
                break;
        }
    }

    bool isActive() const {
        return state != IDLE;
    }

    float getValue() const override {
    return (state == IDLE) ? 0.0f : value; // Force 0 when inactive
}

    void setAttackCurve(float curve) { attackCurve = curve; } // 0=linear, 1=exponential
    void setDecayCurve(float curve) { decayCurve = curve; }
    void setReleaseCurve(float curve) { releaseCurve = curve; }

private:
    State state;
    float value;
    float attackTime;
    float decayTime;
    float sustainLevel;
    float releaseTime;
    float sampleRate;
    float attackCurve = 0.0f;
    float decayCurve = 0.0f;
    float releaseCurve = 0.0f;
};

#endif // ADSR_H