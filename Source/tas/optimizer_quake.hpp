#pragma once

#include <cstdint>
#include "libtasquake/boost_ipc.hpp"
#include "libtasquake/optimizer.hpp"

extern cvar_t tas_optimizer;
extern cvar_t tas_optimizer_minthp;
extern cvar_t tas_optimizer_algs;
extern cvar_t tas_optimizer_casper;
extern cvar_t tas_optimizer_goal;
extern cvar_t tas_optimizer_multigame;
extern cvar_t tas_optimizer_secondarygoals;
struct TASScript;

namespace TASQuake {
    void RunOptimizer(bool canPredict);
    const char* OptimizerGoalStr();
    double OriginalEfficacy();
    double OptimizedEfficacy();
    std::size_t OptimizerIterations();
    const TASScript* GetOptimizedVersion();
    void Optimizer_Frame_Hook();
    void GameOpt_InitOptimizer(int32_t start_frame, int32_t end_frame, int32_t identifier, const OptimizerSettings& settings);
    void Cmd_TAS_Optimizer_Run();
    void Receive_Optimizer_Task(const ipc::Message& msg);
    void Receive_Optimizer_Run(const ipc::Message& msg);
    void Receive_Optimizer_Stop();
    void MultiGame_ReceiveRun(const ipc::Message& msg);
    void MultiGame_ReceiveProgress(const ipc::Message& msg);
    void MultiGame_ReceiveGoal(const ipc::Message& msg);
    void SV_StopMultiGameOpt();
    void Get_Prediction_Frames(int32_t& start_frame, int32_t& end_frame);
}
