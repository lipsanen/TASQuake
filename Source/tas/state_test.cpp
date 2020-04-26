#include <direct.h>
#include <fstream>
#include <map>
#include <vector>
#include <zlib.h>

#include "cpp_quakedef.hpp"
#include "data_export.hpp"
#include "utils.hpp"
#include "state_test.hpp"
#include "reset.hpp"
#include "test_runner.hpp"

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
static int test_frame = 0;
static bool collecting_data = false;
static bool generating_test = false;
static bool running_comparison = false;

void Compare(bool final_iteration=false)
{
	if (oldCase.current != newCase.current)
	{
		auto patch = nlohmann::json::diff(newCase.current, oldCase.current).dump(4);
		Con_Printf("Test failed, differences on frame %d\n", test_frame);
		Con_Print(const_cast<char*>(patch.c_str()));
		running_comparison = false;
		generating_test = false;
		collecting_data = false;
		Cmd_TAS_Cmd_Reset();
		sv.paused = qtrue;
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
	if (tas_gamestate != unpaused)
		return;

	if (collecting_data)
	{
		newCase.GenerateFrame();
		if (test_frame >= newCase.FrameCount() - 1)
		{
			if (running_comparison)
				Compare(true);
			else if (generating_test)
				Write_To_File();

			running_comparison = false;
			generating_test = false;
			collecting_data = false;
		}
		else
		{
			if (running_comparison)
			{
				oldCase.Apply(test_frame);
				Compare();
			}
				
		}
		++test_frame;
	}
}

void Cmd_GenerateTest(void)
{
	char buf[260];

	if (collecting_data)
	{
		Con_Printf("Cannot generate test, already collecting data.\n");
		return;
	}

	if (Cmd_Argc() < 3)
	{
		Con_Printf("Usage: tas_generate_test <filepath> <frames>\n");
	}

	sprintf(buf, "%s/test/", com_gamedir);
	_mkdir(buf);
	sprintf(buf, "%s/test/%s.qd", com_gamedir, Cmd_Argv(1));

	int frames = std::atoi(Cmd_Argv(2));
	newCase = TestCase(frames, buf);
	generating_test = true;
	collecting_data = true;
	test_frame = 0;
	Con_Printf("Started generating test %s\n", buf);
}

void Cmd_Run_Test(void)
{
	char buf[260];

	if (collecting_data)
	{
		Con_Printf("Cannot run test, already collecting data.\n");
		return;
	}

	if (Cmd_Argc() < 3)
	{
		Con_Printf("Usage: tas_run_test <filepath> <test name>\n");
	}

	sprintf(buf, "%s/test/%s.qd", com_gamedir, Cmd_Argv(1));
	oldCase = TestCase::LoadFromFile(buf);
	if (oldCase.FrameCount() == 0)
	{
		Con_Printf("Failed to load testcase from file %s.\n", buf);
		return;
	}

	newCase = TestCase(oldCase.FrameCount(), oldCase.Filepath());
	running_comparison = true;
	collecting_data = true;
	test_frame = 0;
	Con_Printf("Started running test %s\n", Cmd_Argv(2));
}

TestCase::TestCase() {}

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
		current.patch(deltas[frame-1]);
}

void TestCase::GenerateFrame()
{
	auto dump = Dump_SV();
	if (test_frame == 0)
	{
		current = dump;
		initial_data = current;
	}
	else
	{
		auto delta = nlohmann::json::diff(current, dump);
		deltas.push_back(delta);
		current = dump;
	}
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
