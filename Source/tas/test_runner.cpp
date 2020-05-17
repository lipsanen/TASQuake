#include "cpp_quakedef.hpp"
#include "test_runner.hpp"
#include "script_parse.hpp"
#include "afterframes.hpp"
#include <sstream>

static std::vector<TestScript> TEST_VECTOR;
static int SCRIPT_INDEX;
static int BLOCK_INDEX;
static bool TEST_RUNNING = false;
static int HOOK_COUNT = 0;
static bool GENERATION_RUNNING = false;
static std::ostringstream TEST_OUTPUT;

static void HookIteration();

static HookEnum CURRENT_HOOK()
{
	auto& currentBlock = TEST_VECTOR[SCRIPT_INDEX].blocks[BLOCK_INDEX];
	return currentBlock.hook;
}
static unsigned int CURRENT_FILTER()
{
	auto& currentBlock = TEST_VECTOR[SCRIPT_INDEX].blocks[BLOCK_INDEX];
	return currentBlock.afterframes_filter;
}

void Test_Changelevel_Hook()
{
	if (IsRunningTest() && CURRENT_HOOK() == HookEnum::LevelChange)
	{
		HookIteration();
	}
}

void Test_Frame_Hook()
{
	if (IsRunningTest() && CURRENT_HOOK() == HookEnum::Frame && FilterMatchesCurrentFrame(CURRENT_FILTER()))
	{
		HookIteration();
	}
}
static void StopTesting()
{
	TEST_RUNNING = false;
	GENERATION_RUNNING = false;
	TEST_OUTPUT.clear();
}

static void StartNewTest()
{
	auto& currentTest = TEST_VECTOR[SCRIPT_INDEX];
	++SCRIPT_INDEX;
	BLOCK_INDEX = 0;
	HOOK_COUNT = 0;

	if (SCRIPT_INDEX >= TEST_VECTOR.size())
	{
		Con_Print("All tests were ran.\n");
		std::string output = TEST_OUTPUT.str();
		Con_Print(const_cast<char*>(output.c_str()));
		StopTesting();
	}
	else
	{
		TEST_OUTPUT << "Test \"" << currentTest.description << "\" ran successfully.\n";
	}
}

static void BlockChange()
{
	auto& currentTest = TEST_VECTOR[SCRIPT_INDEX];
	++BLOCK_INDEX;
	HOOK_COUNT = 0;

	if (BLOCK_INDEX >= currentTest.blocks.size())
	{
		AddAfterframes(0, currentTest.exit_block.command.c_str());
		StartNewTest();
	}
}

static void HookIteration()
{
	++HOOK_COUNT;
	auto& currentBlock = TEST_VECTOR[SCRIPT_INDEX].blocks[BLOCK_INDEX];

	if (HOOK_COUNT >= currentBlock.hook_count)
	{
		AddAfterframes(0, currentBlock.command.c_str());
		BlockChange();
	}	
}

bool IsGeneratingTest()
{
	return GENERATION_RUNNING;
}

bool IsRunningTest()
{
	return TEST_RUNNING;
}

void ReportFailure(const std::string& error)
{
	if(!IsRunningTest())
		return;

	auto& currentTest = TEST_VECTOR[SCRIPT_INDEX];
	TEST_OUTPUT << "Test \"" << currentTest.description << "\" failed, reason: " << error << '\n';
	StartNewTest();
}