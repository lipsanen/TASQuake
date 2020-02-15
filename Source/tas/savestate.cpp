
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
static int min_frame = 0;
static int max_frame = 0;
static bool in_playback = false;
static std::map<int, Savestate> savestateMap;
cvar_t tas_savestate_auto = {"tas_savestate_auto", "1"};
cvar_t tas_savestate_interval = { "tas_savestate_interval", "72" };
cvar_t tas_savestate_prior = { "tas_savestate_prior", "10" };

static bool Can_Savestate()
{
	return cl.movemessages >= 2 && !scr_disabled_for_loading && tas_gamestate != loading && tas_playing.value == 1;
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

	sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "save ss_%d", save_number);
	Cbuf_AddText(BUFFER);
	savestateMap[frame] = Savestate(frame, save_number);
	++save_number;
	max_frame = max(max_frame, frame);
}

int Savestate_Load_State(int frame)
{
	if (savestateMap.empty())
		return -1;

	auto it = savestateMap.lower_bound(frame);
	if(it == savestateMap.begin())
		return -1;
	else
	{
		--it;
		auto elem = it->second;
		static char BUFFER[80];
		sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "load ss_%d", elem.number);
		tas_gamestate = loading;
		AddAfterframes(1, "disconnect", NoFilter);
		AddAfterframes(2, BUFFER, NoFilter);

		return elem.frame;
	}
}

void Savestate_Frame_Hook(int frame)
{
	current_frame = frame;
	int interval = static_cast<int>(tas_savestate_interval.value);

	if (frame - max_frame >= interval && frame >= min_frame && tas_savestate_auto.value != 0)
		Create_Savestate(frame, false);
}

void Savestate_Script_Updated(int frame)
{
	auto it = savestateMap.lower_bound(frame);
	savestateMap.erase(it, savestateMap.end());
	
	if (savestateMap.empty())
	{
		save_number = 0;
		max_frame = 0;
		min_frame = 0;
	}
	else
	{
		auto last = savestateMap.rbegin();
		max_frame = last->first;
	}
}

void Savestate_Playback_Started(int target_frame)
{
	const int fps = 72;
	min_frame = target_frame - static_cast<int>(fps * tas_savestate_prior.value);
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

static void WriteClientEnt(std::ofstream& out, entity_t* ent, int index)
{
	Write(out, index);
	Write(out, *ent);
}

static void WriteClientEnts(std::ofstream& out)
{
	Write(out, cl.num_entities);

	for (int i = 1; i < cl.num_entities; ++i)
	{
		auto ent = &cl_entities[i];
		WriteClientEnt(out, ent, i);
	}
}

static entity_t saved_ents[MAX_EDICTS];
static int num_entities = 0;

static void ReadClientEnt(std::ifstream& in)
{
	int index;
	Read(in, index);
	entity_t ent;

	Read(in, index);
	Read(in, ent);
	saved_ents[index] = ent;
}

void Read_CL_Entities(std::ifstream& in)
{
	Read(in, num_entities);
	for (int i = 1; i < num_entities; ++i)
	{
		Read(in, cl_entities[i]);
	}
}


void Copy_Entity(int index)
{
#define COPY(prop) cl_entities[index].##prop = saved_ents[index].##prop
#define VECCOPY(prop) VectorCopy(saved_ents[index].##prop, cl_entities[index].##prop)

	VECCOPY(angles);
	VECCOPY(angles1);
	VECCOPY(angles2);
	COPY(forcelink);
	COPY(update_type);
	COPY(baseline);
	COPY(msgtime);
	VECCOPY(msg_origins[0]);
	VECCOPY(msg_origins[1]);
	VECCOPY(origin);
	VECCOPY(msg_angles[0]);
	VECCOPY(msg_angles[1]);
	VECCOPY(angles);
	COPY(frame);
	COPY(syncbase);
	COPY(effects);
	COPY(skinnum);
	COPY(visframe);
	COPY(dlightframe);
	COPY(dlightbits);
	COPY(trivial_accept);
	COPY(modelindex);
	VECCOPY(trail_origin);
	COPY(traildrawn);
	COPY(noshadow);
	COPY(frame_start_time);
	COPY(frame_interval);
	COPY(pose1);
	COPY(pose2);
	COPY(translate_start_time);
	VECCOPY(origin1);
	VECCOPY(origin2);
	COPY(rotate_start_time);
	VECCOPY(angles1);
	VECCOPY(angles2);
	COPY(transparency);
	COPY(smokepuff_time);
	COPY(istransparent);
}

void Restore_CL_Entities()
{
	for (int i = 1; i < num_entities; ++i)
	{
		auto& ent = saved_ents[i];
		if (ent.model)
		{
			Copy_Entity(i);
		}
	}
}

void Cmd_TAS_SS(void)
{
	int	i;
	char name[256];

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("tas_ss <savename> : load a game\n");
		return;
	}

	sprintf(name, "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_ForceExtension(name, ".sav");		// joe: force to ".sav"
	Con_Printf("Saving game to %s...", name);

	std::ofstream out;
	out.open(name, std::ios::binary);

	if (out.bad() || out.eof())
	{
		Con_Printf("ERROR: couldn't open\n");
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
	//Write(out, cl.viewangles);
	//WriteClientEnts(out);

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
	//Read(in, cl.viewangles);
	//Read_CL_Entities(in);

	in.close();

	if (cls.state != ca_dedicated)
	{
		CL_EstablishConnection("local");
		SCR_BeginLoadingPlaque();
		cls.signon = 0;
	}
}