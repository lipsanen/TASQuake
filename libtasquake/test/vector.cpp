#include "catch_amalgamated.hpp"
#include "libtasquake/vector.hpp"
#include "libtasquake/utils.hpp"

TEST_CASE("vec cross product works")
{
    TASQuake::Vector v1(1, 0, 0);
    TASQuake::Vector v2(0, 1, 0);
    TASQuake::Vector v3 = TASQuake::VecCrossProduct(v1, v2);
    TASQuake::Vector v3_expected(0, 0, 1);

    REQUIRE(v3.Distance(v3_expected) < 1e-5);
}

TEST_CASE("dist from point works")
{
    TASQuake::Vector origin;
    TASQuake::Trace tr;
    tr.m_vecAngles = TASQuake::Vector(0, 0, 0);
    tr.m_vecStartPos = TASQuake::Vector(0, 0, 0);

    TASQuake::Vector point(0, 0, 1);
    float dist = TASQuake::DistanceFromPoint(tr, point);
    REQUIRE(TASQuake::DoubleEqual(dist, 1));
}
