#pragma once
#include <string>
#include <map>
#include <vector>


struct FrameBlock
{
	FrameBlock();
	bool parsed;
	int frame;
	std::map<std::string, float> convars;
	std::map<std::string, bool> toggles;
	std::vector<std::string> commands;

	void Stack(const FrameBlock& new_block);
	std::string GetCommand();
	void Add_Command(const std::string& line);
	void Parse_Frame_No(const std::string& line, int& running_frame);
	void Parse_Convar(const std::string& line);
	void Parse_Toggle(const std::string& line);
	void Parse_Command(const std::string& line);
	void Parse_Line(const std::string& line, int& running_frame);
	void Reset();
};

class TASScript
{
public:
	TASScript();
	TASScript(const char* file_name);
	void Load_From_File();
	void Write_To_File();
	std::vector<FrameBlock> blocks;
private:
	std::string file_name;
};