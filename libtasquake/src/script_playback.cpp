#include "libtasquake/script_playback.hpp"
#include "libtasquake/utils.hpp"

PlaybackInfo::PlaybackInfo()
{
	pause_frame = -1;
	current_frame = -1;
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

	for (int i = 0; i < current_script.blocks.size(); ++i)
	{
		if (current_script.blocks[i].frame >= frame)
		{
			return i;
		}
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
