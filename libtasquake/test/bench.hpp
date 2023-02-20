#pragma once
#include "libtasquake/optimizer.hpp"
#include "libtasquake/script_playback.hpp"
#include <functional>

namespace TASQuake
{
	struct Player
	{
		TASQuake::Vector m_vecPos;
		float m_fStrafeYaw = 0.0f;
		bool m_bStrafing = false;
		void Reset();
	};

	typedef std::function<void(Player* player)> SimFunc;
	void BenchTest(SimFunc func, const OptimizerSettings* settings, const PlaybackInfo* playback, const int iterations=16);
  	void MemorylessSim(Player* player);

} // namespace TASQuake
