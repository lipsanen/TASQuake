#include "libtasquake/prediction.hpp"
#include <limits>

using namespace TASQuake;

void PredictionData::Load_From_Memory(TASQuakeIO::BufferReadInterface& iface)
{
    iface.Read(&m_uStartFrame);
    iface.Read(&m_uEndFrame);
    iface.ReadPODVec(m_vecFBdata);
    iface.ReadPODVec(m_vecPoints);
}

void PredictionData::Write_To_Memory(TASQuakeIO::BufferWriteInterface& iface) const
{
    iface.Write(&m_uStartFrame);
    iface.Write(&m_uEndFrame);
    iface.WritePODVec(m_vecFBdata);
    iface.WritePODVec(m_vecPoints);
}

int PredictionData::FindFrameBlock(const Trace& trace)
{
    const float MIN_DIST = 10.0f;
    float lowestDistance = std::numeric_limits<float>::max();
    FrameBlockIndex bestBlock;
    for(auto& blockIndex : m_vecFBdata) {
        auto vecIndex = blockIndex.m_uFrame - m_uStartFrame;
        if(vecIndex < m_vecPoints.size()) {
            float dist = TASQuake::DistanceFromPoint(trace, m_vecPoints[vecIndex]);
            if(dist < lowestDistance)
            {
                lowestDistance = dist;
                bestBlock = blockIndex;
            }
        }
    }

    if(lowestDistance < MIN_DIST)
    {
        return -1;
    }
    else
    {
        return bestBlock.m_uBlockIndex;
    }
}
