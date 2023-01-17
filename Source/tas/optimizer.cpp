#include "draw.hpp"
#include "libtasquake/draw.hpp"
#include "libtasquake/optimizer.hpp"
#include "ipc_prediction.hpp"
#include "ipc2.hpp"
#include "optimizer.hpp"
#include "savestate.hpp"
#include "simulate.hpp"

using namespace TASQuake;

static qboolean Optimizer_Var_Updated(struct cvar_s *var, char *value);

cvar_t tas_optimizer = {"tas_optimizer", "1"};
cvar_t tas_optimizer_goal = {"tas_optimizer_goal", "0", 0, Optimizer_Var_Updated};
cvar_t tas_optimizer_multigame  = {"tas_optimizer_multigame", "0", 0, Optimizer_Var_Updated};
cvar_t tas_optimizer_algs = {"tas_optimizer_algs", "all", 0, Optimizer_Var_Updated};

static bool m_bFirstIteration = false;
static int startFrame = -1;
static size_t m_uOptIterations = 0;
static double m_dOriginalEfficacy = 0;
static double m_dBestEfficacy = 0;
static TASQuake::Optimizer opt;
static double last_updated = 0;
static TASQuake::OptimizerState state = TASQuake::OptimizerState::Stop;
static Simulator sim;
static std::vector<PathPoint> m_BestPoints;
static std::vector<PathPoint> m_CurrentPoints;
static std::vector<Rect> m_BestRects;
static bool multi_game_opt_running = false;
static std::map<size_t, int32_t> m_uIterationCounts; // Stores the iteration counts from clients
static int32_t multi_game_opt_num = 1;

static qboolean Optimizer_Var_Updated(struct cvar_s *var, char *value) {
    last_updated = 0;
    if(multi_game_opt_running) {
        TASQuake::SV_StopMultiGameOpt();
    }
    return qfalse;
}

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
    int32_t start, end;
    TASQuake::Get_Prediction_Frames(start, end);
    settings.m_iFrames = end - start;

    std::pair<const char*, TASQuake::AlgorithmEnum> algs[] = { 
        {"rngmove", TASQuake::AlgorithmEnum::RNGBlockMover},
        {"rngstrafe", TASQuake::AlgorithmEnum::RNGStrafer},
        {"adjstrafe", TASQuake::AlgorithmEnum::StrafeAdjuster},
        {"adjblock", TASQuake::AlgorithmEnum::FrameBlockMover},
        {"turn", TASQuake::AlgorithmEnum::TurnOptimizer},
        };

    for(auto algpair : algs) {
        if(strcmp(tas_optimizer_algs.string, "all") == 0 || strstr(tas_optimizer_algs.string, algpair.first) != NULL) {
            settings.m_vecAlgorithmData.push_back(algpair.second);
        }
    }

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
    auto settings = GetSettings();
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

static void SimOptimizer_Frame(bool canPredict)
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

		TASQuake::FrameData data;
        data.m_dVelTheta = get_vel_theta(sim.info);
        data.pos.x = sim.info.ent.v.origin[0];
        data.pos.y = sim.info.ent.v.origin[1];
        data.pos.z = sim.info.ent.v.origin[2];

        state = opt.OnRunnerFrame(&data);
        sim.RunFrame();
        PathPoint p;
        p.color = color;
        VectorCopy(sim.info.ent.v.origin, p.point);
        m_CurrentPoints.push_back(p);
	}
}

