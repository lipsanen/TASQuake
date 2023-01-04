#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <vector>
#include "script_parse.hpp"
#include "script_playback.hpp"

namespace TASQuake {
    class Optimizer;

    class OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script) = 0;
        virtual void Reset() = 0;
        virtual bool WantsToRun() { return true; } // True if the algorithm could do something useful
        virtual bool WantsToContinue() = 0; // In some case the algorithm might want to run multiple iterations in a row
        virtual int IterationsExpected() { return 1; } // How many iterations the algorithm is expected to run
        // The iteration count is used to divided the time evenly between different algorithms
    };

    std::vector<double> GetCompoundingProbs(const std::vector<std::shared_ptr<OptimizerAlgorithm>> algorithms);
    size_t SelectIndex(double value, const std::vector<double>& compoundingProbs);

    struct Vector {
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };

    struct FrameData {
        Vector pos;
        std::uint32_t m_uKills = 0;
        std::uint32_t m_uSecrets = 0;
        std::uint32_t m_uButtonPresses = 0;
        std::int32_t m_iHealth = 100;
        bool m_bIntermission = false;
    };

    enum class OptimizerState { ContinueIteration, NewIteration, Stop };
    enum class OptimizerGoal { Undetermined, PlusX, NegX, PlusY, NegY };

    struct OptimizerRun {
        PlaybackInfo playbackInfo;
        std::vector<FrameData> m_vecData;
        bool IsBetterThan(const OptimizerRun& run, OptimizerGoal goal);
    };

    OptimizerGoal AutoGoal(const std::vector<FrameData>& data);

    struct OptimizerSettings {
        // Determine which algorithms should be used
        OptimizerGoal m_Goal = OptimizerGoal::Undetermined; // Automatically determined by first run
        std::uint32_t m_uMaxIterationsNoProgress = 3;
        std::uint32_t m_uGiveUpAfterNoProgress = 999;
        std::int32_t m_iEndOffset = 36; // Where to end the optimizer path as frames from the last frame
        std::vector<std::shared_ptr<OptimizerAlgorithm>> m_vecAlgorithms;
    };

    struct Optimizer {
        void ResetIteration();
        // Gets the current frame block or null if no block for current frame
        const FrameBlock* GetCurrentFrameBlock() const;
        // Runner calls this after every frame
        OptimizerState OnRunnerFrame(const FrameData* data);
        // Run the init function with the actual full script, the optimizer figures out the relevant bit from playbackInfo
        bool Init(const PlaybackInfo* playback, const OptimizerSettings* settings);
        void Seed(std::uint32_t value);
        double Random(double min, double max);
        size_t RandomizeIndex();

        OptimizerRun m_currentBest; // The current best run
        OptimizerRun m_currentRun; // The current run
        OptimizerSettings m_settings; // Current settings for the optimizer
        std::mt19937 m_RNG;
        std::vector<double> m_vecCompoundingProbs;
        std::int32_t m_iCurrentAlgorithm = -1;
        std::uint32_t m_uLastFrame = 1;
        std::uint32_t m_uIterationsWithoutProgress = 0; // How many iterations have been ran without progress, determines when we should reset back to best
    };
}
