
#include <map>
#include <fstream>
#include "cpp_quakedef.hpp"
#include "savestate.hpp"
#include "afterframes.hpp"
#include "utils.hpp"
#include "hooks.h"

struct Savestate
{
	Savestate(int fr, int n) : frame(fr), number(n) {}
	Savestate() {}

	int frame;
	int number;
};

static int save_number = 0;
static int current_frame = 0;
static bool in_playback = false;
static std::map<int, Savestate> savestateMap;
cvar_t tas_savestate_auto = {"tas_savestate_auto", "1"};
cvar_t tas_savestate_enabled = {"tas_savestate_enabled", "1"};

void SS(const char* savename);

static bool Can_Savestate()
{
	return cls.state == ca_connected && cls.signon == SIGNONS && tas_playing.value == 1;
}

static void Create_Savestate(int frame, bool force)
{
	static char BUFFER[80];

	if (!Can_Savestate())
	{
		if(force)
			Con_Print("Cannot savestate on current frame.\n");

		return;
	}

	if (!force)
	{
		auto it = savestateMap.find(frame);

		if(it != savestateMap.end())
			return;
	}

	sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "savestates/ss_%d", save_number);
	SS(BUFFER);
	savestateMap[frame] = Savestate(frame, save_number);
	++save_number;
}

int Savestate_Load_State(int frame)
{
	if (savestateMap.empty() || tas_savestate_enabled.value == 0)
		return -1;

	auto exact_match = savestateMap.find(frame);
	auto it = savestateMap.lower_bound(frame);

	int number;
	int savestate_frame;


	if (exact_match != savestateMap.end())
	{
		auto elem = exact_match->second;
		savestate_frame = elem.frame;
		number = elem.number;
	}
	else if (it == savestateMap.begin())
		return -1;
	else
	{
		--it;
		auto elem = it->second;
		savestate_frame = elem.frame;
		number = elem.number;
	}

	static char BUFFER[80];
	sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "tas_ls savestates/ss_%d", number);
	tas_gamestate = loading;
	AddAfterframes(1, "disconnect", NoFilter);
	AddAfterframes(2, BUFFER, NoFilter);


	return savestate_frame;
}

void Savestate_Frame_Hook(int frame)
{
	current_frame = frame;

	if (cl.movemessages == 0 && frame > 10)
		Create_Savestate(frame, false);
}

void Savestate_Script_Updated(int frame)
{
	auto it = savestateMap.lower_bound(frame);

	for (auto it = savestateMap.begin(); it != savestateMap.end(); ++it)
	{
		if (it->first > frame)
		{
			savestateMap.erase(it, savestateMap.end());
			break;
		}
	}

	if (savestateMap.empty())
	{
		save_number = 0;
	}
}

void Savestate_Playback_Started(int target_frame)
{
	const int fps = 72;
}

void Cmd_TAS_Savestate(void)
{
	Create_Savestate(current_frame, true);
}

void GrabEntity(std::ifstream& in, char* data)
{
	char c;
	Read(in, c);

	while (c != '}' && in.good())
	{
		*data = c;
		++data;
		Read(in, c);
	}

	*data = c;
	++data;
	*data = '\0';
}

static void ED_WriteGlobals(std::ofstream& out)
{
	ddef_t		*def;
	int		i, type;
	char		*name;

	for (i = 0; i < progs->numglobaldefs; i++)
	{
		def = &pr_globaldefs[i];
		type = def->type;
		if (!(def->type & DEF_SAVEGLOBAL))
			continue;
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string && type != ev_float && type != ev_entity)
			continue;

		name = pr_strings + def->s_name;
		out << '"' << name << "\" ";
		out << '"' << PR_UglyValueString((etype_t)type, (eval_t *)&pr_globals[def->ofs]) << "\"\n";
	}
	out << '}';
}

