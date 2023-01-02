#include "rewards.hpp"
#include "draw.hpp"
#include "libtasquake/utils.hpp"
#include "ipc_main.hpp"

static qboolean OnChange_tas_reward_display(cvar_t* var, char* value);
static qboolean OnChange_tas_reward_size(cvar_t* var, char* value);

cvar_t tas_reward_size = {"tas_reward_size", "25", 0 , OnChange_tas_reward_size };
cvar_t tas_reward_display = {"tas_reward_display", "1", 0 , OnChange_tas_reward_display};

struct Reward {
	vec3_t pos;
	vec3_t ang;
	bool intermission;
};

static std::vector<Reward> rewards;
static std::array<float, 4> reward_color = {255 / 255.0, 165 / 255.0, 0, 0.25};

static void Update_Drawing_Stuff(bool force=false, float size=-1.0f)
{
	if(!force && !tas_reward_display.value)
		return;

	if (size == -1.0f) {
		size = tas_reward_size.value;
	}

	size = fmax(1, size);

	RemoveRectangles(REWARD_ID);
	for (auto& reward : rewards) {
		if (!reward.intermission)
		{
			Rect r = Rect::Get_Rect(reward_color, reward.pos, reward.ang, 5, tas_reward_size.value, tas_reward_size.value, REWARD_ID);
			AddRectangle(r);
		}		
	}
}

void Cmd_TAS_Reward_Gate()
{
	Reward r;
	VectorCopy(sv_player->v.origin, r.pos);
	VectorCopy(vec3_origin, r.ang);
	r.ang[YAW] = DRound(cl.viewangles[YAW], 90);
	r.intermission = false;
	rewards.push_back(r);
	Update_Drawing_Stuff();
}

void Cmd_TAS_Reward_Pop()
{
	if (rewards.empty())
	{
		Con_Print("Rewards already empty\n");
		return;
	}

	rewards.pop_back();
	Update_Drawing_Stuff();
}

void Cmd_TAS_Reward_Delete_All()
{
	if (rewards.empty())
	{
		Con_Print("Rewards already empty\n");
		return;
	}

	rewards.clear();
	Update_Drawing_Stuff();
}

void Cmd_TAS_Reward_Intermission()
{
	Reward r;
	r.intermission = true;
	rewards.push_back(r);
}

const int EOF_TOK = 0;
const int GATE = 1;
const int INTERMISSION = 2;

void Cmd_TAS_Reward_Save()
{
	auto string = Format_String("%s/rewards/%s", com_gamedir, Cmd_Argv(1));
	std::ofstream os;
	if (!Open_Stream(os, string, std::ios::binary | std::ios::out)) {
		Con_Printf("Cannot open file %s\n", string);
		return;
	}

	for (auto& reward : rewards)
	{
		if (reward.intermission)
		{
			Write(os, INTERMISSION);
		}
		else
		{
			Write(os, GATE);
			Write(os, reward.ang[0]);
			Write(os, reward.ang[1]);
			Write(os, reward.ang[2]);

			Write(os, reward.pos[0]);
			Write(os, reward.pos[1]);
			Write(os, reward.pos[2]);
		}
	}

	Write(os, EOF_TOK);
}

void Cmd_TAS_Reward_Load()
{
	auto string = Format_String("%s/rewards/%s", com_gamedir, Cmd_Argv(1));
	std::ifstream is;
	if (!Open_Stream(is, string, std::ios::binary | std::ios::in)) {
		Con_Printf("Cannot open file %s\n", string);
		return;
	}

	rewards.clear();

	int flag;
	Read(is, flag);
	while (flag != EOF_TOK)
	{
		if (flag == INTERMISSION)
		{
			Reward r;
			r.intermission = true;
			rewards.push_back(r);
		}
		else if (flag == GATE) {
			Reward r;
			r.intermission = false;
			Read(is, r.ang[0]);
			Read(is, r.ang[1]);
			Read(is, r.ang[2]);

			Read(is, r.pos[0]);
			Read(is, r.pos[1]);
			Read(is, r.pos[2]);
			rewards.push_back(r);
		}

		Read(is, flag);
	}
	Update_Drawing_Stuff();
}

void Cmd_TAS_Reward_Dump()
{
	if (!IPC_Active())
	{
		Con_Printf("Client not connected\n");
		return;
	}

	nlohmann::json out_array;
	for (std::size_t i = 0; i < rewards.size(); ++i)
	{
		#define writeitem(item, index) out_array[i][#item "[" #index "]"] = rewards[i].item[index];

		if (rewards[i].intermission)
		{
			out_array[i] = "intermission";
		}
		else
		{
			vec3_t forward;
			AngleVectors(rewards[i].ang, forward, NULL, NULL);

			writeitem(pos, 0);
			writeitem(pos, 1);
			writeitem(pos, 2);

			out_array[i]["forward[0]"] = forward[0];
			out_array[i]["forward[1]"] = forward[1];
			out_array[i]["forward[2]"] = forward[2];
		}
	}

	IPC_Send(out_array);
}

qboolean OnChange_tas_reward_display(cvar_t* var, char* value)
{
	int val = std::atoi(value);

	if (val == 0) 
	{
		RemoveRectangles(REWARD_ID);
	}
	else
	{
		Update_Drawing_Stuff(true);
	}

	return qfalse;
}

qboolean OnChange_tas_reward_size(cvar_t* var, char* value)
{
	float f = std::atof(value);
	Update_Drawing_Stuff(false, f);
	return qfalse;
}
