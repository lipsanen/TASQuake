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

static double ReverseEdge(double x) {
    if(x <= 0) {
        return 1;
    } else {
        return 0;
    }
}

const double EPS = 1e-5;

TEST_CASE("Cliff finder finds step function edge") {
    TASQuake::CliffFinder finder;
    finder.Init(Edge(1), 1, Edge(-1), -1, EPS);
    size_t i=0;

    for(; finder.m_eState == TASQuake::CliffState::InProgress && i < 9999; ++i) {
        double value = Edge(finder.GetValue());
        finder.Report(value);
    }

    double result = finder.GetValue();
    REQUIRE(TASQuake::DoubleEqual(result, 0, EPS));
}

TEST_CASE("Bin search finds step function edge") {
    TASQuake::BinSearcher searcher;
    searcher.Init(1, Edge(1), -0.1, EPS);
    size_t i=0;

    for(; searcher.m_eSearchState != TASQuake::BinarySearchState::Finished && i < 9999; ++i) {
        double value = Edge(searcher.GetValue());
        searcher.Report(value);
    }

    double result = searcher.GetValue();
    REQUIRE(TASQuake::DoubleEqual(result, 0, EPS));
}

TEST_CASE("Bin search finds step function edge other detection") {
    TASQuake::BinSearcher searcher;
    searcher.Init(-1, Edge(-1), 0.1, EPS);
    size_t i=0;

    for(; searcher.m_eSearchState != TASQuake::BinarySearchState::Finished && i < 9999; ++i) {
        double value = Edge(searcher.GetValue());
        searcher.Report(value);
    }

    double result = searcher.GetValue();
    REQUIRE(TASQuake::DoubleEqual(result, 0, EPS));
}


TEST_CASE("Bin search finds step function reverse edge") {
    TASQuake::BinSearcher searcher;
    searcher.Init(1, ReverseEdge(1), -0.1, EPS);
    size_t i=0;

    for(; searcher.m_eSearchState != TASQuake::BinarySearchState::Finished && i < 9999; ++i) {
        double value = ReverseEdge(searcher.GetValue());
        searcher.Report(value);
    }

    double result = searcher.GetValue();
    REQUIRE(TASQuake::DoubleEqual(result, 0, EPS));
}

TEST_CASE("Bin search finds step function reverse edge other detection") {
    TASQuake::BinSearcher searcher;
    searcher.Init(-1, ReverseEdge(-1), 0.1, EPS);
    size_t i=0;

    for(; searcher.m_eSearchState != TASQuake::BinarySearchState::Finished && i < 9999; ++i) {
        double value = ReverseEdge(searcher.GetValue());
        searcher.Report(value);
    }

    double result = searcher.GetValue();
    REQUIRE(TASQuake::DoubleEqual(result, 0, EPS));
}