static void ED_Write(std::ofstream& out, edict_t *ed)
{
	ddef_t	*d;
	int	*v, i, j, type;
	char	*name;

	for (i = 1; i < progs->numfielddefs; i++)
	{
		d = &pr_fielddefs[i];
		name = pr_strings + d->s_name;
		//if (name[strlen(name)-2] == '_')
			//continue;	// skip _x, _y, _z vars

		v = (int *)((char *)&ed->v + d->ofs * 4);

		// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j = 0; j < type_size[type]; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;

		out << '"' << name << "\" ";
		out << '"' << PR_UglyValueString((etype_t)d->type, (eval_t *)v) << "\"\n";
	}

	out << '}';
}

static void WriteEnts(std::ofstream& out)
{
	Write(out, sv.num_edicts);

	for (int i = 0; i < sv.num_edicts; i++)
	{
		auto ent = EDICT_NUM(i);

		if (!ent->free)
		{
			Write(out, i);
			ED_Write(out, ent);
		}
	}

	Write(out, -1);
}


static void ReadEnts(std::ifstream& in, char* str)
{
	int index;

	for (int i = 0; i < MAX_EDICTS; ++i)
	{
		auto ptr = EDICT_NUM(i);
		ptr->free = qtrue;
		ptr->freetime = 0;
	}

	Read(in, sv.num_edicts);
	Read(in, index);

	while (index != -1)
	{
		auto ent = EDICT_NUM(index);
		memset(&ent->v, 0, progs->entityfields * 4);

		GrabEntity(in, str);
		ED_ParseEdict(str, ent);
		ent->free = qfalse;

		SV_LinkEdict(ent, qfalse);
		Read(in, index);
	}
}

static entity_t saved_ents[MAX_EDICTS];
static int num_entities = 0;
static client_state_t cl_backup;

static void WriteClient(std::ofstream& out)
{
	for (int i = 1; i < cl.num_entities; ++i)
	{
		if (cl_entities[i].model)
		{
			Write(out, i);
			Write(out, cl_entities[i]);
		}
		
	}
	Write(out, -1);
	Write(out, cl);
}

void Read_Client(std::ifstream& in)
{
	for (int i = 1; i < MAX_EDICTS; ++i)
	{
		saved_ents[i].model = NULL;
	}

	int index;
	Read(in, index);
	while (index != -1)
	{
		Read(in, saved_ents[index]);
		Read(in, index);
	}

	Read(in, cl_backup);
}


void Copy_Entity(int index)
{
#define COPY(prop) cl_entities[index].##prop = saved_ents[index].##prop
#define VECCOPY(prop) VectorCopy(saved_ents[index].##prop, cl_entities[index].##prop)
	VECCOPY(angles);
	VECCOPY(angles1);
	VECCOPY(angles2);
	VECCOPY(msg_origins[0]);
	VECCOPY(msg_origins[1]);
	VECCOPY(origin);
	VECCOPY(origin1);
	VECCOPY(origin2);
	VECCOPY(msg_angles[0]);
	VECCOPY(msg_angles[1]);
	VECCOPY(trail_origin);

	COPY(forcelink);
	COPY(update_type);
	COPY(baseline);
	COPY(msgtime);
	COPY(frame);
	COPY(syncbase);
	COPY(effects);
	COPY(skinnum);
	COPY(visframe);
	COPY(dlightframe);
	COPY(dlightbits);
	COPY(trivial_accept);
	COPY(modelindex);
	COPY(traildrawn);
	COPY(noshadow);
	COPY(frame_start_time);
	COPY(frame_interval);
	COPY(pose1);
	COPY(pose2);
	COPY(translate_start_time);
	COPY(rotate_start_time);
	COPY(transparency);
	COPY(smokepuff_time);
	COPY(istransparent);
}

