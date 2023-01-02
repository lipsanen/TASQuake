#include "cpp_quakedef.hpp"

int GetPlayerWeaponDelay()
{
	switch (static_cast<int>(sv_player->v.weapon))
	{
		case IT_AXE:
			return 36;
		case IT_SHOTGUN:
			return 36;
		case IT_SUPER_SHOTGUN:
			return 51;
		case IT_NAILGUN: case IT_SUPER_NAILGUN:
			return 8;
		case IT_GRENADE_LAUNCHER:
			return 44;
		case IT_ROCKET_LAUNCHER:
			return 58;
		case IT_LIGHTNING:
			return 8;
		default:
			return 36;
	}
}

void Cmd_TAS_Trace_Edict()
{
	vec3_t start;
	vec3_t end;

	VectorCopy(sv_player->v.origin, start);
	AngleVectors(sv_player->v.angles, end, NULL, NULL);
	VectorScale(end, 1000, end);
	VectorAdd(end, sv_player->v.origin, end);
	trace_t result = SV_Move(start, vec3_origin, vec3_origin, end, MOVE_NORMAL, sv_player);
	if (result.fraction < 1)
	{
		int index = NUM_FOR_EDICT(result.ent);
		Con_Printf("Edict is %d\n", index);
	}
	else
	{
		Con_Print("Trace hit no edicts.\n");
	}
}

void CenterPrint(const char* value, ...)
{
	va_list args;
	va_start(args, value);
	static char BUFFER[80];
	vsprintf(BUFFER, value, args);
	SCR_CenterPrint(BUFFER);
}

float Get_Default_Value(const char* name)
{
	float f;
	auto var = Cvar_FindVar(const_cast<char*>(name));
	if (!var)
		return 0.0f;

	sscanf(var->defaultvalue, "%f", &f);

	return f;
}

