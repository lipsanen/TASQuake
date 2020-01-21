#include <direct.h>
#include <fstream>
#include <map>
#include <vector>
#include <zlib.h>

#include "cpp_quakedef.hpp"

#include "test.hpp"

class DataFrame
{
	unsigned int saved_seed;
	float saved_time;
	std::map<int, edict_t> saved_edicts;

public:
	DataFrame() {}
	void BuildFrame();
	void Clear();
	bool Has_Edict(int index) const;
	const edict_t& Get_Edict(int index) const;
	bool operator==(const DataFrame& rhs) const;
	bool operator!=(const DataFrame& rhs) const;
	friend std::ostream& operator<<(std::ostream& out, const DataFrame& c);
	friend std::istream& operator>>(std::istream& in, DataFrame& c);
};

class TestCase
{
private:
	std::vector<DataFrame> data_frames;
	int frame_count;
	std::string path;

public:
	TestCase();
	TestCase(int frames, const std::string& filepath);
	int FrameCount()
	{
		return frame_count;
	}
	const std::string& Filepath()
	{
		return path;
	}
	bool GenerateFrame();
	bool operator!=(const TestCase& rhs) const;
	void Clear();
	friend std::ostream& operator<<(std::ostream& out, const TestCase& c);
	friend std::istream& operator>>(std::istream& in, TestCase& c);
	void SaveToFile();
	static TestCase LoadFromFile(char* file_name);
};

static TestCase lhsCase;
static TestCase rhsCase;
static bool collecting_data = false;
static bool generating_test = false;
static bool running_comparison = false;

void Compare()
{
	if (lhsCase != rhsCase)
	{
		Con_Printf("Test failed.\n");
	}
	else
	{
		Con_Printf("Test succeeded.\n");
	}
}

void Write_To_File()
{
	rhsCase.SaveToFile();
}

