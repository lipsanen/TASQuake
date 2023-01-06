#pragma once
#include <unordered_map>
#include <string>
#include <vector>

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
	bool parsed = false;
	int frame = 0;
	std::unordered_map<std::string, float> convars;
	std::unordered_map<std::string, bool> toggles;
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

	bool HasCvarValue(const std::string& cmd, float value) const;
	bool HasToggle(const std::string& cmd) const;
	bool HasConvar(const std::string& cvar) const;
};

class TASScript
{
public:
	TASScript();
	TASScript(const char* file_name);
	bool Load_From_File();
	void Write_To_File();
	std::vector<FrameBlock> blocks;
	std::string file_name;
	void Prune(int min_frame, int max_frame);
	void Prune(int min_frame);
	void RemoveTogglesFromRange(const std::string& name, int min_frame, int max_frame);
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
