#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

#include "cpp_quakedef.hpp"
#include "script_parse.hpp"
#include "strafing.hpp"
#include "afterframes.hpp"
#include "utils.hpp"
#include "reset.hpp"
#include "hooks.h"

std::regex FRAME_NO_REGEX(R"#((\+?)(\d+):)#");
std::regex TOGGLE_REGEX(R"#(([\+\-])(\w+))#");
std::regex CONVAR_REGEX(R"#((\w+) "?(\d+(\.\d+)?)"?)#");

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

	if(Cvar_FindVar((char*)sm[1].str().c_str()))
		return true;
	else
		return false;
}

bool Is_Toggle(const std::string& line)
{
	return std::regex_match(line, TOGGLE_REGEX);
}

bool Is_Whitespace(const std::string& s) {
	return std::all_of(s.begin(), s.end(), isspace);
}

FrameBlock::FrameBlock()
{
	parsed = false;
}

std::string FrameBlock::GetCommand()
{
	std::ostringstream oss;

	for (auto& convar : convars)
		oss << convar.first << ' ' << convar.second << ';';

	for (auto& toggle : toggles)
	{
		if(toggle.second)
			oss << '+';
		else
			oss << '-';
		oss << toggle.first << ';';
	}


	for (auto& cmd : commands)
		oss << cmd << ';';

	return oss.str();
}

void FrameBlock::Add_Command(const std::string & line)
{
	commands.push_back(line);
}

void FrameBlock::Parse_Frame_No(const std::string & line, int& running_frame)
{
	std::smatch sm;
	std::regex_match(line, sm, FRAME_NO_REGEX);

	// No + in the frame number, take number as absolute
	if(sm[1].str().empty())
		running_frame = 0;
	frame = std::stoi(sm[2].str()) + running_frame;
	running_frame = frame;
	parsed = true;
}

void FrameBlock::Parse_Convar(const std::string & line)
{
	std::smatch sm;
	std::regex_match(line, sm, CONVAR_REGEX);
	std::string cmd = sm[1].str();
	float val = std::stof(sm[2].str());

	convars[cmd] = val;
}

void FrameBlock::Parse_Toggle(const std::string & line)
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

void FrameBlock::Parse_Command(const std::string & line)
{
	Add_Command(line);
}

void FrameBlock::Parse_Line(const std::string & line, int& running_frame)
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

TASScript::TASScript()
{
}

TASScript::TASScript(const char * file_name)
{
	this->file_name = file_name;
}

void TASScript::Load_From_File()
{
	std::ifstream stream(file_name);
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
	}
	catch (const std::exception& e)
	{
		Con_Printf("Error parsing line %d: %s\n", line_number, e.what());
	}

}
