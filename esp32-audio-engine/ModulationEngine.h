// ModulationEngine.h
#ifndef MODULATION_ENGINE_H
#define MODULATION_ENGINE_H

#include <vector>
#include <cmath>

// Forward declarations
class ModulationSource;

struct ModulationRoute {
    ModulationSource* source;
    float depth;
    float* targetParam;
};

class ModulationEngine {
public:
    void addSource(ModulationSource* src);
    void addRoute(ModulationSource* src, float* targetParam, float depth);
    void update(float dt);

private:
    std::vector<ModulationSource*> sources;
    std::vector<ModulationRoute> routes;
};

class ModulationSource {
public:
    virtual ~ModulationSource() {}
    virtual void update(float dt) = 0;
    virtual float getValue() const = 0;
};

// --- Implementation ---

void ModulationEngine::addSource(ModulationSource* src) {
    sources.push_back(src);
}

void ModulationEngine::addRoute(ModulationSource* src, float* targetParam, float depth) {
    routes.push_back({src, depth, targetParam});
}

void ModulationEngine::update(float dt) {
    // 1. Update all modulation sources
    for (auto* src : sources) {
        src->update(dt);
    }

    // 2. Apply modulation (replace target value completely)
    for (auto& route : routes) {
        if (route.source && route.targetParam) {
            *route.targetParam = route.source->getValue() * route.depth;
        }
    }
}

#endif // MODULATION_ENGINE_H