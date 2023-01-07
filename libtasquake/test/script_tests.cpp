#include "catch_amalgamated.hpp"
#include "libtasquake/script_parse.hpp"

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
