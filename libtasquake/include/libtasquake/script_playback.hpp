#pragma once

#include "script_parse.hpp"

struct PlaybackInfo
{
	PlaybackInfo();

	int pause_frame;
	int current_frame;
	bool script_running;
	bool should_unpause;
	double last_edited;

	FrameBlock stacked;
	TASScript current_script;

	const FrameBlock* Get_Stacked_Block() const;
	const FrameBlock* Get_Current_Block(int frame = -1) const;
	int GetBlockNumber(int frame = -1) const;
	int Get_Number_Of_Blocks() const;
	int Get_Last_Frame() const;
	bool In_Edit_Mode() const;
};
