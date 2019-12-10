#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <regex>

#include "cpp_quakedef.hpp"
#include "script.hpp"
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

void FrameBlock::Add_Command(const std::string & line)
{
	commands += line;
	commands.push_back(';');
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
	Add_Command(line);
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
	Add_Command(line);
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

AfterFrames FrameBlock::Get_Afterframes()
{
	return AfterFrames(frame+1, commands.c_str());
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

void TASScript::Run()
{
	Cmd_TAS_Stop();
	tas_playing.value = 1;
	int last_frame = 0;
	for (auto& block : blocks)
	{
		AfterFrames af = block.Get_Afterframes();
		AddAfterframes(af);
		last_frame = max(last_frame, af.frames);
	}

	AddAfterframes(last_frame, "tas_playing 0");
}

void Cmd_TAS_Stop(void)
{
	ClearAfterframes();
	Cmd_TAS_Reset_f();
}

void Cmd_TAS_Load(void)
{
	char name[256];
	sprintf(name, "%s/tas/%s", com_gamedir, Cmd_Argv(1));
	COM_ForceExtension(name, ".qtas");

	script = TASScript(name);
	script.Load_From_File();
}

void Cmd_TAS_Run(void)
{
	script.Run();
}
