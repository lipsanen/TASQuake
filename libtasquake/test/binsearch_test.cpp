#include "catch_amalgamated.hpp"
#include "libtasquake/optimizer.hpp"

TEST_CASE("BinSearch starts mapping") {
  TASQuake::BinSearcher searcher;
  searcher.Init(0, 1, 1, 0, 90, 0);
  double value = searcher.GiveNewValue();
  REQUIRE(searcher.m_eSearchState == TASQuake::BinarySearchState::Probe);
  REQUIRE(value == 1);
  searcher.Report(1);
  REQUIRE(searcher.m_eSearchState == TASQuake::BinarySearchState::MappingSpace);
}

TEST_CASE("BinSearch worse stops") {
  TASQuake::BinSearcher searcher;
  searcher.Init(0, 1, 1, 0, 90, 0);
  double value = searcher.GiveNewValue();
  REQUIRE(searcher.m_eSearchState == TASQuake::BinarySearchState::Probe);
  REQUIRE(value == 1);
  searcher.Report(-1);
  REQUIRE(searcher.m_eSearchState == TASQuake::BinarySearchState::NoSearch);
}

TEST_CASE("BinSearch always better stops") {
  // If the search always gets better we might be on a continuous upward slope
  // This is just a random heuristic but seems like the search would likely not find a good maximum
  TASQuake::BinSearcher searcher;
  searcher.Init(0, 1, 1, 0, 90, 0);
  double value = searcher.GiveNewValue();
  size_t i;
  for(i=0; i < 100 && searcher.m_eSearchState != TASQuake::BinarySearchState::NoSearch; ++i) {
    searcher.Report(i + 1);
  }

  REQUIRE(searcher.m_eSearchState == TASQuake::BinarySearchState::NoSearch);
}
