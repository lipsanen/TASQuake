#pragma once

#include "libtasquake/io.hpp"
#include "libtasquake/vector.hpp"
#include <cstdint>
#include <vector>

namespace TASQuake
{
    struct FrameBlockIndex {
        std::uint32_t m_uBlockIndex = 0;
        std::uint32_t m_uFrame = 0;
    };

    struct PredictionData {
        std::vector<Vector> m_vecPoints;
        std::vector<FrameBlockIndex> m_vecFBdata;
        std::int32_t m_iStartFrame = 0;
        std::int32_t m_iEndFrame = 0;

        void Load_From_Memory(TASQuakeIO::BufferReadInterface& iface);
        void Write_To_Memory(TASQuakeIO::BufferWriteInterface& iface) const;
        int FindFrameBlock(const Trace& trace); // -1 if none matched, frameblock index otherwise
    };

}