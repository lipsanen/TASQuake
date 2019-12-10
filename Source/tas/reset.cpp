#include "reset.hpp"
#include "afterframes.hpp"
#include "cpp_quakedef.hpp"
#include "script.hpp"

static const char* const EXCLUDE_CVARS[] = {
	"cl_rollangle",
	"cl_bob",
	"gl_texturemode",
	"gl_consolefont",
	"gl_gamma",
	"gl_contrast",
	"tas_pause",
	"tas_playing"
};

static const char* const INCLUDE_SUBSTR[] = {
	"cl_",
	"sv_",
	"tas_",
	"gl_",
	"r_"
};

bool IsResettableCmd(cmd_function_t* func)
{
	if (func->name[0] == '-')
		return true;
	else
		return false;
}

bool IsResettableCvar(cvar_t* var)
{
	bool match = false;

	for (auto included : INCLUDE_SUBSTR)
	{
		if (strstr(var->name, included) == var->name)
		{
			match = true;
			break;
		}
	}

	if(!match)
		return false;

	for(auto excluded : EXCLUDE_CVARS)
		if (strstr(var->name, excluded) == var->name)
		{
			Con_DPrintf("%s was excluded\n", var->name);
			return false;
		}

	return true;
}


void Cmd_TAS_Reset_f(void)
{
	cmd_function_t* func = cmd_functions;
	while (func)
	{
		if (IsResettableCmd(func))
		{
			func->function();
		}
		func = func->next;
	}

	cvar_t* var = cvar_vars;

	while (var)
	{
		float f;
		if (IsResettableCvar(var))
		{
			sscanf(var->defaultvalue, "%f", &f);
			var->value = f;
		}
		var = var->next;
	}

	tas_gamestate = unpaused;
	ClearAfterframes();
	UnpauseAfterframes();
}
