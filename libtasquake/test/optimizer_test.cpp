#include "catch_amalgamated.hpp"
#include "libtasquake/optimizer.hpp"

TEST_CASE("Optimizer", "Works")
{
    TASScript script;
    FrameBlock block;
    block.frame = 100;
    block.parsed = true;
    block.Parse_Line("tas_strafe_yaw 0", block.frame);
    script.blocks.push_back(block);

    TASQuake::OptimizerSettings settings;
    PlaybackInfo info;
    info.current_script = script;

    TASQuake::Optimizer opt;
    opt.Init(&info, &settings);
    opt.ResetIteration();
    size_t expected_frames = 100 + settings.m_iEndOffset;

    for(size_t i=0; i <= expected_frames; ++i) {
        auto block_ptr = opt.GetCurrentFrameBlock();
        TASQuake::FrameData framedata;
        framedata.pos.x = i;
        auto result = opt.OnRunnerFrame(&framedata);

        if(i != 100 && i < expected_frames) {
            REQUIRE(result == TASQuake::OptimizerState::ContinueIteration);
            REQUIRE(block_ptr == NULL);
        } else if (i == 100) {
            REQUIRE(block_ptr != NULL);
        } else {
            REQUIRE(result == TASQuake::OptimizerState::NewIteration);
        }
    }

    REQUIRE(opt.m_settings.m_Goal == TASQuake::OptimizerGoal::PlusX);
}

TEST_CASE("Optimizer gives up")
{
    TASScript script;
    FrameBlock block;
    block.frame = 2;
    block.parsed = true;
    block.Parse_Line("tas_strafe_yaw 0", block.frame);
    script.blocks.push_back(block);

    TASQuake::OptimizerSettings settings;
    settings.m_iEndOffset = 0;
    settings.m_uGiveUpAfterNoProgress = 1;
    PlaybackInfo info;
    info.current_script = script;

    TASQuake::Optimizer opt;
    TASQuake::FrameData framedata;
    opt.Init(&info, &settings);
    opt.ResetIteration();

    REQUIRE(opt.OnRunnerFrame(&framedata) == TASQuake::OptimizerState::ContinueIteration);
    REQUIRE(opt.OnRunnerFrame(&framedata) == TASQuake::OptimizerState::ContinueIteration);
    REQUIRE(opt.OnRunnerFrame(&framedata) == TASQuake::OptimizerState::NewIteration);

    opt.ResetIteration();

    REQUIRE(opt.OnRunnerFrame(&framedata) == TASQuake::OptimizerState::ContinueIteration);
    REQUIRE(opt.OnRunnerFrame(&framedata) == TASQuake::OptimizerState::ContinueIteration);
    REQUIRE(opt.OnRunnerFrame(&framedata) == TASQuake::OptimizerState::Stop);
}

TEST_CASE("Optimizer adjusts time based on playback")
{
    TASScript script;
    FrameBlock block;
    block.frame = 1;
    block.parsed = true;
    block.Parse_Line("tas_strafe_yaw 0", block.frame);
    script.blocks.push_back(block);

    TASQuake::OptimizerSettings settings;
    settings.m_iEndOffset = 0;
    settings.m_uGiveUpAfterNoProgress = 1;
    PlaybackInfo info;
    info.current_script = script;
    info.current_frame = 1;

    TASQuake::Optimizer opt;
    TASQuake::FrameData framedata;
    opt.Init(&info, &settings);
    opt.ResetIteration();

    auto* received = opt.GetCurrentFrameBlock();
    REQUIRE(received != nullptr);
}

TEST_CASE("AutoGoal", "Works")
{
    std::vector<TASQuake::FrameData> vec1;
    REQUIRE(TASQuake::AutoGoal(vec1) == TASQuake::OptimizerGoal::Undetermined);
    vec1.resize(1);
    REQUIRE(TASQuake::AutoGoal(vec1) == TASQuake::OptimizerGoal::Undetermined);
    vec1.resize(2);
    vec1[1].pos.x = 1.0f;
    REQUIRE(TASQuake::AutoGoal(vec1) == TASQuake::OptimizerGoal::PlusX);
    vec1[1].pos.x = -1.0f;
    REQUIRE(TASQuake::AutoGoal(vec1) == TASQuake::OptimizerGoal::NegX);
    vec1[1].pos.x = 0.5f;
    vec1[1].pos.y = 1.0f;
    REQUIRE(TASQuake::AutoGoal(vec1) == TASQuake::OptimizerGoal::PlusY);
    vec1[1].pos.y = -1.0f;
    REQUIRE(TASQuake::AutoGoal(vec1) == TASQuake::OptimizerGoal::NegY);
}

