#include "cpp_quakedef.hpp"
#include "test_runner.hpp"
#include "script_parse.hpp"
#include "afterframes.hpp"
#include "utils.hpp"
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

static void Load_Test(bool generate)
{
	char buf[260];
	sprintf(buf, "%s/test/%s.qtest", com_gamedir, Cmd_Argv(1));
	TestScript script(buf);
	bool result = script.Load_From_File();
	if (!result)
	{
		return;
	}

	TEST_VECTOR.emplace_back(script);
	SCRIPT_INDEX = 0;
	BLOCK_INDEX = 0;
	HOOK_COUNT = 0;
	BLOCK_INDEX = 0;
	TEST_RUNNING = true;
	GENERATION_RUNNING = generate;
}

void Cmd_Test_Generate(void)
{
	Load_Test(true);
}

void Cmd_Test_Run(void)
{
	Load_Test(false);
}

bool Test_IsGeneratingTest()
{
	return GENERATION_RUNNING;
}

bool Test_IsRunningTest()
{
	return TEST_RUNNING;
}

void Test_Changelevel_Hook()
{
	if (Test_IsRunningTest() && CURRENT_HOOK() == HookEnum::LevelChange)
	{
		HookIteration();
	}
}

void Test_Runner_Frame_Hook()
{
	if (Test_IsRunningTest() && CURRENT_HOOK() == HookEnum::Frame && FilterMatchesCurrentFrame(CURRENT_FILTER()))
	{
		HookIteration();
	}
}

void Test_Script_Completed_Hook()
{
	if (Test_IsRunningTest() && CURRENT_HOOK() == HookEnum::ScriptCompleted)
	{
		HookIteration();
	}
}

static void StopTesting()
{
	TEST_RUNNING = false;
	GENERATION_RUNNING = false;
	TEST_OUTPUT = std::ostringstream();
	TEST_VECTOR.clear();
	AddAfterframes(0, "tas_script_stop;disconnect", NoFilter);
}

static void StartNewTest(bool failed)
{
	auto& currentTest = TEST_VECTOR[SCRIPT_INDEX];
	++SCRIPT_INDEX;
	BLOCK_INDEX = 0;
	HOOK_COUNT = 0;

	if (!failed)
	{
		TEST_OUTPUT << "Test \"" << currentTest.description << "\" ran successfully.\n";
	}

	if (SCRIPT_INDEX >= TEST_VECTOR.size())
	{
		std::string output = TEST_OUTPUT.str();
		Con_Print(const_cast<char*>(output.c_str()));
		if (TEST_VECTOR.size() > 1)
		{
			Con_Print("All tests were ran.\n");
		}
		StopTesting();
	}
}

static void BlockChange()
{
	auto& currentTest = TEST_VECTOR[SCRIPT_INDEX];
	++BLOCK_INDEX;
	HOOK_COUNT = 0;

	if (BLOCK_INDEX >= currentTest.blocks.size())
	{
		StartNewTest(false);
	}
}

static void HookIteration()
{
	++HOOK_COUNT;
	auto& currentBlock = TEST_VECTOR[SCRIPT_INDEX].blocks[BLOCK_INDEX];

	if (HOOK_COUNT >= currentBlock.hook_count)
	{
		AddAfterframes(0, currentBlock.command.c_str(), NoFilter);
		BlockChange();
	}	
}

void ReportFailure(const std::string& error)
{
	if(!Test_IsRunningTest())
		return;

	auto& currentTest = TEST_VECTOR[SCRIPT_INDEX];
	TEST_OUTPUT << "Test \"" << currentTest.description << "\" failed, reason: " << error << '\n';
	StartNewTest(true);
}