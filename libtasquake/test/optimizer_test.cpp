#include "catch_amalgamated.hpp"
#include "libtasquake/optimizer.hpp"

class ProbTester : public TASQuake::OptimizerAlgorithm {
public:
    size_t count;
    size_t iterations;
    ProbTester(size_t count) : count(count) { iterations = 0; }
    virtual void Mutate(TASScript* script, TASQuake::Optimizer*) { ++iterations; };
    virtual void Reset() {};
    virtual bool WantsToRun() { return true; }
    virtual bool WantsToContinue() { return false; };
    virtual int IterationsExpected() { return count; }
};

TEST_CASE("OptimizerSettings", "Serialization")
{
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    TASQuake::OptimizerSettings settings;
    settings.m_Goal = TASQuake::OptimizerGoal::NegY;
    settings.m_iEndOffset = 1;
    settings.m_uGiveUpAfterNoProgress = 2;
    settings.m_uResetToBestIterations = 3;
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::FrameBlockMover);
    settings.WriteToBuffer(writer);
    auto reader = TASQuakeIO::BufferReadInterface::Init(writer.m_pBuffer->ptr, writer.m_uFileOffset);
    TASQuake::OptimizerSettings settingsOut;
    settingsOut.ReadFromBuffer(reader);

    REQUIRE(settingsOut.m_Goal == settings.m_Goal);
    REQUIRE(settings.m_iEndOffset == settings.m_iEndOffset);
    REQUIRE(settings.m_uGiveUpAfterNoProgress == settings.m_uGiveUpAfterNoProgress);
    REQUIRE(settings.m_uResetToBestIterations == settings.m_uResetToBestIterations);
    REQUIRE(settings.m_vecAlgorithmData.size() == settingsOut.m_vecAlgorithmData.size());
    REQUIRE(settings.m_vecAlgorithmData[0] == settingsOut.m_vecAlgorithmData[0]);
}

TEST_CASE("Optimizer", "Works")
{
    TASScript script;
    FrameBlock block;
    block.frame = 100;
    block.parsed = true;
    block.Parse_Line("tas_strafe_yaw 0", block.frame);
    script.blocks.push_back(block);

    using alg_t = TASQuake::OptimizerAlgorithm;
    using ptr = std::shared_ptr<alg_t>;
    TASQuake::OptimizerSettings settings;
    settings.m_vecAlgorithms.push_back(std::shared_ptr<alg_t>(new ProbTester(1)));
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
    ProbTester* p = (ProbTester*)settings.m_vecAlgorithms[0].get();
    REQUIRE(p->iterations == 1);
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

TEST_CASE("Optimizer playback stacking works")
{
    // The cvar on frame 0 should get stacked to frame 1
    TASScript script;
    FrameBlock block;
    block.frame = 0;
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
    auto it = received->convars.find("tas_strafe_yaw");
    REQUIRE(it != received->convars.end());
    REQUIRE(it->second == 0.0);
    REQUIRE(opt.m_uLastFrame == settings.m_iEndOffset);
}

TEST_CASE("Compounding index selection works")
{
    std::vector<double> probs = {0.25, 0.3, 0.75, 1.0};
    REQUIRE(TASQuake::SelectIndex(0, probs) == 0);
    REQUIRE(TASQuake::SelectIndex(0.25, probs) == 0);
    REQUIRE(TASQuake::SelectIndex(0.26, probs) == 1);
    REQUIRE(TASQuake::SelectIndex(0.5, probs) == 2);
    REQUIRE(TASQuake::SelectIndex(0.999, probs) == 3);
    REQUIRE(TASQuake::SelectIndex(1, probs) == 3);
}

TEST_CASE("Compounding probability works")
{
    using alg_t = TASQuake::OptimizerAlgorithm;
    using ptr = std::shared_ptr<alg_t>;
    std::vector<ptr> algs;
    algs.push_back(std::shared_ptr<alg_t>(new ProbTester(1)));

    std::vector<double> output = TASQuake::GetCompoundingProbs(algs);
    REQUIRE(output.size() == 1);
    REQUIRE(output[0] == 1.0);

    algs.push_back(std::shared_ptr<alg_t>(new ProbTester(1)));
    output = TASQuake::GetCompoundingProbs(algs);
    REQUIRE(output.size() == 2);
    REQUIRE(output[0] == 0.5);
    REQUIRE(output[1] == 1.0);


    algs.push_back(std::shared_ptr<alg_t>(new ProbTester(2)));
    output = TASQuake::GetCompoundingProbs(algs);
    REQUIRE(output.size() == 3);
    REQUIRE(output[0] == 0.25);
    REQUIRE(output[1] == 0.5);
    REQUIRE(output[2] == 1.0);
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
