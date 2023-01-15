#pragma once

#include <cstdint>

extern cvar_t tas_optimizer;
extern cvar_t tas_optimizer_goal;
struct TASScript;

namespace TASQuake {
    void RunOptimizer(bool canPredict);
    const char* OptimizerGoalStr();
    double OriginalEfficacy();
    double OptimizedEfficacy();
    std::size_t OptimizerIterations();
    const TASScript* GetOptimizedVersion();
    void Optimizer_Frame_Hook();
    void GameOpt_InitOptimizer(int start_frame, int end_frame);
    void Cmd_TAS_Optimizer_Run();
}
