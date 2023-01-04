#include "libtasquake/optimizer.hpp"
#include "libtasquake/utils.hpp"

using namespace TASQuake;

void Optimizer::ResetIteration() {
    m_currentRun.playbackInfo.current_frame = 0;
    m_currentRun.playbackInfo.pause_frame = 0;
    m_currentRun.m_vecData.clear();
}

const FrameBlock* Optimizer::GetCurrentFrameBlock() const {
    auto block = m_currentRun.playbackInfo.Get_Current_Block();

    if(block && block->frame == m_currentRun.playbackInfo.current_frame) {
        return block;
    } else {
        return nullptr;
    }
}

OptimizerState Optimizer::OnRunnerFrame(const FrameData* data) {
    m_currentRun.m_vecData.push_back(*data);
    OptimizerState state = OptimizerState::ContinueIteration;
    auto block = m_currentRun.playbackInfo.Get_Current_Block();

    if(m_currentRun.playbackInfo.current_frame == m_uLastFrame) {
        if(m_settings.m_Goal == OptimizerGoal::Undetermined) {
            m_settings.m_Goal = AutoGoal(m_currentRun.m_vecData);
        }

        if(m_currentRun.IsBetterThan(m_currentBest, m_settings.m_Goal)) {
            m_currentBest = m_currentRun;
            m_uIterationsWithoutProgress = 0;
        } else {
            m_uIterationsWithoutProgress += 1;
        }

        // TODO: mutation
        if(m_uIterationsWithoutProgress >= m_settings.m_uGiveUpAfterNoProgress) {
            state = OptimizerState::Stop;
        } else {
            state = OptimizerState::NewIteration;
        }

    } else if(m_currentRun.playbackInfo.current_frame > m_uLastFrame) {
        TASQuake::Log("Optimizer in buggy state, skipped past last frame\n");
        state = OptimizerState::NewIteration;
    }

    m_currentRun.playbackInfo.current_frame += 1;

    return state;
}

bool Optimizer::Init(const PlaybackInfo* playback, const TASQuake::OptimizerSettings* settings) {
    // TODO: Cut off frames from the start
    if(playback->current_frame == 0) {
        m_currentBest.playbackInfo = *playback;
    } else {
        m_currentBest.playbackInfo = PlaybackInfo::GetTimeShiftedVersion(playback);
    }

    std::int32_t last_frame = playback->Get_Last_Frame() + settings->m_iEndOffset;

    if(last_frame <= 0) {
        return false; // Init failed
    }
    
    m_settings = *settings;
    m_currentBest.m_vecData.clear();
    m_currentRun = m_currentBest;
    m_uIterationsWithoutProgress = 0;
    m_iCurrentAlgorithm = -1;
    m_uLastFrame = last_frame;

    return true;
}

OptimizerGoal TASQuake::AutoGoal(const std::vector<FrameData>& vecData) {
    if(vecData.size() <= 1) {
        return OptimizerGoal::Undetermined;
    }

    Vector last = vecData[vecData.size() - 1].pos;
    Vector secondlast = vecData[vecData.size() - 2].pos;

    // Determine goal automatically from the last delta
    float plusx = last.x - secondlast.x;
    float negx = -plusx;
    float plusy = last.y - secondlast.y;
    float negy = -plusy;

    if(plusx >= negx && plusx >= std::abs(plusy)) {
        return OptimizerGoal::PlusX;
    } else if (negx >= plusx && negx >= std::abs(plusy)) {
        return OptimizerGoal::NegX;
    } else if(plusy >= negy && plusy >= std::abs(plusx)) {
        return OptimizerGoal::PlusY;
    } else {
        return OptimizerGoal::NegY;
    }
    
}

bool OptimizerRun::IsBetterThan(const OptimizerRun& run, OptimizerGoal goal) {
    if(run.m_vecData.empty()) {
        return true; // Something is better than nothing
    } else if(m_vecData.empty()) {
        return false;
    } else if(run.m_vecData.size() != m_vecData.size()) {
        TASQuake::Log("Ran different number of iterations with optimizer");
        return false; // The optimizer should always run the same number of frames
    }

    size_t index = m_vecData.size() - 1;
    Vector last = m_vecData[index].pos;
    Vector rhsLast = run.m_vecData[m_vecData.size() - 1].pos;

    if(goal == OptimizerGoal::Undetermined) {
        return true;
    } else if(goal == OptimizerGoal::NegX) {
        return last.x < rhsLast.x;
    } else if(goal == OptimizerGoal::NegY) {
        return last.y < rhsLast.y;
    } else if(goal == OptimizerGoal::PlusX) {
        return last.x > rhsLast.x;
    } else {
        return last.y > rhsLast.y;
    }
}
