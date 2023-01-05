#include <limits>

#include "libtasquake/optimizer.hpp"

#include "libtasquake/utils.hpp"

using namespace TASQuake;

void Optimizer::ResetIteration()
{
	m_currentRun.playbackInfo.current_frame = 0;
	m_currentRun.playbackInfo.pause_frame = 0;
	m_currentRun.m_vecData.clear();
}

const FrameBlock* Optimizer::GetCurrentFrameBlock() const
{
	auto block = m_currentRun.playbackInfo.Get_Current_Block();

	if (block && block->frame == m_currentRun.playbackInfo.current_frame)
	{
		return block;
	}
	else
	{
		return nullptr;
	}
}

OptimizerState Optimizer::OnRunnerFrame(const FrameData* data)
{
	m_currentRun.m_vecData.push_back(*data);
	OptimizerState state = OptimizerState::ContinueIteration;
	auto block = m_currentRun.playbackInfo.Get_Current_Block();

	if (m_currentRun.playbackInfo.current_frame == m_uLastFrame)
	{
		if (m_settings.m_Goal == OptimizerGoal::Undetermined)
		{
			m_settings.m_Goal = AutoGoal(m_currentRun.m_vecData);
		}

		if (m_currentRun.IsBetterThan(m_currentBest, m_settings.m_Goal))
		{
			m_currentBest = m_currentRun;
			m_uIterationsWithoutProgress = 0;
		}
		else
		{
			m_uIterationsWithoutProgress += 1;
		}

		if (m_uIterationsWithoutProgress >= m_settings.m_uGiveUpAfterNoProgress)
		{
			state = OptimizerState::Stop;
		}
		else
		{
      double efficacy = m_currentRun.RunEfficacy(m_settings.m_Goal);
      if(m_iCurrentAlgorithm != -1) { 
				auto ptr = m_settings.m_vecAlgorithms[m_iCurrentAlgorithm];
        ptr->ReportResult(efficacy);
        if(!ptr->WantsToContinue()) {
          m_iCurrentAlgorithm = -1;
          if(m_uIterationsWithoutProgress >= m_settings.m_uResetToBestIterations) {
            m_currentRun.playbackInfo.current_script = m_currentBest.playbackInfo.current_script;
            efficacy = m_currentRun.RunEfficacy(m_settings.m_Goal);
          }
        }
      }

			// Might be empty in some test cases
			if (!m_settings.m_vecAlgorithms.empty())
			{
        if(m_iCurrentAlgorithm == -1) {
				  m_iCurrentAlgorithm = RandomizeIndex();
        }
				auto ptr = m_settings.m_vecAlgorithms[m_iCurrentAlgorithm];
				ptr->Mutate(&m_currentRun.playbackInfo.current_script, efficacy);
			}

			state = OptimizerState::NewIteration;
		}
	}
	else if (m_currentRun.playbackInfo.current_frame > m_uLastFrame)
	{
		TASQuake::Log("Optimizer in buggy state, skipped past last frame\n");
		state = OptimizerState::NewIteration;
	}

	m_currentRun.playbackInfo.current_frame += 1;

	return state;
}

void Optimizer::Seed(std::uint32_t value)
{
	m_RNG.seed(value);
}

double Optimizer::Random(double min, double max)
{
	double val = m_RNG() / (double)m_RNG.max();
	return min + (max - min) * val;
}

size_t Optimizer::RandomizeIndex()
{
	double val = Random(0, 1);
	return SelectIndex(val, m_vecCompoundingProbs);
}

bool Optimizer::Init(const PlaybackInfo* playback, const TASQuake::OptimizerSettings* settings)
{
	if (playback->current_frame == 0)
	{
		m_currentBest.playbackInfo = *playback;
	}
	else
	{
		m_currentBest.playbackInfo = PlaybackInfo::GetTimeShiftedVersion(playback);
	}

	std::int32_t last_frame = playback->Get_Last_Frame() + settings->m_iEndOffset;

	if (last_frame < 0)
	{
		return false; // Init failed
	}

	m_settings = *settings;
	m_currentBest.m_vecData.clear();
	m_currentRun = m_currentBest;
	m_uIterationsWithoutProgress = 0;
	m_iCurrentAlgorithm = -1;
	m_uLastFrame = last_frame;
	m_vecCompoundingProbs = GetCompoundingProbs(settings->m_vecAlgorithms);

	return true;
}

std::vector<double> TASQuake::GetCompoundingProbs(const std::vector<std::shared_ptr<OptimizerAlgorithm>> algorithms)
{
	double totalIterations = 0;

	for (auto alg : algorithms)
	{
		totalIterations += alg->IterationsExpected();
	}

	std::vector<double> output;
	output.reserve(algorithms.size());

	double compounding = 0;

	for (auto alg : algorithms)
	{
		compounding += alg->IterationsExpected() / totalIterations;
		output.push_back(compounding);
	}

	if (!output.empty())
	{
		output[output.size() - 1] = 1.0;
	}

	return output;
}

size_t TASQuake::SelectIndex(double value, const std::vector<double>& compoundingProbs)
{
	for (size_t i = 0; i < compoundingProbs.size(); ++i)
	{
		if (value <= compoundingProbs[i])
		{
			return i;
		}
	}

	return compoundingProbs.size() - 1;
}

