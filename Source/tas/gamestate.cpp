/*

	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);
	fprintf (f, "%d\n", current_skill);
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n", sv.time);

// write the light styles

	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (sv.lightstyles[i])
			fprintf (f, "%s\n", sv.lightstyles[i]);
		else
			fprintf (f,"m\n");
	}


	ED_WriteGlobals (f);
	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ED_Write (f, EDICT_NUM(i));
		fflush (f);
	}

*/
#include "cpp_quakedef.hpp"
class DataFrame
{
	char* data;
	size_t size;
public:
	DataFrame() { data = NULL; size = 0; }
	void BuildFrame();
	void Discard();
	~DataFrame();
	bool operator==(const DataFrame& other);
};

static DataFrame savedata;





static char* GetData()
{

}

static bool Compare(char* data1, char* data2, int size)
{
	for (int i = 0; i < size; ++i)
	{
		if (data1[i] != data2[i])
			return false;
	}

	return true;
}

void Cmd_SaveState(void)
{
	savedata.BuildFrame();
}

void Cmd_TestState(void)
{
	auto data = DataFrame();
	data.BuildFrame();
	bool compare = data == savedata;
	Con_Printf("Test: %d\n", compare);
}

void DataFrame::BuildFrame()
{
	Discard();
	int index = 0;
	int edicts = sv.num_edicts;
	size = sizeof(edict_t) * edicts;
	size += NUM_SPAWN_PARMS * sizeof(float);
	size += sizeof(float) + sizeof(int) + sizeof(int);

	data = new char[size];

	unsigned int seed = Get_RNG_Seed();
	memcpy(data + index, &seed, sizeof(int));
	index += sizeof(int);

	for (int i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		memcpy(data + index, &svs.clients->spawn_parms[i], sizeof(float));
		index += sizeof(float);
	}

	memcpy(data + index, &current_skill, sizeof(int));
	index += sizeof(int);
	memcpy(data + index, &sv.time, sizeof(float));
	index += sizeof(float);

	for (int i = 0; i < sv.num_edicts; ++i)
	{
		edict_t* ent = EDICT_NUM(i);
		memcpy(data + index, ent, sizeof(edict_t));
		index += sizeof(edict_t);
	}
}

void DataFrame::Discard()
{
	if (data)
		delete[] data;
}

DataFrame::~DataFrame()
{
	Discard();
}

bool DataFrame::operator==(const DataFrame & other)
{
	if (size != other.size)
	{
		Con_Printf("Size: %d != %d\n", size, other.size);
		return false;
	}

	for (int i = 0; i < size; ++i)
	{
		if (data[i] != other.data[i])
		{
			Con_Printf("Difference found at index %d\n", i);
			return false;
		}

	}

	return true;
}
