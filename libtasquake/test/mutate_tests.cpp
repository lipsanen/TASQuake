
#include "catch_amalgamated.hpp"
#include "libtasquake/optimizer.hpp"
#include <iostream>

TEST_CASE("Mutate") {
    const char* str = "+100:\n"
    "\t+attack\n";

    TASScript script;
    TASQuake::RNGShooter shooter;
    TASQuake::Optimizer opt;
    opt.m_uLastFrame = 200;
    shooter.Mutate(&script, &opt);

    std::cout << script.ToString() << std::endl;
}