static void SV_SendBest(const TASQuake::OptimizerRun& run) {
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::OptimizerRun;
    writer.WriteBytes(&type, 1);
    writer.WriteBytes(&multi_game_opt_num, sizeof(multi_game_opt_num));
    run.WriteToBuffer(writer);
    SV_BroadCastMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

void TASQuake::MultiGame_ReceiveRun(const ipc::Message& msg) {
    auto reader = TASQuakeIO::BufferReadInterface::Init((std::uint8_t*)msg.address + 1, msg.length - 1);
    int32_t identifier;
    reader.Read(&identifier, sizeof(identifier));

    if(identifier != multi_game_opt_num) {
        Con_Printf("Result discarded\n");
        return;
    }

    opt.m_currentRun.ReadFromBuffer(reader);
    if(opt.m_currentRun.RunEfficacy() > m_dBestEfficacy || m_bFirstIteration) {
        SV_SendBest(opt.m_currentRun);
        m_dBestEfficacy = opt.m_currentRun.RunEfficacy();
        opt.m_currentBest = opt.m_currentRun;
        m_BestPoints.clear();
        m_BestRects.clear();

        for(auto& data : opt.m_currentRun.m_vecData) {
            PathPoint p;
            p.color = color;
            p.point[0] = data.pos.x;
            p.point[1] = data.pos.y;
            p.point[2] = data.pos.z;
            m_BestPoints.push_back(p);
        }

        if(m_bFirstIteration) {
            m_dOriginalEfficacy = m_dBestEfficacy;
            m_bFirstIteration = false;
            AddCurve(&m_BestPoints, OPTIMIZER_ID);
        }
    }
}

void TASQuake::MultiGame_ReceiveProgress(const ipc::Message& msg) {
    auto reader = TASQuakeIO::BufferReadInterface::Init((std::uint8_t*)msg.address + 1, msg.length - 1);
    int32_t identifier;
    reader.Read(&identifier, sizeof(identifier));

    if(identifier != multi_game_opt_num) {
        Con_Printf("Result discarded\n");
        return;
    }


    int32_t iterations;
    reader.Read(&iterations, sizeof(iterations));
    m_uIterationCounts[msg.connection_id] = iterations;

    // Update the count
    m_uOptIterations = 0;
    for(auto& pair : m_uIterationCounts) {
        m_uOptIterations += pair.second;
    }
}

static void SV_StartMultiGameOpt() {
    ++multi_game_opt_num;
    last_updated = Sys_DoubleTime();
    auto info = GetPlaybackInfo();
    int32_t start_frame, end_frame;
    Get_Prediction_Frames(start_frame, end_frame);
    opt.m_settings = GetSettings();

    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::OptimizerTask;
    writer.WriteBytes(&type, sizeof(type));
    opt.m_settings.WriteToBuffer(writer);
    writer.WriteBytes(&start_frame, sizeof(start_frame));
    writer.WriteBytes(&end_frame, sizeof(end_frame));
    writer.WriteBytes(&multi_game_opt_num, sizeof(multi_game_opt_num));
    info->current_script.Write_To_Memory(writer);
    m_dBestEfficacy = 0;

    SV_BroadCastMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

void TASQuake::Receive_Optimizer_Task(const ipc::Message& msg) {
    auto reader = TASQuakeIO::BufferReadInterface::Init((std::uint8_t*)msg.address + 1, msg.length - 1);
    TASQuake::OptimizerSettings settings;
    int32_t start, end, identifier;
    settings.ReadFromBuffer(reader);
    reader.Read(&start, sizeof(start));
    reader.Read(&end, sizeof(end));
    reader.Read(&identifier, sizeof(identifier));

    auto info = GetPlaybackInfo();
    info->current_script.blocks.clear();
    info->current_script.Load_From_Memory(reader);

    TASQuake::GameOpt_InitOptimizer(start, end, identifier, settings);
}

void TASQuake::Receive_Optimizer_Stop() {
    game_opt_running = false;
}

void TASQuake::SV_StopMultiGameOpt() {
    if(!multi_game_opt_running) {
        return;
    }

    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::OptimizerStop;
    writer.WriteBytes(&type, sizeof(type));
    SV_BroadCastMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
    multi_game_opt_running = false;
}

static void MultiGameOpt_Frame(bool canPredict) {
    if(!canPredict || tas_optimizer.value == 0 || game_opt_running) {

        if(startFrame != -1) {
            startFrame = -1;
            RemoveCurve(OPTIMIZER_ID);
            TASQuake::SV_StopMultiGameOpt();
        }

        return;
    }

    auto info = GetPlaybackInfo();

    if(!multi_game_opt_running || info->last_edited > last_updated) {
        if(!IPC_Prediction_HasLine()) {
            return; // Wait until IPC prediction has finished prior to sending new opt request
        }

        // Start the multi-game process
        RemoveCurve(OPTIMIZER_ID);
        startFrame = info->current_frame;
        multi_game_opt_running = true;

        m_bFirstIteration = true;
        m_uOptIterations = 0;
        last_updated = info->last_edited;

        state = TASQuake::OptimizerState::ContinueIteration;
        m_uIterationCounts.clear();
        m_CurrentPoints.clear();
        m_BestPoints.clear();

        SV_StartMultiGameOpt();
    }
}

void TASQuake::RunOptimizer(bool canPredict)
{
    if(tas_optimizer_multigame.value != 0) {
        MultiGameOpt_Frame(canPredict);
    } else {
        SimOptimizer_Frame(canPredict);
    }
}

static int32_t game_opt_start_frame = 0;
static int32_t game_opt_end_frame = 0;
static int32_t game_opt_identifier = 0;


void TASQuake::Receive_Optimizer_Run(const ipc::Message& msg) {
    auto reader = TASQuakeIO::BufferReadInterface::Init((std::uint8_t*)msg.address + 1, msg.length - 1);
    int32_t identifier;
    reader.Read(&identifier, sizeof(identifier));

    if(identifier != game_opt_identifier) {
        Con_Printf("Result discarded\n");
        return;
    }

    TASQuake::OptimizerRun newRun;
    newRun.ReadFromBuffer(reader);

    if(newRun.IsBetterThan(opt.m_currentBest)) {
        opt.m_currentBest = newRun;
        m_dBestEfficacy = newRun.RunEfficacy();
        Con_Printf("Improvement found by another client\n");
    } else {
        Con_Printf("Received run was worse or equal to ours, discard...\n");
    }
}

static void CL_SendRun(const OptimizerRun& run) {
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::OptimizerRun;
    writer.WriteBytes(&type, 1);
    writer.WriteBytes(&game_opt_identifier, sizeof(game_opt_identifier));
    run.WriteToBuffer(writer);
    TASQuake::CL_SendMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

static void CL_SendOptimizerProgress(int iterations) {
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::OptimizerProgress;
    writer.WriteBytes(&type, 1);
    writer.WriteBytes(&game_opt_identifier, sizeof(game_opt_identifier));
    writer.WriteBytes(&iterations, sizeof(iterations));
    TASQuake::CL_SendMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

// TODO: Clean up this mess and move it inside the optimizer settings
void TASQuake::GameOpt_InitOptimizer(int32_t start_frame, int32_t end_frame, int32_t identifier, const OptimizerSettings& _settings) {
    start_frame = std::max(1, start_frame);
    end_frame = std::max(start_frame+1, end_frame);
    game_opt_identifier = identifier;

    tas_optimizer.value = 0; // Disable normal optimizer
    auto info = GetPlaybackInfo();
    info->current_script.RemoveBlocksAfterFrame(end_frame);
    info->current_frame = start_frame;

    game_opt_start_frame = start_frame;
    game_opt_end_frame = end_frame;
    m_bFirstIteration = true;
    m_uOptIterations = 0;

    if(!opt.Init(info, &_settings)) {
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
    auto settings = GetSettings();
    TASQuake::GameOpt_InitOptimizer(start, end, 0, settings);
}

void TASQuake::MultiGame_ReceiveGoal(const ipc::Message& msg) {
    auto reader = TASQuakeIO::BufferReadInterface::Init((std::uint8_t*)msg.address + 1, msg.length - 1);
    OptimizerGoal goal;
    reader.Read(&goal, sizeof(goal));

    if(goal == OptimizerGoal::Undetermined) {
        Con_Print("Invalid optimizer goal from client\n");
    } else if(opt.m_settings.m_Goal != OptimizerGoal::Undetermined &&
     opt.m_settings.m_Goal != goal) {
        Con_Print("Conflicting optimizer goals received from client.\n");
    }
    opt.m_settings.m_Goal = goal;
}

static void CL_SendGoal() {
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::OptimizerGoal;
    writer.WriteBytes(&type, 1);
    writer.WriteBytes(&opt.m_settings.m_Goal, sizeof(opt.m_settings.m_Goal));
    TASQuake::CL_SendMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

static void GameOpt_NewIteration() {
    auto info = GetPlaybackInfo();
    info->current_script.AddScript(&opt.m_currentRun.playbackInfo.current_script, game_opt_start_frame);

    if(m_bFirstIteration) {
        m_dOriginalEfficacy = opt.m_currentBest.RunEfficacy();
        m_dBestEfficacy = m_dOriginalEfficacy;
        m_bFirstIteration = false;
        m_BestPoints = m_CurrentPoints;
        CL_SendGoal();
        CL_SendRun(opt.m_currentRun);
        AddCurve(&m_BestPoints, OPTIMIZER_ID);
    } else {
        double runEfficacy = opt.m_currentRun.RunEfficacy();
        if(runEfficacy > m_dBestEfficacy) {
            CL_SendRun(opt.m_currentRun); // Send run over IPC if connected to server
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
        CL_SendOptimizerProgress(m_uOptIterations + 1);
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

