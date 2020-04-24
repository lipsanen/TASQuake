#include "data_export.hpp"
#include "utils.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>


static nlohmann::json Ent_To_Json(edict_t* ent)
{
#define COPY(prop) if (ent->v.prop != 0) out[#prop] = ent->v.prop;
#define VEC_COPY(prop) COPY(prop[0]) COPY(prop[1]) COPY(prop[2])

	nlohmann::json out;

	VEC_COPY(absmin);
	VEC_COPY(absmax);
	VEC_COPY(origin);
	VEC_COPY(oldorigin);
	VEC_COPY(velocity);
	VEC_COPY(angles);
	VEC_COPY(avelocity);
	VEC_COPY(punchangle);
	VEC_COPY(mins);
	VEC_COPY(maxs);
	VEC_COPY(size);
	VEC_COPY(view_ofs);
	VEC_COPY(v_angle);
	VEC_COPY(movedir);

	COPY(modelindex);
	COPY(ltime);
	COPY(movetype);
	COPY(solid);
	COPY(classname);
	COPY(model);
	COPY(frame);
	COPY(skin);
	COPY(effects);
	COPY(touch);
	COPY(use);
	COPY(think);
	COPY(blocked);
	COPY(nextthink);
	COPY(groundentity);
	COPY(health);
	COPY(frags);
	COPY(weapon);
	COPY(weaponmodel);
	COPY(weaponframe);
	COPY(currentammo);
	COPY(ammo_shells);
	COPY(ammo_nails);
	COPY(ammo_rockets);
	COPY(ammo_cells);
	COPY(items);
	COPY(takedamage);
	COPY(chain);
	COPY(deadflag);
	COPY(button0);
	COPY(button1);
	COPY(button2);
	COPY(impulse);
	COPY(fixangle);
	COPY(idealpitch);
	COPY(netname);
	COPY(enemy);
	COPY(flags);
	COPY(colormap);
	COPY(team);
	COPY(max_health);
	COPY(teleport_time);
	COPY(armortype);
	COPY(armorvalue);
	COPY(waterlevel);
	COPY(watertype);
	COPY(ideal_yaw);
	COPY(yaw_speed);
	COPY(aiment);
	COPY(goalentity);
	COPY(spawnflags);
	COPY(target);
	COPY(targetname);
	COPY(dmg_take);
	COPY(dmg_save);
	COPY(dmg_inflictor);
	COPY(owner);
	COPY(message);
	COPY(sounds);
	COPY(noise);
	COPY(noise1);
	COPY(noise2);
	COPY(noise3);

	return out;
}
static nlohmann::json Ent_To_Json_Compact(edict_t* ent)
{
	const double eps = 1e-4;

#define COPY_ROUND(prop) if (ent->v.prop != 0) out[#prop] = IRound(ent->v.prop, eps);
#define VEC_COPY_ROUND(prop) COPY_ROUND(prop[0]) COPY_ROUND(prop[1]) COPY_ROUND(prop[2])

	nlohmann::json out;

	VEC_COPY_ROUND(origin);
	VEC_COPY_ROUND(v_angle);
	COPY_ROUND(nextthink);

	return out;
}


nlohmann::json Dump_SV()
{
	const double eps = 1e-4;
	nlohmann::json ents, out;
	char BUFFER[5];

	for (int i = 0; i < sv.num_edicts; ++i)
	{
		auto ptr = EDICT_NUM(i);
		if (!ptr->free)
		{
			sprintf_s(BUFFER, 5, "%04d", i);
			ents[BUFFER] = Ent_To_Json(ptr);
		}
	}


	out["time"] = sv.time;
	out["lastcheck"] = sv.lastcheck;
	out["lastchecktime"] = sv.lastchecktime;
	out["seed"] = Get_RNG_Seed();
	out["entities"] = ents;

	return out;
}

nlohmann::json Dump_Test()
{
	const double eps = 1e-4;
	nlohmann::json out;

	out["time"] = IRound(sv.time, eps);
	out["lastchecktime"] = IRound(sv.lastchecktime, eps);
	out["seed"] = Get_RNG_Seed();

	auto ptr = EDICT_NUM(1);

	if (!ptr->free)
		out["player"] = Ent_To_Json_Compact(ptr);

	return out;
}

void Cmd_TAS_Dump_SV()
{
	if (Cmd_Argc() <= 1)
	{
		Con_Print("Usage: tas_dump_sv <filename>\n");
		return;
	}

	char name[256];
	sprintf(name, "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_ForceExtension(name, ".json");

	auto json = Dump_SV();
	std::ofstream os;
	if (!Open_Stream(os, name))
	{
		Con_Printf("Couldn't create file with name %s\n", name);
		return;
	}

	os << json.dump(4) << std::endl;
	os.close();
}
