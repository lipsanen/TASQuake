#include "draw.hpp"
#include "libtasquake/draw.hpp"
#include "libtasquake/optimizer.hpp"
#include "ipc2.hpp"
#include "optimizer.hpp"
#include "savestate.hpp"
#include "simulate.hpp"

using namespace TASQuake;

cvar_t tas_optimizer {"tas_optimizer", "1"};
cvar_t tas_optimizer_goal {"tas_optimizer_goal", "0"};
static bool m_bFirstIteration = false;
static int startFrame = -1;
static size_t m_uOptIterations = 0;
static double m_dOriginalEfficacy = 0;
static double m_dBestEfficacy = 0;
static TASQuake::Optimizer opt;
static double last_updated = 0;
static TASQuake::OptimizerState state = TASQuake::OptimizerState::Stop;
static TASQuake::OptimizerSettings settings;
static Simulator sim;
static double last_sim_time = 0;
static std::vector<PathPoint> m_BestPoints;
static std::vector<PathPoint> m_CurrentPoints;
static std::vector<Rect> m_BestRects;


const char* TASQuake::OptimizerGoalStr() {
    switch(opt.m_settings.m_Goal) {
        case OptimizerGoal::NegX:
            return "-X";
        case OptimizerGoal::NegY:
            return "-Y";
        case OptimizerGoal::PlusX:
            return "+X";
        case OptimizerGoal::PlusY:
            return "+Y";
        default:
            return "Undetermined";
    }
}

double TASQuake::OriginalEfficacy() {
    return m_dOriginalEfficacy;
}

double TASQuake::OptimizedEfficacy() {
    return m_dBestEfficacy;
}

size_t TASQuake::OptimizerIterations() {
    return m_uOptIterations;
}

const TASScript* TASQuake::GetOptimizedVersion() {
    return &opt.m_currentBest.playbackInfo.current_script;
}

static TASQuake::OptimizerSettings GetSettings() {
    TASQuake::OptimizerSettings settings;
    settings.m_Goal = (TASQuake::OptimizerGoal)tas_optimizer_goal.value;
    settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::RNGBlockMover()));
    settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::RNGStrafer()));
    settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::StrafeAdjuster()));
    settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::FrameBlockMover()));
    settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::TurnOptimizer()));
    return settings;
}

static Simulator GetOptSimulator(TASQuake::Optimizer* opt)
{
	Simulator sim;
	sim.playback = &opt->m_currentRun.playbackInfo;
	sim.info = Get_Sim_Info();
	sim.info.vars.simulated = true;
	sim.frame = 0;

	return sim;
}

static double get_vel_theta_player() {
    if(!sv_player) {
        return 0;
    }

    double vel2d = std::sqrt(sv_player->v.velocity[0] * sv_player->v.velocity[0] + sv_player->v.velocity[1] * sv_player->v.velocity[1]);

    if(!IsZero(vel2d))
		return NormalizeRad(std::atan2(sv_player->v.velocity[1], sv_player->v.velocity[0]));
	else
	{
		double vel_theta = NormalizeRad(tas_strafe_yaw.value * M_DEG2RAD);
		// there's still some issues with prediction so enjoy this lovely hack that fixes some issues with basically 0 cost
		int places = 10000;
		return static_cast<int>(vel_theta * places) / static_cast<double>(places);
	}
}

static double get_vel_theta(const SimulationInfo& info) {
    double vel2d = std::sqrt(info.ent.v.velocity[0] * info.ent.v.velocity[0] + info.ent.v.velocity[1] * info.ent.v.velocity[1]);

    if (!IsZero(vel2d))
		return NormalizeRad(std::atan2(info.ent.v.velocity[1], info.ent.v.velocity[0]));
	else
	{
		double vel_theta = NormalizeRad(info.vars.tas_strafe_yaw * M_DEG2RAD);
		// there's still some issues with prediction so enjoy this lovely hack that fixes some issues with basically 0 cost
		int places = 10000;
		return static_cast<int>(vel_theta * places) / static_cast<double>(places);
	}
}

static void InitOptimizer(PlaybackInfo* playback) {
    startFrame = playback->current_frame;
    m_bFirstIteration = true;
    m_uOptIterations = 0;
    last_updated = playback->last_edited;
    settings = GetSettings();
    if(!opt.Init(playback, &settings)) {
        return;
    }
    sim = GetOptSimulator(&opt);
    state = TASQuake::OptimizerState::ContinueIteration;
    m_CurrentPoints.clear();
    m_BestPoints.clear();
}

static void InitNewIteration() {
    if(m_bFirstIteration) {
        m_dOriginalEfficacy = opt.m_currentBest.RunEfficacy();
        m_dBestEfficacy = m_dOriginalEfficacy;
        m_bFirstIteration = false;
        m_BestPoints = m_CurrentPoints;
        AddCurve(&m_BestPoints, OPTIMIZER_ID);
    } else {
        double runEfficacy = opt.m_currentRun.RunEfficacy();
        if(runEfficacy > m_dBestEfficacy) {
            m_BestPoints = m_CurrentPoints;
            m_dBestEfficacy = runEfficacy;
        }
    }
    opt.ResetIteration();
    sim = GetOptSimulator(&opt);
    ++m_uOptIterations;
    m_CurrentPoints.clear();
}

static bool game_opt_running = false;
static const std::array<float, 4> color = { 0, 1, 1, 0.5 };

