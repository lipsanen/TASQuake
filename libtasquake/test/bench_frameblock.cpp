#include "catch_amalgamated.hpp"
#include "libtasquake/script_playback.hpp"

static void BenchFrameBlocks(size_t count, Catch::Benchmark::Chronometer* meter) {
    PlaybackInfo info;
    size_t stride = 10;

    for(size_t i=0; i < count; ++i) {
        FrameBlock block;
        block.frame = i * stride;
        info.current_script.blocks.push_back(block);
    }

    meter->measure([&]() {
        size_t frames = count * stride;
        for(size_t i=0; i < frames; ++i) {
            size_t expected_block = i / stride;
            if((i % stride) != 0) {
                expected_block += 1;
            }
            size_t block = info.GetBlockNumber(i);
            if(block != expected_block) {
                std::printf("ERROR: index %lu block %lu\n", i, block);
            }
            REQUIRE(block == expected_block);
        }
    }
    );
}

TEST_CASE("Frameblock bench") {
    BENCHMARK_ADVANCED("FrameBlock 1")(Catch::Benchmark::Chronometer meter) {
        BenchFrameBlocks(1, &meter);
    };
    BENCHMARK_ADVANCED("FrameBlock 100")(Catch::Benchmark::Chronometer meter) {
        BenchFrameBlocks(100, &meter);
    };
    BENCHMARK_ADVANCED("FrameBlock 1000")(Catch::Benchmark::Chronometer meter) {
        BenchFrameBlocks(1000, &meter);
    };
}