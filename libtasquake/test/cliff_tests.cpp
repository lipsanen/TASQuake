#include "catch_amalgamated.hpp"
#include "libtasquake/optimizer.hpp"
#include "libtasquake/utils.hpp"

static double Edge(double x) {
    if(x < 0) {
        return 0;
    } else {
        return 1;
    }
}

TEST_CASE("Cliff finder finds step function edge") {
    TASQuake::CliffFinder finder;
    finder.Init(Edge(1), 1, Edge(-1), -1, 1e-2);
    size_t i=0;

    for(; finder.m_eState == TASQuake::CliffState::InProgress && i < 9999; ++i) {
        double value = Edge(finder.GetValue());
        finder.Report(value);
    }

    double result = finder.GetValue();
    REQUIRE(TASQuake::DoubleEqual(result, 0, 1e-2));
}
