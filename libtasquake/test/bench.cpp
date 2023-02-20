#include <cstdio>
#include <thread>

#include "bench.hpp"

#include "catch_amalgamated.hpp"
#include "libtasquake/utils.hpp"

using namespace TASQuake;

void Player::Reset()
{
	m_bStrafing = false;
	m_fStrafeYaw = 0.0f;
	m_vecPos.x = m_vecPos.y = m_vecPos.z = 0.0f;
}

struct BenchResult
{
	int bestIterations = 0;
	double best = std::numeric_limits<double>::lowest();
};

static BenchResult Bench(SimFunc func,
                         const OptimizerSettings* settings,
                         const PlaybackInfo* playback)
{
	BenchResult result;
	Optimizer opt;
	opt.Seed(rand());
	opt.Init(playback, settings);

	int frame = 0;
	Player player;
	TASQuake::ExtendedFrameData data;
	size_t iterations = 0;

	while(true)
	{
		auto block = opt.GetCurrentFrameBlock();

		if (block && block->frame == frame)
		{
			if (block->HasConvar("tas_strafe_yaw"))
			{
				player.m_fStrafeYaw = block->convars.at("tas_strafe_yaw");
			}
			if (block->HasConvar("tas_strafe"))
			{
				player.m_bStrafing = block->convars.at("tas_strafe") != 0;
			}
		}

		func(&player);
		data.m_frameData.pos = player.m_vecPos;

		auto state = opt.OnRunnerFrame(&data);

		if (state == TASQuake::OptimizerState::Stop)
		{
			break;
		}
		else if (state == TASQuake::OptimizerState::NewIteration)
		{
			double efficacy = opt.m_currentRun.RunEfficacy();
			if (efficacy > result.best)
			{
				result.best = efficacy;
				result.bestIterations = iterations;
			}

			++iterations;
			frame = 0;
			player.Reset();
			opt.ResetIteration();
		}
		else
		{
			++frame;
		}
	}

	return result;
}

void TASQuake::BenchTest(SimFunc func,
                         const OptimizerSettings* settings,
                         const PlaybackInfo* playback,
						 const int iterations)
{
	double min = std::numeric_limits<double>::max();
	double max = std::numeric_limits<double>::lowest();
	double avg = 0;
	double avgIterations = 0;

	for(size_t i=0; i < iterations; ++i)
	{
		auto result = Bench(func, settings, playback);
		min = std::min(result.best, min);
		max = std::max(result.best, max);
		avg += result.best;
		avgIterations += result.bestIterations;
	}

	avg /= iterations;
	avgIterations /= iterations;
	std::printf("Min: %f, Max %f, Avg %f, AvgIt %f\n", min, max, avg, avgIterations);
}

void TASQuake::MemorylessSim(Player* player)
{
	if (!player->m_bStrafing)
	{
		return;
	}

	double theta = player->m_fStrafeYaw * M_DEG2RAD;
	player->m_vecPos.x += std::cos(theta);
	player->m_vecPos.y += std::sin(theta);
}
