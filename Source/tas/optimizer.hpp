#pragma once

#include <cstdint>

extern cvar_t tas_optimizer;
extern cvar_t tas_optimizer_goal;
struct TASScript;

namespace TASQuake {
    void RunOptimizer();
    const char* OptimizerGoalStr();
    double OriginalEfficacy();
    double OptimizedEfficacy();
    std::size_t OptimizerIterations();
    const TASScript* GetOptimizedVersion();
}