void TASQuake::RunOptimizer(bool canPredict)
{
	if (!canPredict || tas_optimizer.value == 0 || game_opt_running)
	{
        if(startFrame != -1) {
            startFrame = -1;
            RemoveCurve(OPTIMIZER_ID);
        }
		return;
	}

	auto playback = GetPlaybackInfo();
	double currentTime = Sys_DoubleTime();
	if (last_updated < playback->last_edited || startFrame != playback->current_frame)
	{
        InitOptimizer(playback);
	}

	double realTimeStart = Sys_DoubleTime();

	while (Sys_DoubleTime() - realTimeStart < tas_predict_per_frame.value && state != TASQuake::OptimizerState::Stop) {
        if(state == TASQuake::OptimizerState::NewIteration) {
            InitNewIteration();
        }

        sim.RunFrame();
		TASQuake::FrameData data;
        data.m_dVelTheta = get_vel_theta(sim.info);
        data.pos.x = sim.info.ent.v.origin[0];
        data.pos.y = sim.info.ent.v.origin[1];
        data.pos.z = sim.info.ent.v.origin[2];

        state = opt.OnRunnerFrame(&data);
        PathPoint p;
        p.color = color;
        VectorCopy(sim.info.ent.v.origin, p.point);
        m_CurrentPoints.push_back(p);
	}

	return;
}

static int game_opt_start_frame = 0;
static int game_opt_end_frame = 0;

void TASQuake::GameOpt_InitOptimizer(int start_frame, int end_frame) {
    start_frame = std::max(1, start_frame);
    end_frame = std::max(start_frame+1, end_frame);

    tas_optimizer.value = 0; // Disable normal optimizer
    auto info = GetPlaybackInfo();
    info->current_script.RemoveBlocksAfterFrame(end_frame);
    info->current_frame = start_frame;

    game_opt_start_frame = start_frame;
    game_opt_end_frame = end_frame;
    m_bFirstIteration = true;
    m_uOptIterations = 0;
    settings = GetSettings();
    settings.m_iEndOffset = end_frame - info->Get_Last_Frame();

    if(!opt.Init(info, &settings)) {
        return;
    }

    state = TASQuake::OptimizerState::ContinueIteration;
    m_CurrentPoints.clear();
    m_BestPoints.clear();
    Savestate_Script_Updated(game_opt_end_frame);
    Run_Script(game_opt_end_frame+1, true);
    game_opt_running = true;
}

void TASQuake::Cmd_TAS_Optimizer_Run() {
	if (Cmd_Argc() <= 2) {
        Con_Print("Usage: tas_optimizer_run <start> <end>\n");
        return;
    }

    int start = atoi(Cmd_Argv(1));
    int end = atoi(Cmd_Argv(2));
    TASQuake::GameOpt_InitOptimizer(start, end);
}

static void GameOpt_NewIteration() {
    auto info = GetPlaybackInfo();
    info->current_script.AddScript(&opt.m_currentRun.playbackInfo.current_script, game_opt_start_frame);

    if(m_bFirstIteration) {
        m_dOriginalEfficacy = opt.m_currentBest.RunEfficacy();
        m_dBestEfficacy = m_dOriginalEfficacy;
        m_bFirstIteration = false;
        m_BestPoints = m_CurrentPoints;
        AddCurve(&m_BestPoints, OPTIMIZER_ID);
    } else {
        double runEfficacy = opt.m_currentRun.RunEfficacy();
        if(runEfficacy > m_dBestEfficacy) {
            TASQuake::CL_SendRun(opt.m_currentRun); // Send run over IPC if connected to server
            m_BestPoints = m_CurrentPoints;
            m_dBestEfficacy = runEfficacy;
        }
    }
    opt.ResetIteration();
    ++m_uOptIterations;
    m_CurrentPoints.clear();
    Savestate_Script_Updated(game_opt_start_frame);
    Run_Script(game_opt_end_frame+1, true);
    state = TASQuake::OptimizerState::ContinueIteration;
}

static void Game_Opt_Add_FrameData(int current_frame) {
    TASQuake::FrameData data;
    data.m_dVelTheta = get_vel_theta_player();

    if(sv_player) {
        data.pos.x = sv_player->v.origin[0];
        data.pos.y = sv_player->v.origin[1];
        data.pos.z = sv_player->v.origin[2];
    }

    state = opt.OnRunnerFrame(&data);
    PathPoint p;
    p.color = color;
    if(sv_player)
        VectorCopy(sv_player->v.origin, p.point);
    else
        VectorCopy(vec3_origin, p.point);
    m_CurrentPoints.push_back(p);
}

static void Game_Opt_Ended() {
    if(state == TASQuake::OptimizerState::NewIteration) {
        TASQuake::CL_SendOptimizerProgress(m_uOptIterations + 1);
        GameOpt_NewIteration();
    } else if(state == TASQuake::OptimizerState::Stop) {
        Con_Printf("Optimizer ran to completion\n");
        game_opt_running = false;
        Run_Script(game_opt_start_frame, true);
    } else {
        Con_Printf("Optimizer ended with invalid state\n");
        game_opt_running = false;
    }
}

void TASQuake::Optimizer_Frame_Hook() {
    if(!game_opt_running)
        return;
    
    auto info = GetPlaybackInfo();
    if(tas_gamestate == unpaused) {
        int current_frame = info->current_frame;
        if(current_frame >= game_opt_start_frame && current_frame <= game_opt_end_frame) {
            Game_Opt_Add_FrameData(current_frame);
        } 

        if(state == TASQuake::OptimizerState::NewIteration) {
            Game_Opt_Ended();   
        }
    } 
    else if(tas_gamestate == paused) {
        Game_Opt_Ended();
    }
}

