#pragma once

#include "script_parse.hpp"

struct PlaybackInfo
{
	PlaybackInfo();

	int pause_frame = 0;
	int current_frame = 0;
	bool script_running = false;
	bool should_unpause = false;
	double last_edited = 0.0;
	mutable int prev_block_number = 0;

	FrameBlock stacked;
	TASScript current_script;

	const FrameBlock* Get_Stacked_Block() const;
	const FrameBlock* Get_Current_Block(int frame = -1) const;
	int GetBlockNumber(int frame = -1) const;
	int Get_Number_Of_Blocks() const;
	int Get_Last_Frame() const;
	bool In_Edit_Mode() const;
	void CalculateStack();

	static PlaybackInfo GetTimeShiftedVersion(const PlaybackInfo* info, int start_frame = -1);
};
