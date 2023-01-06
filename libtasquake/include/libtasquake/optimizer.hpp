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
        virtual void Mutate(TASScript* script, Optimizer* opt) = 0;
        virtual void ReportResult(double efficacy) {};
        virtual void Reset() = 0;
        virtual bool WantsToRun(TASScript* script) { return true; } // True if the algorithm could do something useful
        virtual bool WantsToContinue() = 0; // In some case the algorithm might want to run multiple iterations in a row
        virtual int IterationsExpected() { return 1; } // How many iterations the algorithm is expected to run
        // The iteration count is used to divided the time evenly between different algorithms
    };

    enum class BinarySearchState { 
        NoSearch, // No search is in progress
        Probe, // Sent out a positive or negative probe
        MappingSpace, // Some improvement found, looking for max that gives worse result than current value
        BinarySearch // Found min and max, now searching
    };

    struct BinSearcher {
        // If min addition set, provides a lower bound for the next value to be tried
        void Init(double orig, double start, double min_addition, double min, double max, double orig_efficacy);
        void SetRandomStartOffset(double orig, double rng_seed);
        double GiveNewValue() const;
        void Report(double result); // Report the result of the iteration in higher = better format
        void Reset();

        // Constant
        const std::uint32_t m_uMappingIterations = 5;
        double m_dMinAddition = 0;
        double m_dRangeMin = 0;
        double m_dRangeMax = 0;

        bool m_bInitialized = false;
        double m_dOrigEfficacy = 0;
        double m_dMinEfficacy = 0, m_dMaxEfficacy = 0;
        double m_dMin = 0, m_dMax = 1;
        double m_dOriginalValue = 0;
        std::uint32_t m_uMappingIteration = 0;
        BinarySearchState m_eSearchState = BinarySearchState::NoSearch;
        bool m_bIsInteger = true;
    };

    // The world's only stone that rolls uphill
    struct RollingStone {
        void Init(double efficacy, double startValue, double startDelta, double maxValue);
        bool ShouldContinue(double newEfficacy) const;
        void NextValue();

        double m_dMultiplicationFactor = 2;
        double m_dPrevEfficacy = 0;
        double m_dCurrentValue = 0;
        double m_dPrevDelta = 0;
        double m_dMax = 0;
    };

    class RNGStrafer : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    };

    class RNGBlockMover : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    };

    class StrafeAdjuster : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    private:
        // These values are only non-default in the case that the algorithm found some improvement
        // and now wants to RollingStone over it
        std::int32_t m_iCurrentBlockIndex = -1;
        RollingStone m_Stone;
    };

    class FrameBlockMover : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    private:
        // These values are only non-default in the case that the algorithm found some improvement
        // and now wants to RollingStone over it
        std::int32_t m_iCurrentBlockIndex = -1;
        RollingStone m_Stone;
    };

    std::vector<double> GetCompoundingProbs(const std::vector<std::shared_ptr<OptimizerAlgorithm>> algorithms);
    size_t SelectIndex(double value, const std::vector<double>& compoundingProbs);

    struct Vector {
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };

    struct FrameData {
        Vector pos;
        double m_dVelTheta = 999.0;
        void FindSmallestStrafeYawIncrements(float strafe_yaw, float& min, float& max) const; 
    };

    enum class OptimizerState { ContinueIteration, NewIteration, Stop };
    enum class OptimizerGoal { Undetermined, PlusX, NegX, PlusY, NegY };

    struct OptimizerRun {
        PlaybackInfo playbackInfo;
        std::vector<FrameData> m_vecData;
        double RunEfficacy(OptimizerGoal goal) const;
        bool IsBetterThan(const OptimizerRun& run, OptimizerGoal goal) const;
        void StrafeBounds(size_t blockIndex, float& min, float& max) const;
    };

    OptimizerGoal AutoGoal(const std::vector<FrameData>& data);

    struct OptimizerSettings {
        // Determine which algorithms should be used
        OptimizerGoal m_Goal = OptimizerGoal::Undetermined; // Automatically determined by first run
        std::uint32_t m_uResetToBestIterations = 3;
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
