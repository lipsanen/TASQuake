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
	Frame,          // Execute like a normal command right after the previous one
	LevelChange,	// Execute when level change has completed
	ScriptCompleted // Script run completed
};

struct TestBlock
{
	HookEnum hook; // The hook used for the test block
	int hook_count; // How many iterations of this hook should complete before performing the action
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
	TestScript();						// Default constructor is only intended for containers
	TestScript(const char* file_name);	// Initialize script with c-string filename
	bool Load_From_File();				// Load the script from file
	void Write_To_File();				// Write the script to file
	std::vector<TestBlock> blocks;		// Testblocks in the test
	TestBlock exit_block;				// Testblock to be run after the test is finished
	std::string file_name;				// Filename of the test
	std::string description;			// Description on what the test is testing
};