OptimizerGoal TASQuake::AutoGoal(const std::vector<FrameData>& vecData)
{
	if (vecData.size() <= 1)
	{
		return OptimizerGoal::Undetermined;
	}

	Vector last = vecData[vecData.size() - 1].pos;
	Vector secondlast = vecData[vecData.size() - 2].pos;

	// Determine goal automatically from the last delta
	float plusx = last.x - secondlast.x;
	float negx = -plusx;
	float plusy = last.y - secondlast.y;
	float negy = -plusy;

	if (plusx >= negx && plusx >= std::abs(plusy))
	{
		return OptimizerGoal::PlusX;
	}
	else if (negx >= plusx && negx >= std::abs(plusy))
	{
		return OptimizerGoal::NegX;
	}
	else if (plusy >= negy && plusy >= std::abs(plusx))
	{
		return OptimizerGoal::PlusY;
	}
	else
	{
		return OptimizerGoal::NegY;
	}
}

double OptimizerRun::RunEfficacy(OptimizerGoal goal) const
{
	if (m_vecData.empty() || goal == OptimizerGoal::Undetermined)
	{
		return std::numeric_limits<double>::min();
	}

	size_t index = m_vecData.size() - 1;
	Vector last = m_vecData[index].pos;

	if (goal == OptimizerGoal::NegX)
	{
		return -last.x;
	}
	else if (goal == OptimizerGoal::NegY)
	{
		return -last.y;
	}
	else if (goal == OptimizerGoal::PlusX)
	{
		return last.x;
	}
	else
	{
		return last.y;
	}
}

bool OptimizerRun::IsBetterThan(const OptimizerRun& run, OptimizerGoal goal) const 
{
  if (m_vecData.empty()) {
    return false;
  } else if (run.m_vecData.empty()) {
    return true;
  } else if (run.m_vecData.size() != m_vecData.size())
	{
		TASQuake::Log("Ran different number of iterations with optimizer\n");
    abort();
	}
	else if (goal == OptimizerGoal::Undetermined)
	{
		return true;
	}

	double ours = RunEfficacy(goal);
	double theirs = run.RunEfficacy(goal);

	return ours > theirs;
}

void BinSearcher::Init(double orig, double start, double min_addition, double min, double max, double orig_efficacy)
{
	m_eSearchState = BinarySearchState::Probe;
	m_dOriginalValue = orig;
	m_dMin = start;
  m_dMinAddition = min_addition;
  m_dRangeMin = min;
  m_dRangeMax = max;
  m_bInitialized = true;
}

void BinSearcher::SetRandomStartOffset(double orig, double rng_seed) {
  double start = orig;
  if(rng_seed > 0.5) {
    start += m_dMinAddition;
  } else {
    start -= m_dMinAddition;
  }
}

double BinSearcher::GiveNewValue() const
{
	if (m_eSearchState == BinarySearchState::NoSearch)
	{
		abort(); // Not initialized on entry
	}

	double output;

  if(m_eSearchState == BinarySearchState::Probe) {
    output = m_dMin;
  } else if (m_eSearchState == BinarySearchState::MappingSpace) {
    double range;

    if(m_dMin > m_dOriginalValue) {
      range = m_dRangeMax - m_dMin;
    } else {
      range = m_dMin - m_dRangeMin;
    }

		double m_uMappingDelta = m_uMappingIterations - m_uMappingIteration;
		// Try different deltas with log scaling
		double addition = range * std::pow(2, -m_uMappingDelta);
		output = m_dOriginalValue + addition;
	}
	else {
		output = (m_dMax + m_dMin) / 2;
	}

	return output;
}

void BinSearcher::Report(double result) {
  if(m_eSearchState == BinarySearchState::NoSearch) {
    abort();
  } else if(m_eSearchState == BinarySearchState::Probe) {
    if(result > m_dOrigEfficacy) {
      m_dMinEfficacy = result;
      m_eSearchState = BinarySearchState::MappingSpace;
    } else {
      m_eSearchState = BinarySearchState::NoSearch;
    }
  } else if (m_eSearchState == BinarySearchState::MappingSpace) {
    m_dMax = GiveNewValue();
    if(result < m_dMinEfficacy || m_uMappingIterations < m_uMappingIteration) {
      m_dMaxEfficacy = result;
      m_eSearchState = BinarySearchState::BinarySearch;
    } else {
      m_dMinEfficacy = result;
      m_dMin = m_dMax;
    }
    ++m_uMappingIteration;
  } else {
    if(m_dMax > m_dMin) {
      m_dMin = (m_dMax + m_dMin) / 2;
      m_dMinEfficacy = result;
    } else {
      m_dMax = (m_dMax + m_dMin) / 2;
      m_dMaxEfficacy;
    }

    double delta = std::abs(m_dMax - m_dMin);
    if(delta <= m_dMinAddition) {
      m_eSearchState = BinarySearchState::NoSearch;
    }
  }
}

void BinSearcher::Reset()
{
  m_dMinAddition = 0;
  m_dRangeMin = 0;
  m_dRangeMax = 0;
  m_bInitialized = false;
  m_dMin = 0;
  m_dMax = 1;
  m_dOriginalValue = 0;
  m_uMappingIteration = 0;
  m_eSearchState = BinarySearchState::NoSearch;
  m_bIsInteger = true;
  m_dMinEfficacy = 0;
  m_dMaxEfficacy = 0;
  m_dOrigEfficacy = 0;
}
