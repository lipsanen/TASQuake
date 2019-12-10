#pragma once
#include <string>
#include <map>
#include <vector>
#include "afterframes.hpp"


struct FrameBlock
{
	FrameBlock();
	bool parsed;
	int frame;
	std::map<std::string, float> convars;
	std::map<std::string, bool> toggles;
	std::string commands;

	void Add_Command(const std::string& line);
	void Parse_Frame_No(const std::string& line, int& running_frame);
	void Parse_Convar(const std::string& line);
	void Parse_Toggle(const std::string& line);
	void Parse_Command(const std::string& line);
	void Parse_Line(const std::string& line, int& running_frame);
	AfterFrames Get_Afterframes();
	void Reset();
};

class TASScript
{
public:
	TASScript();
	TASScript(const char* file_name);
	void Load_From_File();
	void Run();
private:
	std::string file_name;
	std::vector<FrameBlock> blocks;
};

void Cmd_TAS_Stop(void);
void Cmd_TAS_Load(void);
void Cmd_TAS_Run(void);