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
    if(player->m_vecPos.x < 10.0) {
        player->m_vecPos.y = std::min(player->m_vecPos.y, 10.0f);
    }
}

static void PinHoleFunc(TASQuake::Player* player) {
    TASQuake::MemorylessSim(player);
    const float PIN_SIZE = 1.0f;
    // prevent players from moving past 50 y if they dont pass within the pinhole
    if(player->m_vecPos.y > 50 && player->m_vecPos.y < 52 && std::abs(player->m_vecPos.x) > PIN_SIZE / 2) {
        player->m_vecPos.y = 50;
    }
}

#if 0
TEST_CASE("PinHoleBench") {
        BENCHMARK_ADVANCED("Pin hole bench")(Catch::Benchmark::Chronometer meter) {
        PlaybackInfo info;
        FrameBlock block1;
        block1.convars["tas_strafe_yaw"] = 0.0;
        block1.convars["tas_strafe_type"] = 3.0f;
        block1.convars["tas_strafe"] = 1.0;

        FrameBlock block2;
        block2.convars["tas_strafe_yaw"] = 90.0;
        block2.frame = 100;

        info.current_script.blocks.push_back(block1);
        info.current_script.blocks.push_back(block2);

        TASQuake::OptimizerSettings settings;
        settings.m_uResetToBestIterations = 1;
        settings.m_uGiveUpAfterNoProgress = 9999;
        settings.m_vecAlgorithms.push_back(
            std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::StrafeAdjuster()));
        settings.m_vecAlgorithms.push_back(
            std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::RNGStrafer()));
        settings.m_vecAlgorithms.push_back(
            std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::RNGBlockMover()));
        double optimal = 52;

        meter.measure([&]{TASQuake::BenchTest(PinHoleFunc, &settings, &info, optimal);});
    };
}
#endif

TEST_CASE("CornerYawBench") {
        BENCHMARK_ADVANCED("Corner bench with yaw turns")(Catch::Benchmark::Chronometer meter) {
        PlaybackInfo info;
        FrameBlock block1;
        block1.convars["tas_strafe_yaw"] = 0.0;
        block1.convars["tas_strafe_type"] = 3.0f;
        block1.convars["tas_strafe"] = 1.0;

        FrameBlock block2;
        block2.convars["tas_strafe_yaw"] = 90.0;
        block2.frame = 100;

        info.current_script.blocks.push_back(block1);
        info.current_script.blocks.push_back(block2);

        TASQuake::OptimizerSettings settings;
        settings.m_uResetToBestIterations = 1;
        settings.m_uGiveUpAfterNoProgress = 9999;
        settings.m_vecAlgorithms.push_back(
            std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::StrafeAdjuster()));
        settings.m_vecAlgorithms.push_back(
            std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::RNGStrafer()));
        settings.m_vecAlgorithms.push_back(
            std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::RNGBlockMover()));
        double optimal = 130;

        meter.measure([&]{TASQuake::BenchTest(CornerFunc, &settings, &info, optimal);});
    };
}

TEST_CASE("Optimizer bench") {
    BENCHMARK_ADVANCED("Corner bench")(Catch::Benchmark::Chronometer meter) {
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
        settings.m_uResetToBestIterations = 1;
        settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::FrameBlockMover()));
        double optimal = 126; // This should be the y coordinate

        meter.measure([&]{TASQuake::BenchTest(CornerFunc, &settings, &info, optimal);});
    };
}


TEST_CASE("Turn bench") {
    BENCHMARK_ADVANCED("Turn bench")(Catch::Benchmark::Chronometer meter) {
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
        settings.m_uResetToBestIterations = 1;
        settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::StrafeAdjuster()));
        settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::FrameBlockMover()));
        settings.m_vecAlgorithms.push_back(std::shared_ptr<TASQuake::OptimizerAlgorithm>(new TASQuake::TurnOptimizer()));
        double optimal = 132.5;

        meter.measure([&]{TASQuake::BenchTest(CornerFunc, &settings, &info, optimal);});
    };
}
