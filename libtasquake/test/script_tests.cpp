#include "catch_amalgamated.hpp"
#include "libtasquake/script_parse.hpp"
#include "libtasquake/utils.hpp"

TEST_CASE("script addition test") {
    TASScript script;
    FrameBlock block1;
    block1.frame = 1;
    script.blocks.push_back(block1);

    TASScript script2;
    FrameBlock block2;
    block2.commands.push_back("test");
    block2.frame = 0;
    script2.blocks.push_back(block2);

    script.AddScript(&script2, 2);
    REQUIRE(script.blocks.size() == 2);
    REQUIRE(script.blocks[1].commands[0] == "test");

    script.AddScript(&script2, 1);
    REQUIRE(script.blocks.size() == 1);
    REQUIRE(script.blocks[0].commands[0] == "test");
}

TEST_CASE("shot test")
{
    TASScript script;
    bool rval = script.AddShot(1, 2, 20, 10);
    REQUIRE(script.blocks.size() == 3);
    REQUIRE(script.blocks[0].frame == 10);
    REQUIRE(TASQuake::DoubleEqual(script.blocks[0].convars["tas_view_pitch"], 1));
    REQUIRE(TASQuake::DoubleEqual(script.blocks[0].convars["tas_view_yaw"], 2));
    REQUIRE(script.blocks[1].toggles["attack"] == true);
    REQUIRE(script.blocks[1].frame == 19);
    REQUIRE(script.blocks[2].toggles["attack"] == false);
    REQUIRE(script.blocks[2].frame == 20);

    REQUIRE(rval == true);

    // Addition to the same place should return false, it already exists
    rval = script.AddShot(1, 2, 20, 10);
    REQUIRE(rval == false);

    // Changing the pitch should result in true
    rval = script.AddShot(2, 2, 20, 10);
    REQUIRE(rval == true);

    // Changing the yaw should result in true
    rval = script.AddShot(2, 3, 20, 10);
    REQUIRE(rval == true);

    // Changing the frame should result in true
    rval = script.AddShot(2, 3, 21, 10);
    REQUIRE(rval == true);

    // Changing the turn frames should result in true
    rval = script.AddShot(2, 3, 21, 9);
    REQUIRE(rval == true);
}
