#include "ent_export.hpp"
#include "utils.hpp"
#include <fstream>


static nlohmann::json Ent_To_Json(edict_t* ent, int index)
{
#define COPY(prop) if (ent->v.prop != 0) out[#prop] = ent->v.prop;
#define VEC_COPY(prop) COPY(prop[0]) COPY(prop[1]) COPY(prop[2])

	nlohmann::json out;
	out["index"] = index;

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

nlohmann::json Ents_To_Json()
{
	nlohmann::json out;

	for (int i = 0; i < sv.num_edicts; ++i)
	{
		auto ptr = EDICT_NUM(i);
		if (!ptr->free)
		{
			out[i] = Ent_To_Json(ptr, i);
		}
	}

	return out;
}

void Cmd_TAS_Dump_Ents()
{
	if (Cmd_Argc() <= 1)
	{
		Con_Print("Usage: tas_dump_ents <filename>\n");
		return;
	}

	char name[256];
	sprintf(name, "%s/%s", com_gamedir, Cmd_Argv(1));
	COM_ForceExtension(name, ".json");

	auto json = Ents_To_Json();
	std::ofstream os;
	if (!Open_Stream(os, name))
	{
		Con_Printf("Couldn't create file with name %s\n", name);
		return;
	}

	os << json.dump(4) << std::endl;
	os.close();
}
