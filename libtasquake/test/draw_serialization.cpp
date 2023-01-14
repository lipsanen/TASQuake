#include "catch_amalgamated.hpp"
#include "libtasquake/draw.hpp"
#include "libtasquake/io.hpp"

TEST_CASE("Can save rects") {
    auto size = 99999;
    std::array<float, 4> color = {1, 0.75, 0.5, 0.25};
    std::vector<TASQuake::Rect> rects;

    for(size_t i=0; i < size; ++i) {
        float center[3];
        center[0] = i;
        center[1] = i;
        center[2] = i;

        rects.push_back(TASQuake::Rect::Get_Rect(color, center, 10, 10, 1));
    }

    auto buffer = TASQuakeIO::GetBufferFromElems(rects);
    std::vector<TASQuake::Rect> copy;
    TASQuakeIO::GetElemsFromBuffer(buffer, copy);

    REQUIRE(copy.size() == size);    
    REQUIRE(memcmp(&rects[0], &copy[0], sizeof(TASQuake::Rect) * copy.size()) == 0);
}
