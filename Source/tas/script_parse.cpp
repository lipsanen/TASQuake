#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "cpp_quakedef.hpp"

#include "script_parse.hpp"

#include "hooks.h"

#include "afterframes.hpp"
#include "reset.hpp"
#include "script_playback.hpp"
#include "strafing.hpp"
#include "utils.hpp"

std::regex FRAME_NO_REGEX(R"#((\+?)(\d+):)#");
std::regex TOGGLE_REGEX(R"#(([\+\-])(\w+))#");
std::regex CONVAR_REGEX(R"#((\w+) "?(-?\d+(\.\d+)?)"?)#");

static TASScript script;

void Parse_Newline(std::istream& is, std::string& str)
{
	std::getline(is, str);
	auto comment_pos = str.find("//");
	if (comment_pos != std::string::npos)
	{
		str = str.substr(0, comment_pos);
	}
	trim(str);
}

bool Is_Frame_Number(const std::string& line)
{
	return std::regex_match(line, FRAME_NO_REGEX);
}

bool Is_Convar(const std::string& line)
{
	std::smatch sm;
	std::regex_match(line, sm, CONVAR_REGEX);

	if (Cvar_FindVar((char*)sm[1].str().c_str()))
		return true;
	else
		return false;
}

bool Is_Toggle(const std::string& line)
{
	return std::regex_match(line, TOGGLE_REGEX);
}

bool Is_Whitespace(const std::string& s)
{
	return std::all_of(s.begin(), s.end(), isspace);
}

FrameBlock::FrameBlock()
{
	parsed = false;
}

void FrameBlock::Stack(const FrameBlock& new_block)
{
	for (auto& pair : new_block.toggles)
		toggles[pair.first] = pair.second;

	for (auto& pair : new_block.convars)
		convars[pair.first] = pair.second;
}

std::string FrameBlock::GetCommand()
{
	std::ostringstream oss;

	for (auto& convar : convars)
		oss << convar.first << ' ' << convar.second << ';';

	for (auto& toggle : toggles)
	{
		if (toggle.second)
			oss << '+';
		else
			oss << '-';
		oss << toggle.first << ';';
	}

	for (auto& cmd : commands)
		oss << cmd << ';';

	return oss.str();
}

void FrameBlock::Add_Command(const std::string& line)
{
	commands.push_back(line);
}

void FrameBlock::Parse_Frame_No(const std::string& line, int& running_frame)
{
	std::smatch sm;
	std::regex_match(line, sm, FRAME_NO_REGEX);

	// No + in the frame number, take number as absolute
	if (sm[1].str().empty())
		running_frame = 0;
	frame = std::stoi(sm[2].str()) + running_frame;
	running_frame = frame;
	parsed = true;
}

void FrameBlock::Parse_Convar(const std::string& line)
{
	std::smatch sm;
	std::regex_match(line, sm, CONVAR_REGEX);
	std::string cmd = sm[1].str();
	float val = std::stof(sm[2].str());

	convars[cmd] = val;
}

void FrameBlock::Parse_Toggle(const std::string& line)
{
	std::smatch sm;
	std::regex_match(line, sm, TOGGLE_REGEX);
	std::string cmd = sm[2].str();

	if (sm[1].str() == "+")
	{
		toggles[cmd] = true;
	}
	else
	{
		toggles[cmd] = false;
	}
}

void FrameBlock::Parse_Command(const std::string& line)
{
	Add_Command(line);
}

void FrameBlock::Parse_Line(const std::string& line, int& running_frame)
{
	if (Is_Whitespace(line))
		return;
	else if (Is_Frame_Number(line))
		Parse_Frame_No(line, running_frame);
	else if (Is_Convar(line))
		Parse_Convar(line);
	else if (Is_Toggle(line))
		Parse_Toggle(line);
	else
		Parse_Command(line);
}

void FrameBlock::Reset()
{
	parsed = false;
	convars.clear();
	toggles.clear();
	commands.clear();
}

bool FrameBlock::HasToggle(const std::string& cmd) const
{
	return toggles.find(cmd) != toggles.end();
}

bool FrameBlock::HasConvar(const std::string& cvar) const
{
	return convars.find(cvar) != convars.end();
}

TASScript::TASScript() {}

TASScript::TASScript(const char* file_name)
{
	this->file_name = file_name;
}

void TASScript::Load_From_File()
{
	std::ifstream stream;

	if (!Open_Stream(stream, file_name.c_str()))
	{
		Con_Printf("Unable to open script %s\n", file_name.c_str());
		return;
	}

	FrameBlock fb;
	int running_frame = 0;
	int line_number = 0;
	std::string current_line;

	try
	{
		while (stream.good() && !stream.eof())
		{
			Parse_Newline(stream, current_line);
			++line_number;
			if (Is_Frame_Number(current_line) && fb.parsed)
			{
				blocks.push_back(fb);
				fb.Reset();
			}
			fb.Parse_Line(current_line, running_frame);
		}
		if (fb.parsed)
			blocks.push_back(fb);
		Con_Printf("Script %s loaded with %u blocks.\n", file_name.c_str(), blocks.size());
	}
	catch (const std::exception& e)
	{
		Con_Printf("Error parsing line %d: %s\n", line_number, e.what());
	}
}

static bool Get_Backup_Save(char* buffer, const char* file_name, int backup)
{
	if (backup == 0)
	{
		sprintf(buffer, "%s", file_name);
		return true;
	}

	static char without_ext[256];
	const char* ext = strrchr(file_name, '.');
	if (!ext)
		return false;
	int sz = ext - file_name;
	int i;

	for (i = 0; i < sz && i < 255; ++i)
	{
		without_ext[i] = file_name[i];
	}

	without_ext[i] = '\0';
	sprintf(buffer, "%s-%d.qtas", without_ext, backup);
	return true;
}

