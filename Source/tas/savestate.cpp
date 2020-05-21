
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

static ddef_t* ED_FieldAtOfs(int ofs)
{
	ddef_t* def;
	int	i;

	for (i = 0; i < progs->numfielddefs; i++)
	{
		def = &pr_fielddefs[i];
		if (def->ofs == ofs)
			return def;
	}

	return NULL;
}


/*
============
ED_FindField
============
*/
static ddef_t* ED_FindField(char* name)
{
	ddef_t* def;
	int	i;

	for (i = 0; i < progs->numfielddefs; i++)
	{
		def = &pr_fielddefs[i];
		if (!strcmp(pr_strings + def->s_name, name))
			return def;
	}

	return NULL;
}

static qboolean _ED_ParseEpair(void* base, ddef_t* key, char* s)
{
	int		i;
	char		string[128];
	ddef_t* def;
	char* v, * w;
	void* d;
	dfunction_t* func;
	int val;

	d = (void*)((int*)base + key->ofs);

	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t*)d = ED_NewString(s) - pr_strings;
		break;

	case ev_float:
		val = atoi(s);
		*(float*)d = *reinterpret_cast<float*>(&val);
		break;

	case ev_vector:
		strcpy(string, s);
		v = string;
		w = string;
		for (i = 0; i < 3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float*)d)[i] = atof(w);
			w = v = v + 1;
		}
		break;

	case ev_entity:
		*(int*)d = EDICT_TO_PROG(EDICT_NUM(atoi(s)));
		break;

	case ev_field:
		if (!(def = ED_FindField(s)))
		{
			// LordHavoc: don't warn about worldspawn sky/fog fields
			// because they don't require mod support
			if (strcmp(s, "sky") && strncmp(s, "fog_", 4))
				Con_Printf("Can't find field %s\n", s);
			return qfalse;
		}
		*(int*)d = G_INT(def->ofs);
		break;

	case ev_function:
		if (!(func = ED_FindFunction(s)))
		{
			Con_Printf("Can't find function %s\n", s);
			return qfalse;
		}
		*(func_t*)d = func - pr_functions;
		break;

	default:
		break;
	}

	return qtrue;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
static char* ED_ParseEdict(char* data, edict_t* ent)
{
	ddef_t* key;
	qboolean	anglehack;
	qboolean	init;
	char		keyname[256];
	int		n;

	init = qfalse;

	// clear it
	if (ent != sv.edicts)	// hack
		memset(&ent->v, 0, progs->entityfields * 4);

	// go through all the dictionary pairs
	while (1)
	{
		// parse key
		data = COM_Parse(data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error("ED_ParseEntity: EOF without closing brace");

		// anglehack is to allow QuakeEd to write single scalar angles
		// and allow them to be turned into vectors. (FIXME...)
		if (!strcmp(com_token, "angle"))
		{
			strcpy(com_token, "angles");
			anglehack = qtrue;
		}
		else
			anglehack = qfalse;

		// FIXME: change light to _light to get rid of this hack
		if (!strcmp(com_token, "light"))
			strcpy(com_token, "light_lev");	// hack for single light def

		strcpy(keyname, com_token);

		// another hack to fix keynames with trailing spaces
		n = strlen(keyname);
		while (n && keyname[n - 1] == ' ')
		{
			keyname[n - 1] = 0;
			n--;
		}

		// parse value	
		if (!(data = COM_Parse(data)))
			Sys_Error("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error("ED_ParseEntity: closing brace without data");

		init = qtrue;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		if (!(key = ED_FindField(keyname)))
		{
			Con_Printf("'%s' is not a field\n", keyname);
			continue;
		}

		if (anglehack)
		{
			char	temp[32];

			strcpy(temp, com_token);
			sprintf(com_token, "0 %s 0", temp);
		}

		if (!_ED_ParseEpair((void*)&ent->v, key, com_token))
			Host_Error("ED_ParseEdict: parse error");
	}

	if (!init)
		ent->free = qtrue;

	return data;
}

static char* _PR_UglyValueString(etype_t type, eval_t* val)
{
	static char	line[256];
	ddef_t* def;
	dfunction_t* f;

	type = static_cast<etype_t>(type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", pr_strings + val->string);
		break;

	case ev_entity:
		sprintf(line, "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;

	case ev_function:
		f = pr_functions + val->function;
		sprintf(line, "%s", pr_strings + f->s_name);
		break;

	case ev_field:
		def = ED_FieldAtOfs(val->_int);
		sprintf(line, "%s", pr_strings + def->s_name);
		break;

	case ev_void:
		sprintf(line, "void");
		break;

	case ev_float:
		sprintf(line, "%d", val->_int);
		break;

	case ev_vector:
		sprintf(line, "%d %d %d", *reinterpret_cast<int32_t*>(&val->vector[0]), *reinterpret_cast<int32_t*>(&val->vector[1]), *reinterpret_cast<int32_t*>(&val->vector[2]));
		break;

	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
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
		out << '"' << _PR_UglyValueString((etype_t)type, (eval_t *)&pr_globals[def->ofs]) << "\"\n";
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
		out << '"' << _PR_UglyValueString((etype_t)d->type, (eval_t *)v) << "\"\n";
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

static ddef_t* ED_FindGlobal(char* name)
{
	ddef_t* def;
	int	i;

	for (i = 0; i < progs->numglobaldefs; i++)
	{
		def = &pr_globaldefs[i];
		if (!strcmp(pr_strings + def->s_name, name))
			return def;
	}

	return NULL;
}

static void ED_ParseGlobals(char* data)
{
	char	keyname[64];
	ddef_t* key;

	while (1)
	{
		// parse key
		data = COM_Parse(data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error("ED_ParseEntity: EOF without closing brace");

		strcpy(keyname, com_token);

		// parse value	
		if (!(data = COM_Parse(data)))
			Sys_Error("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error("ED_ParseEntity: closing brace without data");

		if (!(key = ED_FindGlobal(keyname)))
		{
			Con_Printf("'%s' is not a global\n", keyname);
			continue;
		}

		if (!_ED_ParseEpair((void*)pr_globals, key, com_token))
			Host_Error("ED_ParseGlobals: parse error");
	}
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
	Write(out, sv.lastcheck);
	Write(out, sv.lastchecktime);

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
	int lastcheck;
	double lastchecktime;
	Read(in, seed);
	Read(in, current_skill);
	Read(in, mapname);
	double t;
	Read(in, t);
	Read(in, lastcheck);
	Read(in, lastchecktime);

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
	sv.lastcheck = lastcheck;
	sv.lastchecktime = lastchecktime;

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