void Restore_Client()
{
	for (int i = 1; i < MAX_EDICTS; ++i)
	{
		auto& ent = saved_ents[i];
		if (ent.model)
		{
			Copy_Entity(i);
		}
	}

#define COPY2(prop) cl.##prop = cl_backup.##prop
#define VECCOPY2(prop) VectorCopy(cl_backup.##prop, cl.##prop)

	COPY2(cdtrack);
	COPY2(cmd);
	COPY2(completed_time);
	COPY2(console_ping);
	COPY2(console_status);
	COPY2(ctime);
	COPY2(driftmove);
	COPY2(faceanimtime);
	COPY2(gametype);
	COPY2(idealpitch);
	COPY2(intermission);
	COPY2(inwater);
	COPY2(items);
	COPY2(laststop);
	COPY2(last_ping_time);
	COPY2(last_received_message);
	COPY2(last_status_time);
	COPY2(looptrack);
	COPY2(maxclients);
	COPY2(movemessages);
	COPY2(mtime[0]);
	COPY2(mtime[1]);
	COPY2(nodrift);
	COPY2(num_entities);
	COPY2(num_statics);
	COPY2(oldtime);
	COPY2(onground);
	COPY2(paused);
	COPY2(pitchvel);
	COPY2(prev_cshifts[0]);
	COPY2(prev_cshifts[1]);
	COPY2(prev_cshifts[2]);
	COPY2(prev_cshifts[3]);
	COPY2(time);
	COPY2(viewheight);

	VECCOPY2(mvelocity[0]);
	VECCOPY2(mvelocity[1]);
	VECCOPY2(mviewangles[0]);
	VECCOPY2(mviewangles[1]);
	VECCOPY2(mviewangles[1]);
	VECCOPY2(prev_viewangles);
	VECCOPY2(punchangle);
	VECCOPY2(velocity);
	VECCOPY2(viewangles);
}

void SS(const char* savename)
{
	int	i;
	char name[256];

	sprintf(name, "%s/%s", com_gamedir, savename);
	COM_ForceExtension(name, ".sav");		// joe: force to ".sav"
	Con_Printf("Saving game to %s...", name);

	std::ofstream out;

	if (!Open_Stream(out, name, std::ios::binary | std::ios::out))
	{
		Con_Printf("ERROR: couldn't open file %s\n", name);
		return;
	}

	Write(out, Get_RNG_Seed());
	Write(out, current_skill);
	Write(out, sv.name);
	Write(out, sv.time);

	// write the light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		WriteString(out, sv.lightstyles[i]);
	}

	ED_WriteGlobals(out);
	WriteEnts(out);
	Write(out, svs.clients->spawn_parms);
	WriteClient(out);

	out.close();
	Con_Printf("done.\n");
}

void Cmd_TAS_LS(void)
{
	char	name[MAX_OSPATH], mapname[MAX_QPATH], str[32768];
	int	i;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("tas_ls <savename> : load a game\n");
		return;
	}

	cls.demonum = -1;		// stop demo loop in case this fails

	sprintf(name, "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension(name, ".sav");
	Con_Printf("Loading game from %s...\n", name);

	std::ifstream in;
	in.open(name, std::ios::binary);

	if (in.bad() || in.eof())
	{
		Con_Printf("ERROR: couldn't open\n");
		return;
	}

	unsigned int seed;
	Read(in, seed);
	Read(in, current_skill);
	Read(in, mapname);
	double t;
	Read(in, t);

	TAS_Set_Seed(seed);
	Cvar_SetValue(&skill, (float)current_skill);

	CL_Disconnect_f();
	SV_SpawnServer(mapname);
	
	if (!sv.active)
	{
		Con_Printf("Couldn't load map\n");
		return;
	}
	sv.paused = qtrue;		// pause until all clients connect
	sv.loadgame = qtrue;
	tas_gamestate = loading;
	sv.time = t;

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		ReadString(in, str);
		sv.lightstyles[i] = (char*)Hunk_Alloc(strlen(str) + 1);
		strcpy(sv.lightstyles[i], str);
	}

	GrabEntity(in, str);
	ED_ParseGlobals(str);
	ReadEnts(in, str);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		Read(in, svs.clients->spawn_parms[i]);
	Read_Client(in);

	in.close();

	if (cls.state != ca_dedicated)
	{
		CL_EstablishConnection("local");
		SCR_BeginLoadingPlaque();
		cls.signon = 0;
	}
}