static bool Move_Saves(const char* file_name)
{
	static char old_filename[256];
	static char new_filename[256];
	int backups = (int)tas_edit_backups.value;

	if (backups <= 0)
		return true;

	bool result = Get_Backup_Save(new_filename, file_name, backups - 1);
	if (!result)
	{
		Con_Printf("Failed to move saves for file %s\n", file_name);
		return false;
	}

	if (std::filesystem::exists(new_filename))
		std::remove(new_filename);

	for (int i = backups - 2; i >= 0; --i)
	{
		result = Get_Backup_Save(old_filename, file_name, i);
		if (!result)
		{
			Con_Printf("Failed to move save %d for file %s\n", file_name);
			return false;
		}
		if (std::filesystem::exists(old_filename))
			std::rename(old_filename, new_filename);
		std::strcpy(new_filename, old_filename);
	}

	return true;
}

void TASScript::Write_To_File()
{
	if (blocks.empty())
	{
		Con_Printf("Cannot write an empty script to file!\n");
		return;
	}

	bool result = Move_Saves(file_name.c_str());

	if (!result)
	{
		return;
	}

	std::ofstream os;

	if (!Open_Stream(os, file_name.c_str()))
	{
		Con_Printf("Unable to write to %s\n", file_name.c_str());
		return;
	}

	int current_frame = 0;

	for (auto& block : blocks)
	{
		int diff = block.frame - current_frame;
		os << '+' << diff << ':' << '\n';
		current_frame = block.frame;

		for (auto& cvar : block.convars)
		{
			os << '\t' << cvar.first << ' ' << cvar.second << '\n';
		}

		for (auto& toggle : block.toggles)
		{
			os << '\t' << (toggle.second ? '+' : '-') << toggle.first << '\n';
		}

		for (auto& cmd : block.commands)
		{
			os << '\t' << cmd << '\n';
		}
	}
	Con_Printf("Wrote script to file %s\n", file_name.c_str());
}

Bookmark::Bookmark() {}

Bookmark::Bookmark(int index, bool frame)
{
	this->index = index;
	this->frame = frame;
}

TestScript::TestScript()
{
}

TestScript::TestScript(const char * file_name)
{
	this->file_name = file_name;
}

void TestScript::Load_From_File()
{
	std::ifstream stream;

	if (!Open_Stream(stream, file_name.c_str()))
	{
		Con_Printf("Unable to open test %s\n", file_name.c_str());
		return;
	}

	int line_number = 0;
	std::string current_line;

	try
	{
		while (stream.good() && !stream.eof())
		{
			Parse_Newline(stream, current_line);
			if (line_number == 0)
			{
				this->description = current_line;
			}
			else
			{
				if (!Is_Whitespace(current_line))
				{
					blocks.push_back(TestBlock(current_line));
				}

			}
			++line_number;
		}
		
		Con_Printf("Test %s loaded with %u blocks.\n", file_name.c_str(), blocks.size());
	}
	catch (const std::exception& e)
	{
		Con_Printf("Error parsing line %d: %s\n", line_number, e.what());
	}
}

void TestScript::Write_To_File()
{
	if (blocks.empty())
	{
		Con_Printf("Cannot write an empty test to file!\n");
		return;
	}

	std::ofstream os;

	if (!Open_Stream(os, file_name.c_str()))
	{
		Con_Printf("Unable to write to %s\n", file_name.c_str());
		return;
	}

	os << this->description << '\n';
	
	for (auto& block : blocks)
	{
		block.Write_To_Stream(os);
	}

	Con_Printf("Wrote script to file %s\n", file_name.c_str());
}

void TestBlock::Write_To_Stream(std::ostream & os)
{
	switch (this->hook)
	{
	case HookEnum::Frame:
		os << 'f';
		break;
	case HookEnum::LevelChange:
		os << 'l';
		break;
	default:
		break;
	}

	os << '\t' << this->afterframes << '\t';

	for (int i = 0; i < 5; ++i)
	{
		
		if((this->afterframes_filter & (4 - i)) != 0)
			os << '1';
		else
			os << '0';
	}

	os << '\t';
	os << this->command << '\n';
}

TestBlock::TestBlock(const std::string& line)
{
	char cmd_buffer[256];
	char filter[5];
	int frames;
	char hook[1];

	int read = sscanf_s(line.c_str(),
	                    "%c\t%u\t%4s\t%s",
	                    hook,
	                    ARRAYSIZE(hook),
	                    &frames,
	                    filter,
	                    ARRAYSIZE(filter),
	                    cmd_buffer,
	                    ARRAYSIZE(cmd_buffer));

	if (read != 4)
		throw std::exception("Unable to read all required variables from line.");

	this->afterframes = frames;

	if(this->afterframes < 0)
	 throw std::exception("Afterframes cannot be negative.");

	switch (hook[0])
	{
	case 'f':
		this->hook = HookEnum::Frame;
		break;
	case 'l':
		this->hook = HookEnum::LevelChange;
		break;
	default:
		throw std::exception("invalid hook type");
	}

	this->afterframes_filter = 0;

	for (int i = 0; i < 5; ++i)
	{
		if (filter[i] == '1')
			this->afterframes_filter |= 1 << (4 - i);
		else if (filter[i] != '0')
			throw std::exception("Invalid bit in filter.");
	}
	
	command = cmd_buffer;
}

TestBlock::TestBlock()
{
	afterframes_filter = 0;
	afterframes = 0;
	hook_count = 0;
	hook = HookEnum::Frame;
}
