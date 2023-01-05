#include "catch_amalgamated.hpp"
#include "bench.hpp"
#include "libtasquake/optimizer.hpp"

TEST_CASE("Stone rolls up slope")
{
    TASQuake::RollingStone stone;
    stone.Init(0, 1, 1, 10);
    for(size_t i=1; stone.ShouldContinue(i); ++i) {
        stone.NextValue();
    }
    REQUIRE(stone.m_dCurrentValue == 10);
}

static void CornerFunc(TASQuake::Player* player) {
    TASQuake::MemorylessSim(player);
    if(player->m_vecPos.x <= 10.0) {
        player->m_vecPos.y = std::max(player->m_vecPos.y, 0.0f);
    }
}
TEST_CASE("Optimizer bench") {
    PlaybackInfo info;
    FrameBlock block1;
    block1.convars["tas_strafe_yaw"] = 0.0;
    block1.convars["tas_strafe"] = 1.0;

    FrameBlock block2;
    block2.convars["tas_strafe_yaw"] = 90.0;
    block2.frame = 100;

    info.current_script.blocks.push_back(block1);
    info.current_script.blocks.push_back(block2);

    TASQuake::OptimizerSettings settings;
    settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::FrameBlockMover()));
    double optimal = info.Get_Last_Frame() - 10.0; // This should be the y coordinate
    TASQuake::BenchTest(CornerFunc, &settings, &info, optimal);

    BENCHMARK("Corner bench") {
        TASQuake::BenchTest(CornerFunc, &settings, &info, optimal);
    };
}
