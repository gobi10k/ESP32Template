// ModulationEngine.h
#ifndef MODULATION_ENGINE_H
#define MODULATION_ENGINE_H

#include <vector>
#include <cmath>
#include <map>
#include <algorithm>

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
    void removeSource(ModulationSource* src);
    void addRoute(ModulationSource* src, float* targetParam, float depth);
    void removeRoute(ModulationSource* src, float* targetParam);
    void clearRoutes();
    void update(float dt);

private:
    std::vector<ModulationSource*> sources;
    std::vector<ModulationRoute> routes;
    std::map<float*, float> baseValues;
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

void ModulationEngine::removeSource(ModulationSource* src) {
    sources.erase(std::remove(sources.begin(), sources.end(), src), sources.end());
}

void ModulationEngine::addRoute(ModulationSource* src, float* targetParam, float depth) {
    // Store the base value if it's not already tracked
    if (baseValues.find(targetParam) == baseValues.end()) {
        baseValues[targetParam] = *targetParam;
    }
    routes.push_back({src, depth, targetParam});
}

void ModulationEngine::removeRoute(ModulationSource* src, float* targetParam) {
    routes.erase(std::remove_if(routes.begin(), routes.end(),
        [this, src, targetParam](const ModulationRoute& route) {
            if (route.source == src && route.targetParam == targetParam) {
                // If this is the last route for this param, remove from baseValues
                bool isLastRoute = true;
                for (const auto& otherRoute : routes) {
                    if (&otherRoute != &route && otherRoute.targetParam == targetParam) {
                        isLastRoute = false;
                        break;
                    }
                }
                if (isLastRoute) {
                    baseValues.erase(targetParam);
                }
                return true;
            }
            return false;
        }), routes.end());
}

void ModulationEngine::clearRoutes() {
    routes.clear();
    baseValues.clear();
}

void ModulationEngine::update(float dt) {
    // 1. Update all modulation sources
    for (auto* src : sources) {
        src->update(dt);
    }

    // Reset all modulated parameters to their base values
    for (auto const& [param, baseValue] : baseValues) {
        if(param) {
            *param = baseValue;
        }
    }

    // 2. Apply modulation (additive instead of replacement)
    for (auto& route : routes) {
        if (route.source && route.targetParam) {
            *route.targetParam += route.source->getValue() * route.depth;
        }
    }
}

#endif // MODULATION_ENGINE_H