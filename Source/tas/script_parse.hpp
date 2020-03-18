#pragma once
#include <map>
#include <string>
#include <vector>

struct Bookmark
{
	Bookmark();
	Bookmark(int index, bool frame);

	int index;
	bool frame;
};

enum class HookEnum
{
	None,          // Execute like a normal command right after the previous one
	CommandFinish, // Execute after a special command has finished that reports to the tester
	LevelChange // Execute when level change has completed
};

struct TestBlock
{
	HookEnum hook; // The hook used for the test block
	int hook_count; // How many iterations of this hook should complete before performing the action
	int afterframes; // How many frames after the hook has fired should it be executed
	unsigned int afterframes_filter; // Filter for the afterframes command
	std::string command; // The command to be executed


	void Write_To_Stream(std::ostream& os);
	TestBlock(const std::string& line);
	TestBlock();
};

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

	bool HasToggle(const std::string& cmd) const;
	bool HasConvar(const std::string& cvar) const;
};

class TASScript
{
public:
	TASScript();
	TASScript(const char* file_name);
	void Load_From_File();
	void Write_To_File();
	std::vector<FrameBlock> blocks;
	std::map<std::string, Bookmark> bookmarks;
	std::string file_name;
};

class TestScript
{
public:
	TestScript();
	TestScript(const char* file_name);
	void Load_From_File();
	void Write_To_File();
	std::vector<TestBlock> blocks;
	TestBlock exit_block;
	std::string file_name;
};
