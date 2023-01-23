#include "cpp_quakedef.hpp"

#include "reset.hpp"

#include "afterframes.hpp"
#include "libtasquake/utils.hpp"

static const char* const EXCLUDE_CVARS[] = {"gl_texturemode",
                                            "gl_consolefont",
                                            "gl_gamma",
                                            "gl_contrast",
                                            "tas_pause",
                                            "tas_playing",
                                            "cl_independentphysics",
                                            "tas_hud",
                                            "tas_edit",
                                            "tas_script",
                                            "tas_timescale",
                                            "r_overlay",
                                            "tas_predict",
											"tas_savestate",
											"r_wateralpha",
											"r_fullbright",
											"tas_ipc",
											"tas_reward",
											"tas_freecam",
											"tas_optimizer"};

static const char* const INCLUDE_SUBSTR[] = {"cl_", "sv_", "tas_", "gl_", "r_", "v_centerspeed"};

static const char* const MOVEMENT_COMMANDS[] = {"forward", "back", "moveleft", "moveright", "moveup", "movedown"};

bool IsUpCmd(cmd_function_t* func)
{
	if (func->name[0] == '-')
		return true;
	else
		return false;
}

bool IsDownCmd(const char* text)
{
	auto cmd = Cmd_FindCommand(const_cast<char*>(text));
	return cmd && IsDownCmd(cmd);
}

bool IsDownCmd(cmd_function_t* func)
{
	if (func->name[0] == '+')
		return true;
	else
		return false;
}

bool IsGameplayCvar(cvar_t* var)
{
	if (!var->defaultvalue || !var->defaultvalue[0])
		return false;

	bool match = false;

	for (auto included : INCLUDE_SUBSTR)
	{
		if (strstr(var->name, included) == var->name)
		{
			match = true;
			break;
		}
	}

	if (!match)
		return false;

	for (auto excluded : EXCLUDE_CVARS)
		if (strstr(var->name, excluded) == var->name)
		{
			Con_DPrintf("%s was excluded\n", var->name);
			return false;
		}

	return true;
}

bool IsGameplayCvar(const char* text)
{
	auto cvar = Cvar_FindVar(const_cast<char*>(text));
	return cvar && IsGameplayCvar(cvar);
}

bool IsMovementCommand(const char* text)
{
	if (!IsDownCmd(text) && !IsUpCmd(text))
		return false;

	text += 1;

	bool match = false;

	for (auto included : MOVEMENT_COMMANDS)
	{
		if (strstr(text, included) == text)
		{
			match = true;
			break;
		}
	}

	return match;
}

bool IsUpCmd(const char* text)
{
	auto cmd = Cmd_FindCommand(const_cast<char*>(text));
	return cmd && IsUpCmd(cmd);
}

void Cmd_TAS_Cmd_Reset(void)
{
	tas_gamestate = unpaused;
	sv.paused = qfalse;
	VectorCopy(vec3_origin, cl.viewangles);
	ClearAfterframes();
	UnpauseAfterframes();

	cmd_function_t* func = cmd_functions;
	while (func)
	{
		if (IsUpCmd(func))
		{
			Cmd_ExecuteString(func->name, src_command);
		}
		func = func->next;
	}

	cvar_t* var = cvar_vars;
	char BUFFER[256];

	while (var)
	{
		if (IsGameplayCvar(var))
		{
			snprintf(BUFFER, 256, "%s %s", var->name, var->defaultvalue);
			Cmd_ExecuteString(BUFFER, src_command);
		}
		var = var->next;
	}
}

void Cmd_TAS_Reset_Movement(void)
{
	cmd_function_t* func = cmd_functions;
	while (func)
	{
		if (IsUpCmd(func))
		{
			func->function();
		}
		func = func->next;
	}

	cvar_t* var = cvar_vars;

	while (var)
	{
		float f;
		if (strstr(var->name, "tas_") == var->name)
		{
			sscanf(var->defaultvalue, "%f", &f);
			var->value = f;
		}
		var = var->next;
	}
}
