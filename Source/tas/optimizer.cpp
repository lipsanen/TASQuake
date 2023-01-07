#include "draw.hpp"
#include "libtasquake/optimizer.hpp"
#include "optimizer.hpp"
#include "simulate.hpp"

cvar_t tas_optimizer {"tas_optimizer", "1"};
cvar_t tas_optimizer_goal {"tas_optimizer_goal", "0"};
static bool m_bFirstIteration = false;
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
        m_dOriginalEfficacy = opt.m_currentBest.RunEfficacy(opt.m_settings.m_Goal);
        m_dBestEfficacy = m_dOriginalEfficacy;
        m_bFirstIteration = false;
        m_BestPoints = m_CurrentPoints;
        AddCurve(&m_BestPoints, OPTIMIZER_ID);
    } else {
        double runEfficacy = opt.m_currentRun.RunEfficacy(opt.m_settings.m_Goal);
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

void TASQuake::RunOptimizer()
{
	if (tas_optimizer.value == 0)
	{
		return;
	}

	auto playback = GetPlaybackInfo();
	double currentTime = Sys_DoubleTime();
	if (last_updated < playback->last_edited)
	{
        InitOptimizer(playback);
	}

	double realTimeStart = Sys_DoubleTime();
	const std::array<float, 4> color = { 0, 1, 1, 0.5 };

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
