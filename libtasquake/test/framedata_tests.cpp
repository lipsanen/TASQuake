#include "catch_amalgamated.hpp"
#include "libtasquake/optimizer.hpp"
#include "libtasquake/utils.hpp"

static bool ANG_EQ(float f1, float f2) {
    const float EPS = 1e-3;
    float diff = std::abs(NormalizeDeg(f1 - f2));

    return diff < EPS;
}

TEST_CASE("Strafe bounds work")
{
    TASQuake::FrameData data;
    data.m_dVelTheta = 0;

    float min = -999.0;
    float max = 999.0f;
    data.FindSmallestStrafeYawIncrements(1, min, max);
    REQUIRE(ANG_EQ(-1.01, min));
    REQUIRE(max == 999.0f);

    data.FindSmallestStrafeYawIncrements(-1, min, max);
    REQUIRE(ANG_EQ(-1.01, min));
    REQUIRE(ANG_EQ(1.01, max));
}

TEST_CASE("OptimizerRun finds strafe bounds")
{
    TASQuake::OptimizerRun run;
    FrameBlock block;
    block.convars["tas_strafe_yaw"] = 0;
    block.frame = 0;

    run.playbackInfo.current_script.blocks.push_back(block);
    for(size_t i=0; i < 36; ++i) {
        TASQuake::FrameData data;
        data.m_dVelTheta = 1;
        run.m_vecData.push_back(data);
    }

    float min, max;
    run.StrafeBounds(0, min, max);
    REQUIRE(ANG_EQ(min, -TASQuake::INVALID_ANGLE));
    REQUIRE(ANG_EQ(max, M_RAD2DEG + 0.01));
    run.playbackInfo.current_script.blocks[0].convars["tas_strafe_type"] = 3;

    run.StrafeBounds(0, min, max);
    REQUIRE(ANG_EQ(min, -0.01));
    REQUIRE(ANG_EQ(max, 0.01));

    run.playbackInfo.current_script.blocks[0].convars["tas_strafe_type"] = 0;

    run.StrafeBounds(0, min, max);
    REQUIRE(ANG_EQ(min, -TASQuake::INVALID_ANGLE));
    REQUIRE(ANG_EQ(max, M_RAD2DEG + 0.01));

    FrameBlock block2;
    block2.convars["tas_strafe_yaw"] = 0;
    block2.frame = 1;
    run.playbackInfo.current_script.blocks.push_back(block2);
    run.m_vecData[1].m_dVelTheta = -1;
    // should have no impact, vel theta changed in the duration of the next block
    run.StrafeBounds(0, min, max);
    REQUIRE(ANG_EQ(min, -TASQuake::INVALID_ANGLE));
    REQUIRE(ANG_EQ(max, M_RAD2DEG + 0.01));
}
