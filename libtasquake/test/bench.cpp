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

void TASQuake::BenchTest(SimFunc func,
                         const OptimizerSettings* settings,
                         const PlaybackInfo* playback,
                         double passThreshold)
{
	Optimizer opt;
	opt.Seed(rand());
	opt.Init(playback, settings);

	double currentBest = passThreshold - 1;
	int frame = 0;
	Player player;
	TASQuake::FrameData data;
	size_t iterations = 0;

	while (currentBest < passThreshold)
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
		data.pos = player.m_vecPos;

		auto state = opt.OnRunnerFrame(&data);

		if (state == TASQuake::OptimizerState::Stop)
		{
			break;
		}
		else if (state == TASQuake::OptimizerState::NewIteration)
		{
			++iterations;
			frame = 0;
			currentBest = opt.m_currentBest.RunEfficacy(opt.m_settings.m_Goal);
			if (currentBest >= passThreshold)
			{
				break;
			}
			player.Reset();
			opt.ResetIteration();

			// The simulation is expected to take longer than the lib code in a real world case
			// Sleep a bit to simulate this
			std::this_thread::sleep_for(std::chrono::microseconds(5));
		}
		else
		{
			++frame;
		}
	}

	double result = opt.m_currentBest.RunEfficacy(opt.m_settings.m_Goal);

	if (result < passThreshold)
	{
		std::fprintf(stderr, "Did not match pass threshold %f < %f\n", result, passThreshold);
	}
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
