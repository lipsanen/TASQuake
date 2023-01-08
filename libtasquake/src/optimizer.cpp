#include "libtasquake/optimizer.hpp"
#include "libtasquake/utils.hpp"
#include <functional>
#include <cmath>
#include <limits>

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

void Optimizer::_FinishIteration(OptimizerState& state) {
  if (m_settings.m_Goal == OptimizerGoal::Undetermined)
  {
    m_settings.m_Goal = AutoGoal(m_currentRun.m_vecData);
    
    for(size_t i=0; i < m_currentRun.m_vecData.size(); i += 36) {
      m_vecNodes.push_back(m_currentRun.m_vecData[i].pos);
    }
  }

  if (m_currentRun.IsBetterThan(m_currentBest, m_settings.m_Goal, m_vecNodes))
  {
    m_currentBest = m_currentRun;
    double efficacy = m_currentRun.RunEfficacy(m_settings.m_Goal, m_vecNodes);
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
    if(m_iCurrentAlgorithm != -1) { 
      auto ptr = m_settings.m_vecAlgorithms[m_iCurrentAlgorithm];
      double efficacy = m_currentRun.RunEfficacy(m_settings.m_Goal, m_vecNodes);
      ptr->ReportResult(efficacy);
      if(!ptr->WantsToContinue()) {
        ptr->Reset();
        m_iCurrentAlgorithm = -1;
        if(m_uIterationsWithoutProgress >= m_settings.m_uResetToBestIterations) {
          m_currentRun.playbackInfo.current_script = m_currentBest.playbackInfo.current_script;
          efficacy = m_currentRun.RunEfficacy(m_settings.m_Goal, m_vecNodes);
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
      ptr->Mutate(&m_currentRun.playbackInfo.current_script, this);
    }

    state = OptimizerState::NewIteration;
  }
}

OptimizerState Optimizer::OnRunnerFrame(const FrameData* data)
{
	m_currentRun.m_vecData.push_back(*data);
	OptimizerState state = OptimizerState::ContinueIteration;
	auto block = m_currentRun.playbackInfo.Get_Current_Block();

	if (m_currentRun.playbackInfo.current_frame == m_uLastFrame)
	{
    _FinishIteration(state);
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

	std::int32_t last_frame;

  if(playback->Get_Last_Frame() > playback->current_frame) {
    last_frame = playback->Get_Last_Frame() - playback->current_frame + settings->m_iEndOffset;
  } else {
    last_frame = settings->m_iEndOffset;
  }

	if (last_frame < 0)
	{
		return false; // Init failed
	}

  for(auto& alg : m_settings.m_vecAlgorithms) {
    alg->Reset();
  }

	m_settings = *settings;
	m_currentBest.m_vecData.clear();
  m_vecNodes.clear();
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

double OptimizerRun::RunEfficacy(OptimizerGoal goal, const std::vector<Vector>& nodes) const
{
	if (m_vecData.empty() || goal == OptimizerGoal::Undetermined)
	{
    return std::numeric_limits<double>::lowest();
	}

  size_t nodeIndex = 0;
  const float MAX_DIST = 100.0f;
  
  for(size_t i=0; i < m_vecData.size() && nodeIndex < nodes.size(); ++i) {
    float dist = nodes[nodeIndex].Distance(m_vecData[i].pos);
    if(dist < MAX_DIST) {
      ++nodeIndex;
    }
  }

  if(nodeIndex != nodes.size()) {
    return std::numeric_limits<double>::lowest();
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

bool OptimizerRun::IsBetterThan(const OptimizerRun& run, OptimizerGoal goal, const std::vector<Vector>& nodes) const 
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

	double ours = RunEfficacy(goal, nodes);
	double theirs = run.RunEfficacy(goal, nodes);

	return ours > theirs;
}


void OptimizerRun::StrafeBounds(size_t blockIndex, float& min, float& max) const {
  min = -TASQuake::INVALID_ANGLE;
  max = TASQuake::INVALID_ANGLE;

  FrameBlock block = playbackInfo.current_script.blocks[blockIndex];
  int start_frame = block.frame;

  if(block.convars.find("tas_strafe_yaw") == block.convars.end()) {
    return;
  } else if(block.HasCvarValue("tas_strafe_type", 3)) {
    // Direction strafing = any change affects the result
    min = -0.01;
    max = 0.01;
    return;
  }

  float strafe_yaw = block.convars["tas_strafe_yaw"];
  int last_frame = -1;
  size_t blocks = playbackInfo.current_script.blocks.size();

  for(size_t i=blockIndex+1; i < blocks; ++i) {
    if(playbackInfo.current_script.blocks[i].convars.find("tas_strafe_yaw") != block.convars.end()) {
      last_frame = playbackInfo.current_script.blocks[i].frame;
    } else if(playbackInfo.current_script.blocks[i].HasCvarValue("tas_strafe_type", 3)) {
      min = -0.01;
      max = 0.01;
      return;
    }
  }

  if (last_frame == -1) {
    last_frame = m_vecData.size();
  }

  for(size_t i=start_frame; i < last_frame; ++i) {
    m_vecData[i].FindSmallestStrafeYawIncrements(strafe_yaw, min, max);
  }
}

void BinSearcher::Init(double orig, double orig_efficacy, double max, double eps)
{
	m_eSearchState = BinarySearchState::MappingSpace;
  m_Cliffer.Reset();
	m_vecMapping.clear();
  m_dOriginalValue = orig;
  m_dRangeMax = max;
  m_bInitialized = true;
  m_dEPS = eps;
  m_vecMapping.push_back({orig, orig_efficacy});
}

static double GetMappingValue(size_t iteration, size_t totalIterations, double rangeMax, double orig) {
  double range = (rangeMax - orig);
  double m_uMappingDelta = totalIterations - iteration;
  // Try different deltas with log scaling
  double addition = range * std::pow(2, -m_uMappingDelta);
  return orig + addition;
}

double BinSearcher::GetValue() const
{
	if (m_eSearchState == BinarySearchState::NoSearch)
	{
		abort(); // Not initialized on entry
	} else if(m_eSearchState == BinarySearchState::Finished) {
    if(m_Cliffer.m_eState == CliffState::Finished) {
      return m_Cliffer.GetValue();
    } else {
      return m_dOriginalValue;
    }
  }

  if (m_eSearchState == BinarySearchState::MappingSpace) {
		return GetMappingValue(m_uMappingIteration, m_uMappingIterations, m_dRangeMax, m_dOriginalValue);
	} else {
		return m_Cliffer.GetValue();
	}
}

void BinSearcher::Report(double result) {
  if(m_eSearchState == BinarySearchState::NoSearch) {
    abort();
  } else if (m_eSearchState == BinarySearchState::MappingSpace) {
    m_vecMapping.push_back({GetValue(), result});
    if(m_uMappingIterations == m_uMappingIteration) {
      m_Cliffer.Init(m_vecMapping, m_dEPS);
      m_eSearchState = BinarySearchState::BinarySearch;
    } else {
      ++m_uMappingIteration;
    }
  } else {
    m_Cliffer.Report(result);

    if(m_Cliffer.m_eState == TASQuake::CliffState::Finished) {
      m_eSearchState = BinarySearchState::Finished;
    }
  }
}

void BinSearcher::Reset()
{
  m_dRangeMax = 0;
  m_bInitialized = false;
  m_dOriginalValue = 0;
  m_uMappingIteration = 0;
  m_eSearchState = BinarySearchState::NoSearch;
  m_dEPS = 1e-5;
  m_Cliffer.Reset();
}

void RollingStone::Init(double efficacy, double startValue, double startDelta, double maxValue)
{
  m_dPrevEfficacy = efficacy;
  m_dCurrentValue = startValue;
  m_dPrevDelta = startDelta;
  m_dMax = maxValue;
}

bool RollingStone::ShouldContinue(double newEfficacy) const {
  if(newEfficacy <= m_dPrevEfficacy) {
    return false;
  } else if(m_dCurrentValue == m_dMax) {
    return false;
  } else {
    return true;
  }
}

void RollingStone::NextValue() {
  m_dPrevDelta *= m_dMultiplicationFactor;
  m_dCurrentValue += m_dPrevDelta;
  if(m_dPrevDelta < 0) {
    m_dCurrentValue = std::max(m_dCurrentValue, m_dMax);
  } else {
    m_dCurrentValue = std::min(m_dCurrentValue, m_dMax);
  }
}

static std::int32_t modulo(std::int32_t index, std::int32_t size) {
  if(index == -1) {
    return size - 1;
  } else if(index == size) {
    return 0;
  } else {
    return index;
  }
}

static std::int32_t FindSuitableBlock(std::function<bool(TASScript*, size_t)> predicate, TASScript* script, Optimizer* opt) {
  if(script->blocks.size() == 0) {
    return -1;
  }

  std::int32_t index = opt->m_RNG() % script->blocks.size();
  if(predicate(script, index)) {
    return index;
  }

  // Randomize the probe direction in order to get rid of bias
  std::int32_t probe_direction = (opt->Random(0, 1) > 0.5) ? 1 : -1;
  std::int32_t probe_index = index + probe_direction;
  probe_index = modulo(probe_index, script->blocks.size());

  while(!predicate(script, probe_index) && probe_index != index) {
    probe_index += probe_direction;
    probe_index = modulo(probe_index, script->blocks.size());
  }

  // Went through every index
  if(probe_index == index) {
    return -1;
  } else {
    return probe_index;
  }
}

void RNGBlockMover::Reset() {}
bool RNGBlockMover::WantsToRun(TASScript* script) { return !script->blocks.empty(); }
bool RNGBlockMover::WantsToContinue() { return false; }
void RNGBlockMover::ReportResult(double efficacy) {}

void RNGStrafer::Mutate(TASScript* script, Optimizer* opt) {
  size_t blockIndex = opt->m_RNG() % script->blocks.size();
  bool hasExistingValue = false;

  if(script->blocks[blockIndex].convars.find("tas_strafe_yaw") != script->blocks[blockIndex].convars.end()) {
    hasExistingValue = true;
  }

  if(hasExistingValue && opt->Random(0, 1) < 0.8) {
    float prev = script->blocks[blockIndex].convars["tas_strafe_yaw"];
    script->blocks[blockIndex].convars["tas_strafe_yaw"] = NormalizeDeg(prev + opt->Random(-5, 5)); 
  } else {
    script->blocks[blockIndex].convars["tas_strafe_yaw"] = opt->Random(0, 360);
  }
}

void RNGStrafer::Reset() {

}

bool RNGStrafer::WantsToRun(TASScript* script) {
  return !script->blocks.empty();
}

bool RNGStrafer::WantsToContinue() {
  return false;
}

void RNGStrafer::ReportResult(double efficacy) {

}

void StrafeAdjuster::Mutate(TASScript* script, Optimizer* opt) {
  if(m_iCurrentBlockIndex == -1) {
    m_iCurrentBlockIndex = FindSuitableBlock([](TASScript* script, size_t blockIndex) { 
      return script->blocks[blockIndex].HasConvar("tas_strafe_yaw"); 
      }, script, opt);
    if(m_iCurrentBlockIndex == -1)
      return; // Shouldn't happen

    float min, max;
    FrameBlock* block = &script->blocks[m_iCurrentBlockIndex];
    float strafe_yaw = block->convars["tas_strafe_yaw"];
    opt->m_currentRun.StrafeBounds(m_iCurrentBlockIndex, min, max);
    double efficacy = opt->m_currentRun.RunEfficacy(opt->m_settings.m_Goal, opt->m_vecNodes);

    if(min == -TASQuake::INVALID_ANGLE && max == TASQuake::INVALID_ANGLE) {
      // Strafe angle does nothing
      m_iCurrentBlockIndex = -1;
      return;
    }
    bool pickMin;

    if (min == -TASQuake::INVALID_ANGLE) {
      pickMin = false;
    } else if (max == TASQuake::INVALID_ANGLE) {
      pickMin = true;
    } else {
      pickMin = opt->Random(0, 1) > 0.5;
    }

    if(pickMin) {
      m_Stone.Init(efficacy, strafe_yaw + max, max, strafe_yaw + 180.0);
    } else {
      m_Stone.Init(efficacy, strafe_yaw + min, min, strafe_yaw - 180.0);
    }
  }

  script->blocks[m_iCurrentBlockIndex].convars["tas_strafe_yaw"] = NormalizeDeg(m_Stone.m_dCurrentValue);
}

void StrafeAdjuster::Reset() {
  m_iCurrentBlockIndex = -1;
}

bool StrafeAdjuster::WantsToRun(TASScript* script) {
  for(size_t i=0; i < script->blocks.size(); ++i) {
    if(script->blocks[i].HasConvar("tas_strafe_yaw")) {
      return true;
    }
  }

  return false;
}

bool StrafeAdjuster::WantsToContinue() {
  return m_iCurrentBlockIndex != -1;
}

void StrafeAdjuster::ReportResult(double efficacy) {
  if(m_Stone.ShouldContinue(efficacy)) {
    m_Stone.NextValue();
  } else {
    m_iCurrentBlockIndex = -1;
  }
}

static bool MovableBlock(TASScript* script, size_t index) {
  int frame = script->blocks[index].frame;
  if(index == script->blocks.size() - 1) {
    return true;
  } else if(script->blocks[index + 1].frame > frame + 1) {
    return true;
  } else if(index > 0 && script->blocks[index-1].frame < frame - 1) {
    return true;
  } else {
    return false;
  }
}

static void find_block_minmax(TASScript* script, size_t index, size_t& min, size_t& max) {
  if(index == script->blocks.size() - 1) {
    max = script->blocks[index].frame + 36;
  } else {
    max = script->blocks[index + 1].frame - 1;
  }

  if(index == 0) {
    min = 0;
  } else {
    min = script->blocks[index - 1].frame + 1;
  }
}

void RNGBlockMover::Mutate(TASScript* script, Optimizer* opt) {
  auto index = FindSuitableBlock(MovableBlock, script, opt);
  FrameBlock* block = &script->blocks[index];
  size_t min, max;
  find_block_minmax(script, index, min, max);
  int result = opt->Random(min, max + 1);
  int delta = result - block->frame;
  script->ShiftBlocks(index, delta);
}

void FrameBlockMover::Mutate(TASScript* script, Optimizer* opt) {
  if(script->blocks.empty()) {
    return;
  }

  if(m_iCurrentBlockIndex == -1) {
    m_iCurrentBlockIndex = FindSuitableBlock(MovableBlock, script, opt);
    FrameBlock* block = &script->blocks[m_iCurrentBlockIndex];

    double orig = block->frame;
    double min;
    double max;
    double delta;
    bool positive;

    if(m_iCurrentBlockIndex == script->blocks.size() - 1) {
      max = block->frame + 36;
    } else {
      max = script->blocks[m_iCurrentBlockIndex + 1].frame - 1;
    }

    if(m_iCurrentBlockIndex == 0) {
      min = 0;
    } else {
      min = script->blocks[m_iCurrentBlockIndex - 1].frame + 1;
    }

    bool positive_legal = max != block->frame;
    bool negative_legal = min != block->frame;

    if(!positive_legal && !negative_legal) {
      return;
    }

    double value = (opt->m_RNG() / (double)opt->m_RNG.max());

    if(!positive_legal) {
      positive = false;
    } else if(!negative_legal) {
      positive = true;
    } else {
      positive = value > 0.5;
    }

    if(positive) {
      delta = 1;
    } else {
      delta = -1;
      max = min;
    }

    double efficacy = opt->m_currentRun.RunEfficacy(opt->m_settings.m_Goal, opt->m_vecNodes);
    m_Stone.Init(efficacy, orig + delta, delta, max);
  }
  
  FrameBlock* block = &script->blocks[m_iCurrentBlockIndex];
  int delta = m_Stone.m_dCurrentValue - block->frame;
  script->ShiftBlocks(m_iCurrentBlockIndex, delta);
}

void FrameBlockMover::Reset() {
  
}

bool FrameBlockMover::WantsToRun(TASScript* script) {
  return true;
}

bool FrameBlockMover::WantsToContinue() {
  return m_iCurrentBlockIndex != -1;
}

void FrameBlockMover::ReportResult(double efficacy) {
  if(m_Stone.ShouldContinue(efficacy)) {
    m_Stone.NextValue();
  } else {
    m_iCurrentBlockIndex = -1;
  }
}

void FrameData::FindSmallestStrafeYawIncrements(float strafe_yaw, float& min, float& max) const {
  const float EPS_DIFF = 1e-5;
  const float EPS_DEG = 1e-2;
	if(m_dVelTheta == 999.0 || std::isnan(m_dVelTheta)) {
		return;
	}

	double target_theta = strafe_yaw * M_DEG2RAD;
	double diff = NormalizeRad(target_theta - m_dVelTheta);
  float absDiffDeg = std::abs(diff * M_RAD2DEG) + EPS_DEG;
	
  if(diff >= 0) {
    min = std::max(min, -absDiffDeg);
	} else if(diff < 0) {
    max = std::min(max, absDiffDeg);
  }
}

void CliffFinder::Init(double edgeEfficacy, double edgePosition, double groundEfficacy, double groundPosition, double epsilon) {
  m_dEdge = edgePosition;
  m_dEdgeEfficacy = edgeEfficacy;
  m_dGround = groundPosition;
  m_dGroundEfficacy = groundEfficacy;
  m_eState = CliffState::InProgress;
  m_dEpsilon = epsilon;
}

void CliffFinder::Init(const std::vector<TASQuake::ValueEfficacyPair>& vec, double epsilon) {
  if(vec.size() < 2) {
    return;
  }

  size_t rising_index = 0;
  double rising_max_value = vec[0].efficacy;
  double rising_drop = 0;

  for(size_t i=1; i < vec.size(); ++i) {
    if(vec[i].efficacy >= rising_max_value) {
      rising_max_value = vec[i].efficacy;
    } else if(vec[i-1].efficacy == rising_max_value) {
      double drop = rising_max_value - vec[i].efficacy;

      if(drop > rising_drop) {
        rising_index = i-1;
        rising_drop = drop;
      }
    }
  }

  int falling_index = vec.size() - 1;
  double falling_max_value = vec[falling_index].efficacy;
  double falling_drop = 0;

  for(int i=vec.size()-2; i >= 0; --i) {
    if(vec[i].efficacy >= falling_max_value) {
      falling_max_value = vec[i].efficacy;
    } else if(vec[i+1].efficacy == falling_max_value) {
      double drop = falling_max_value - vec[i].efficacy;

      if(drop > falling_drop) {
        falling_index = i+1;
        falling_drop = drop;
      }
    }
  }

  if(rising_drop > falling_drop) {
    Init(vec[rising_index].efficacy, vec[rising_index].value, vec[rising_index+1].efficacy, vec[rising_index+1].value, epsilon);
  } else if(rising_drop < falling_drop) {
    Init(vec[falling_index].efficacy, vec[falling_index].value, vec[falling_index-1].efficacy, vec[falling_index-1].value, epsilon);
  }
}

void CliffFinder::Report(double result) {
  double value = GetValue();

  if(result >= m_dEdgeEfficacy) {
    m_dEdge = value;
    m_dEdgeEfficacy = result;
  } else {
    m_dGround = value;
    m_dGroundEfficacy = result;
  }

  if(std::abs(m_dEdge - m_dGround) < m_dEpsilon) {
    m_eState = CliffState::Finished;
  }
}

double CliffFinder::GetValue() const {
  if(m_eState == CliffState::Finished) {
    return m_dEdge;
  } else {
    return (m_dEdge + m_dGround) / 2;
  }
}

void CliffFinder::Reset() {
  m_eState = CliffState::NotCliffing;
  m_dEdgeEfficacy = 0;
  m_dEdge = 0;
  m_dGroundEfficacy = 0;
  m_dGround = 0;
  m_dEpsilon = 1e-5;
}

void TurnOptimizer::Mutate(TASScript* script, Optimizer* opt) {
  if(m_iTurnIndex == -1) {
    Init(script, opt);
  } else {
    script->blocks[m_iStrafeIndex].convars["tas_strafe_yaw"] = NormalizeDeg(m_Searcher.GetValue());
  }
}

void TurnOptimizer::Reset() {
  m_Searcher.Reset();
  m_iTurnIndex = -1;
  m_iStrafeIndex = -1;
  m_fOrigStrafeYaw = 0.0f;
  m_dSearchMax = 0.0;
}

bool TurnOptimizer::WantsToRun(TASScript* script) {
  return true;
}

bool TurnOptimizer::WantsToContinue() { 
  bool isNotFinished = m_Searcher.m_eSearchState != BinarySearchState::Finished;
  bool hasTurn = m_iTurnIndex != -1;
  return hasTurn && isNotFinished;
}

void TurnOptimizer::ReportResult(double efficacy) {
  if(m_Searcher.m_eSearchState == BinarySearchState::NoSearch) {
    const double EPS = 1e-2;
    m_Searcher.Init(m_fOrigStrafeYaw, efficacy, m_fOrigStrafeYaw + m_dSearchMax, EPS);
  } else {
    m_Searcher.Report(efficacy);
  }
}

static size_t FindStrafeBlock(TASScript* script, size_t turnIndex) {
  if(turnIndex == 0) {
    return 0;
  }

  // Find the previous frameblock that altered strafe yaw
  for(int i=turnIndex-1;i >= 0; --i) {
    if(script->blocks[i].HasConvar("tas_strafe_yaw")) {
      return i;
    }
  }

  return turnIndex;
}

static bool IsTurnFrameBlock(TASScript* script, size_t blockIndex) {
  bool has_yaw = script->blocks[blockIndex].HasConvar("tas_strafe_yaw");
  bool has_prev = FindStrafeBlock(script, blockIndex) != blockIndex;

  return has_yaw && has_prev;
}

void TurnOptimizer::Init(TASScript* script, Optimizer* opt) {
  bool useDifferentIndex = true;
  m_iTurnIndex = FindSuitableBlock(IsTurnFrameBlock, script, opt);

  if(m_iTurnIndex == -1) {
    return; // Failure, no turns
  }

  if(opt->Random(0, 1) > 0.25) {
    m_iStrafeIndex = FindStrafeBlock(script, m_iTurnIndex);
  } else {
    m_iStrafeIndex = m_iTurnIndex;
  }

  int signedDelta = opt->Random(0, 1) > 0.5 ? 1 : -1;

  if(!script->ShiftBlocks(m_iTurnIndex, signedDelta)) {
    m_iTurnIndex = -1;
    m_iStrafeIndex = -1;
    // Was unable to perform shift, give up
    return;
  }

  m_fOrigStrafeYaw = script->blocks[m_iStrafeIndex].convars["tas_strafe_yaw"];
  m_dSearchMax = opt->Random(-90, 90);
  double absMax = std::max(0.1, std::abs(m_dSearchMax));
  m_dSearchMax = std::copysign(absMax, m_dSearchMax);
}


float Vector::Distance(const Vector& rhs) const {
  return std::sqrt((x - rhs.x) * (x - rhs.x) + (y - rhs.y) * (y - rhs.y) + (z - rhs.z) * (z - rhs.z));
}
