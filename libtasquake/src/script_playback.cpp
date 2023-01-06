#include "libtasquake/script_playback.hpp"
#include "libtasquake/utils.hpp"

PlaybackInfo::PlaybackInfo()
{
	pause_frame = 0;
	current_frame = 0;
	script_running = false;
	should_unpause = false;
}

const FrameBlock* PlaybackInfo::Get_Current_Block(int frame) const
{
	int blck = GetBlockNumber(frame);

	if (blck >= current_script.blocks.size())
		return nullptr;
	else
		return &current_script.blocks[blck];
}

const FrameBlock* PlaybackInfo::Get_Stacked_Block() const
{
	return &stacked;
}

int PlaybackInfo::GetBlockNumber(int frame) const
{
	if (frame == -1)
		frame = current_frame;

	const size_t MAX_LINEAR_SEARCH_SIZE = 16;
	size_t blocks = current_script.blocks.size();

	if(blocks == 0) {
		return blocks;
	}

	if(blocks < MAX_LINEAR_SEARCH_SIZE) {
		for (int i = 0; i < blocks; ++i)
		{
			if (current_script.blocks[i].frame >= frame)
			{
				return i;
			}
		}
	} else {
		size_t low = 0;
		size_t high = blocks;

		if(current_script.blocks[prev_block_number].frame >= frame) {
			if(prev_block_number == 0) {
				return prev_block_number;
			} else if(current_script.blocks[prev_block_number-1].frame < frame) {
				return prev_block_number;
			}
		}

		// Binary search for higher block counts
		while(low < high - 1) {
			size_t mid = (low + high) / 2;
			if (current_script.blocks[mid].frame == frame) {
				low = mid;
				break;
			} else if (current_script.blocks[mid].frame > frame) {
				high = mid;
			} else {
				low = mid;
			}
		}

		// Fix up the index
		if(current_script.blocks[low].frame < frame) {
			prev_block_number = low + 1;
		} else {
			prev_block_number = low;
		}

		return prev_block_number;
	}

	return current_script.blocks.size();
}

int PlaybackInfo::Get_Number_Of_Blocks() const
{
	return current_script.blocks.size();
}

int PlaybackInfo::Get_Last_Frame() const
{
	if (current_script.blocks.empty())
		return 0;
	else
	{
		return current_script.blocks[current_script.blocks.size() - 1].frame;
	}
}

bool PlaybackInfo::In_Edit_Mode() const
{
	return !script_running && TASQuake::GamePaused() && !current_script.blocks.empty();
}

PlaybackInfo PlaybackInfo::GetTimeShiftedVersion(const PlaybackInfo* info, int start_frame) {
	PlaybackInfo output;
	output.current_script.file_name = info->current_script.file_name;
	int old_start_frame = info->current_frame;
	if(start_frame == -1) {
		start_frame = info->current_frame;
	}

	FrameBlock stacked;
	stacked.frame = 0;
	stacked.parsed = true;
	bool added_stack = false;

	for (int i = 0; i < info->current_script.blocks.size(); ++i)
	{
		FrameBlock block = info->current_script.blocks[i];
		if (block.frame <= start_frame) {
			stacked.Stack(block);
		} else {
			if(!added_stack) {
				output.current_script.blocks.push_back(stacked);
				added_stack = true;
			}

			block.frame -= start_frame;
			output.current_script.blocks.push_back(block);
		}
	}

	if(!added_stack) {
		output.current_script.blocks.push_back(stacked);
		added_stack = true;
	}
	
	return output;
}
