#include <direct.h>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <zlib.h>

#include "cpp_quakedef.hpp"
#include "data_export.hpp"
#include "utils.hpp"
#include "state_test.hpp"
#include "reset.hpp"
#include "test_runner.hpp"
#include "script_playback.hpp"

class TestCase
{
private:
	nlohmann::json initial_data;
	std::vector<nlohmann::json> deltas;
	int frame_count;
	std::string path;

public:
	nlohmann::json current;
	TestCase();
	TestCase(int frames, const std::string& filepath);
	void Apply(int frame);
	int FrameCount()
	{
		return frame_count;
	}
	const std::string& Filepath()
	{
		return path;
	}
	void GenerateFrame();
	void Clear();
	friend std::ostream& operator<<(std::ostream& out, const TestCase& c);
	friend std::istream& operator>>(std::istream& in, TestCase& c);
	void SaveToFile();
	static TestCase LoadFromFile(char* file_name);
};

static TestCase oldCase;
static TestCase newCase;
static int TEST_FRAME = 0;
static int TEST_LENGTH = 0;
static bool COLLECTING_DATA = false;
constexpr int START_OFFSET = 6;

void Compare(bool final_iteration=false)
{
	if (oldCase.current != newCase.current)
	{
		auto patch = nlohmann::json::diff(newCase.current, oldCase.current).dump(4);
		COLLECTING_DATA = false;
		std::ostringstream oss;
		oss << "Test failed, differences on frame " << TEST_FRAME << '\n' << "Was: " << oldCase.current << "Supposed to be: " << newCase.current << '\n';
		ReportFailure(oss.str());
	}
	else if (final_iteration)
	{
		Con_Print("Test ran successfully.\n");
	}
}

void Write_To_File()
{
	newCase.SaveToFile();
}

void Test_Host_Frame_Hook()
{
	auto& playback = GetPlaybackInfo();
	if (tas_gamestate != unpaused || playback.current_frame < START_OFFSET)
		return;

	bool runningComparison = !Test_IsGeneratingTest();
	
	if (COLLECTING_DATA)
	{
		if (TEST_FRAME >= TEST_LENGTH)
		{
			if (runningComparison)
				Compare(true);
			else
				Write_To_File();

			COLLECTING_DATA = false;
			Test_Script_Completed_Hook();
		}
		else
		{
			newCase.GenerateFrame();
			if (runningComparison)
			{
				oldCase.Apply(TEST_FRAME);
				Compare();
			}			
		}
	}

	++TEST_FRAME;
}

static bool CheckTestConditions()
{
	if (!Test_IsRunningTest())
	{
		Con_Print("Cannot run script test outside of a test file.\n");
		return false;
	}
	else if (COLLECTING_DATA)
	{
		Con_Printf("Cannot run test, already collecting data.\n");
		return false;
	}
	
	return true;
}

void Cmd_TAS_Test_Script(void)
{
	char testfile[260];
	char script[260];

	if (!CheckTestConditions())
		return;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("Usage: tas_test_script <filepath>\n");
		return;
	}

	const char* testname = Cmd_Argv(1);
	sprintf(script, "%s/test/%s.qtas", com_gamedir, testname);
	sprintf(testfile, "%s/test/%s.qd", com_gamedir, testname);
	bool scriptLoaded = TAS_Script_Load(script);

	if (!scriptLoaded)
	{
		Con_Printf("Failed to load test.\n");
		return;
	}

	auto& playback = GetPlaybackInfo();
	int frames = playback.Get_Last_Frame() - START_OFFSET;
	COLLECTING_DATA = true;
	TEST_FRAME = 0;
	TEST_LENGTH = frames;

	if (Test_IsGeneratingTest())
	{
		newCase = TestCase(frames, testfile);
		Con_Printf("Started generating test %s\n", testfile);
	}
	else
	{
		oldCase = TestCase::LoadFromFile(testfile);
		if (oldCase.FrameCount() != frames)
		{
			Con_Printf("Old testcase has wrong framecount %s.\n", testfile);
			return;
		}

		newCase = TestCase(frames, oldCase.Filepath());
		Con_Printf("Started running test %s\n", Cmd_Argv(1));
	}

	Run_Script(-1, true, false);
}

TestCase::TestCase() { }

TestCase::TestCase(int frames, const std::string& filepath)
{
	frame_count = frames;
	path = filepath;
}

void TestCase::Apply(int frame)
{
	if(frame == 0)
		current = initial_data;
	else
		current = current.patch(deltas[frame-1]);
}

void TestCase::GenerateFrame()
{
	auto dump = Dump_Test();
	if (Test_IsGeneratingTest())
	{
		if (TEST_FRAME == 0)
		{		
			current = dump;
			initial_data = current;
		}
		else
		{
			auto delta = nlohmann::json::diff(current, dump);
			deltas.push_back(delta);
		}
	}
	current = dump;
}


void TestCase::Clear()
{
	frame_count = 0;
	deltas.clear();
}

void TestCase::SaveToFile()
{
	char buff[FILENAME_MAX];
	_getcwd(buff, FILENAME_MAX);
	Con_Printf("Writing test case to %s/%s...", buff, path.c_str());
	std::ofstream os;
	if (!Open_Stream(os, path.c_str(), std::ios::out))
	{
		Con_Printf("failed to open file %s\n", path.c_str());
		return;
	}
	os << *this;
	os.close();

	Con_Print("done.\n");
}

TestCase TestCase::LoadFromFile(char* file_name)
{
	TestCase c;
	std::ifstream is;
	if (!Open_Stream(is, file_name, std::ios::in))
	{
		Con_Printf("Couldn't open test case %s.\n", file_name);
		return c;
	}
	is >> c;
	return c;
}



std::ostream& operator<<(std::ostream& out, const TestCase& c)
{
	out << c.frame_count;
	out << c.initial_data;

	for (auto& frame : c.deltas)
		out << frame;

	return out;
}

std::istream& operator>>(std::istream& in, TestCase& c)
{
	c.Clear();
	in >> c.frame_count;
	in >> c.initial_data;

	nlohmann::json delta;
	for (int i = 0; i < c.frame_count - 1; ++i)
	{
		in >> delta;
		c.deltas.push_back(delta);
	}

	return in;
}