void Test_Host_Frame_Hook()
{
	if (collecting_data)
	{
		bool result = rhsCase.GenerateFrame();
		if (!result)
		{
			if (running_comparison)
				Compare();
			else if (generating_test)
				Write_To_File();
			running_comparison = false;
			generating_test = false;
			collecting_data = false;
		}
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
	rhsCase = TestCase(frames, buf);
	generating_test = true;
	collecting_data = true;
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
	lhsCase = TestCase::LoadFromFile(buf);
	if (lhsCase.FrameCount() == 0)
	{
		Con_Printf("Failed to load testcase from file %s.\n", buf);
		return;
	}

	rhsCase = TestCase(lhsCase.FrameCount(), lhsCase.Filepath());
	running_comparison = true;
	collecting_data = true;
	Con_Printf("Started running test %s\n", Cmd_Argv(2));
}

void DataFrame::BuildFrame()
{
	Clear();

	this->saved_seed = Get_RNG_Seed();
	this->saved_time = sv.time;

	for (int i = 0; i < sv.num_edicts; ++i)
	{
		edict_t* ent = EDICT_NUM(i);
		if (!ent->free && ent == sv_player)
		{
			saved_edicts[i] = *ent;
		}
	}
}

void DataFrame::Clear()
{
	this->saved_seed = 0;
	this->saved_time = 0;
	saved_edicts.clear();
}

bool DataFrame::Has_Edict(int index) const
{
	return saved_edicts.find(index) != saved_edicts.end();
}

bool EdictsEqual(const edict_t& edict1, const edict_t& edict2)
{
	for (int i = 0; i < 3; ++i)
	{
		if (edict1.v.origin[i] != edict1.v.origin[i])
		{
			Con_DPrintf("Pos difference at coordinate %d\n", i);
			return false;
		}

		if (edict1.v.velocity[i] != edict1.v.velocity[i])
		{
			Con_DPrintf("Vel difference at coordinate %d\n", i);
			return false;
		}

		if (edict1.v.angles[i] != edict2.v.angles[i])
		{
			Con_DPrintf("Angle difference at coordinate %d\n", i);
			return false;
		}
	}

	return true;
}

const edict_t& DataFrame::Get_Edict(int index) const
{
	return saved_edicts.at(index);
}

bool DataFrame::operator==(const DataFrame& other) const
{
	int edicts_size = saved_edicts.size();
	int other_edicts_size = other.saved_edicts.size();
	bool out = true;

	if (edicts_size != other_edicts_size)
	{
		Con_DPrintf("Size: %d != %d\n", edicts_size, other_edicts_size);
		out = false;
	}
	else
	{
		for (auto& entry : saved_edicts)
		{
			int key = entry.first;

			if (!other.Has_Edict(key) || !EdictsEqual(entry.second, other.Get_Edict(key)))
			{
				Con_DPrintf("Difference spotted in edict %d\n", key);
				out = false;
				break;
			}
		}
	}

	if (saved_time != other.saved_time)
	{
		Con_DPrintf("Time is different\n");
		out = false;
	}

	if (saved_seed != other.saved_seed)
	{
		out = false;
		Con_DPrintf("Seed is different\n");
	}

	return out;
}

bool DataFrame::operator!=(const DataFrame& rhs) const
{
	return !(*this == rhs);
}

TestCase::TestCase() {}

TestCase::TestCase(int frames, const std::string& filepath)
{
	frame_count = frames;
	path = filepath;
}

bool TestCase::GenerateFrame()
{
	DataFrame frame;
	frame.BuildFrame();
	data_frames.push_back(frame);

	return data_frames.size() < frame_count;
}

bool TestCase::operator!=(const TestCase& rhs) const
{
	if (data_frames.size() != rhs.data_frames.size())
	{
		Con_Printf("Test failed, data has different amount of frames.\n");
		return true;
	}

	for (int i = 0; i < data_frames.size(); ++i)
	{
		if (data_frames[i] != rhs.data_frames[i])
		{
			Con_Printf("Test failed at frame %d\n", i);
			return true;
		}
	}

	return false;
}

void TestCase::Clear()
{
	frame_count = 0;
	data_frames.clear();
}

void TestCase::SaveToFile()
{
	char buff[FILENAME_MAX];
	_getcwd(buff, FILENAME_MAX);
	Con_Printf("Writing test case to %s/%s\n", buff, path.c_str());
	std::ofstream os(path, std::ios::binary | std::ios::out);
	os << *this;
	os.close();
}

TestCase TestCase::LoadFromFile(char* file_name)
{
	TestCase c;
	std::ifstream is(file_name, std::ios::binary | std::ios::in);
	if (!is.good())
		Con_Printf("opened no good\n");
	is >> c;
	return c;
}

template<typename T>
void Read(std::istream& in, T& value, int offset = 0)
{
	char* pointer = reinterpret_cast<char*>(&value) + offset;
	in.read(pointer, sizeof(T));
}

template<typename T>
void Write(std::ostream& os, const T& value, int offset = 0)
{
	const char* pointer = reinterpret_cast<const char*>(&value) + offset;
	os.write(pointer, sizeof(T));
}

std::ostream& operator<<(std::ostream& out, const DataFrame& df)
{
	Write(out, df.saved_seed);
	Write(out, df.saved_time);
	Write(out, df.saved_edicts.size());

	for (auto& entry : df.saved_edicts)
	{
		Write(out, entry.first);

		for (int i = 0; i < sizeof(entvars_t) / 4; ++i)
		{
			const int* pointer = reinterpret_cast<const int*>(&entry.second.v) + i;
			if (*pointer != 0)
			{
				Write(out, (short)(i * 4));
				Write(out, *pointer, 0);
			}
		}

		Write(out, (short)-1);
	}

	return out;
}

std::istream& operator>>(std::istream& in, DataFrame& df)
{
	df.Clear();
	Read(in, df.saved_seed);
	Read(in, df.saved_time);
	size_t edict_count;
	Read(in, edict_count);

	edict_t dummy;
	int* pointer = reinterpret_cast<int*>(&dummy.v);
	int key;
	short offset;

	for (int i = 0; i < edict_count; ++i)
	{
		memset(&dummy, 0, sizeof(edict_t));
		Read(in, key);
		Read(in, offset);
		while (offset != -1)
		{
			if (offset > sizeof(entvars_t) || offset < 0)
			{
				int a = 0;
			}

			Read(in, *pointer, offset);
			Read(in, offset);
		}
		df.saved_edicts[key] = dummy;
	}

	return in;
}

std::ostream& operator<<(std::ostream& out, const TestCase& c)
{
	Write(out, c.frame_count);
	for (auto& frame : c.data_frames)
		out << frame;

	return out;
}

std::istream& operator>>(std::istream& in, TestCase& c)
{
	c.Clear();
	DataFrame frame;
	Read(in, c.frame_count);
	for (int i = 0; i < c.frame_count; ++i)
	{
		in >> frame;
		c.data_frames.push_back(frame);
	}

	return in;
}
