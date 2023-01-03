#include "ipc_main.hpp"
#include "libtasquake/ipc.hpp"
#include "afterframes.hpp"
#include "simulate.hpp"
#include "libtasquake/utils.hpp"

struct IPCCondition
{
	int index;
	vec3_t mins;
	vec3_t maxs;
	bool running_condition;

	IPCCondition()
	{
		running_condition = false;
	}

};

static qboolean OnChange_tas_ipc(cvar_t* var, char* string);
static ipc::IPCServer server;

cvar_t tas_ipc = { "tas_ipc", "0", 0, OnChange_tas_ipc };
cvar_t tas_ipc_feedback = { "tas_ipc_feedback", "0", 0  };
cvar_t tas_ipc_port = { "tas_ipc_port", "10001"};
cvar_t tas_ipc_verbose = { "tas_ipc_verbose", "1"};
cvar_t tas_ipc_timeout = { "tas_ipc_timeout", "200"};
constexpr float MAX_TIMEOUT = 20000;
static bool running_sim = false;
static IPCCondition ipc_condition;
static Simulator sim;

static void SendConditionResult(bool result)
{
	ipc_condition.running_condition = false;
	const PlaybackInfo* playback = GetPlaybackInfo();
	nlohmann::json msg;
	msg["type"] = "condition";
	msg["result"] = result;
	msg["frame"] = playback->current_frame;
	IPC_Send(msg);
}


static void ConditionIteration()
{
	if (!IPC_Active() || !ipc_condition.running_condition)
	{
		return;
	}

	int n = ipc_condition.index;

	if (n < sv.num_edicts || n < 0)
	{
		edict_t* ent = EDICT_NUM(n);

		if (ent->free == qtrue || ent->v.health <= 0)
		{
			SendConditionResult(false);
		}
		else
		{
			bool inbounds = true;
			for (int i = 0; i < 3; ++i)
			{
				inbounds = inbounds && InBounds(ent->v.origin[i], ipc_condition.mins[i], ipc_condition.maxs[i]);
			}

			if (inbounds)
			{
				SendConditionResult(true);
			}
		}
	}
}

void Cmd_TAS_IPC_Condition_Disable()
{
	if (!IPC_Active())
	{
		Con_Print("IPC not active.\n");
		return;
	}
	else if (!ipc_condition.running_condition)
	{
		Con_Print("Condition not active.\n");
		return;
	}

	SendConditionResult(false);
}

void Cmd_TAS_IPC_Condition()
{
	if (!IPC_Active())
	{
		Con_Print("IPC not active.\n");
		return;
	}
	else if(Cmd_Argc() < 8)
	{
		Con_Print("Usage: tas_ipc_condition <edict> <x1> <y1> <z1> <x2> <y2> <z2>\n");
		return;
	}
	
	ipc_condition.running_condition = true;
	ipc_condition.index = std::atoi(Cmd_Argv(1));
	float x1 = std::atof(Cmd_Argv(2));
	float y1 = std::atof(Cmd_Argv(3));
	float z1 = std::atof(Cmd_Argv(4));
	float x2 = std::atof(Cmd_Argv(5));
	float y2 = std::atof(Cmd_Argv(6));
	float z2 = std::atof(Cmd_Argv(7));

	ipc_condition.mins[0] = fmin(x1, x2);
	ipc_condition.mins[1] = fmin(y1, y2);
	ipc_condition.mins[2] = fmin(z1, z2);
	ipc_condition.maxs[0] = fmax(x1, x2);
	ipc_condition.maxs[1] = fmax(y1, y2);
	ipc_condition.maxs[2] = fmax(z1, z2);
}


static void IPC_Print(const char* string)
{
	if (tas_ipc_verbose.value != 0)
	{
		char* str = const_cast<char*>(string);
		Con_Printf("IPC: %s", str);
	}
}

void Cmd_TAS_IPC_Print_Seed()
{
	if (!IPC_Active())
		return;

	nlohmann::json data;
	data["type"] = "seed";
	data["seed"] = Get_RNG_Seed();
	IPC_Send(data);
}

void Cmd_TAS_IPC_Print_Posvel()
{
	if(!IPC_Active())
		return;

	nlohmann::json data;
	data["type"] = "playerdata";
	data["pos[0]"] = sv_player->v.origin[0];
	data["pos[1]"] = sv_player->v.origin[1];
	data["pos[2]"] = sv_player->v.origin[2];
	data["vel[0]"] = sv_player->v.velocity[0];
	data["vel[1]"] = sv_player->v.velocity[1];
	data["vel[2]"] = sv_player->v.velocity[2];
	data["map"] = sv.name;
	IPC_Send(data);
}

void Cmd_TAS_IPC_Print_Playback()
{
	if (!IPC_Active())
		return;

	auto playback = GetPlaybackInfo();
	nlohmann::json data;
	data["type"] = "playback";
	data["frame"] = playback->current_frame;
	data["pause_frame"] = playback->pause_frame;
	data["script_running"] = playback->script_running;
	IPC_Send(data);
}

static void Cmd_Callback(const nlohmann::json& msg)
{
	if (msg.find("cmd") != msg.end())
	{
		std::string cmd = msg["cmd"];
		if (running_sim)
		{
			sim.RunFrame(cmd);
		}
		else
		{
			AddAfterframes(0, cmd.c_str(), NoFilter);
		}
	}
	else
	{
		IPC_Print("Message contained no cmd field!\n");
	}
}

static void Start_IPC()
{
	if (!ipc::Winsock_Initialized())
	{
		server.InitWinsock();
		server.StartListening(tas_ipc_port.string);
	}
}

static void Shutdown_IPC()
{
	if (ipc::Winsock_Initialized())
	{
		server.CloseConnections();
		ipc::Shutdown_IPC();
	}
}

static qboolean OnChange_tas_ipc(cvar_t* var, char* string)
{
	int a = std::atoi(string);

	if (a != 0) {
		Start_IPC();
		IPC_Print("Started.\n");
	}
	else
	{
		Shutdown_IPC();
		IPC_Print("Shut down.\n");
	}

	return qfalse;
}

static void Sim_Feedback()
{
	nlohmann::json playerData;
	playerData["type"] = "playerdata";
	playerData["pos[0]"] = sim.info.ent.v.origin[0];
	playerData["pos[1]"] = sim.info.ent.v.origin[1];
	playerData["pos[2]"] = sim.info.ent.v.origin[2];
	playerData["vel[0]"] = sim.info.ent.v.velocity[0];
	playerData["vel[1]"] = sim.info.ent.v.velocity[1];
	playerData["vel[2]"] = sim.info.ent.v.velocity[2];
	IPC_Send(playerData);
}

void Cmd_TAS_IPC_Simulate()
{
	if (!IPC_Active())
	{
		IPC_Print("No active IPC connection\n");
		return;
	}

	sim = Simulator::GetSimulator();
	running_sim = true;
	int timeout = static_cast<int>(bound(1, tas_ipc_timeout.value, MAX_TIMEOUT));

	while (running_sim)
	{
		Sim_Feedback();
		bool response = server.BlockForMessages("sim_response", timeout);
		if (!response) 
			break;
	}
}

static void Sim_Callback(const nlohmann::json& msg)
{
	if (msg.find("cmd") != msg.end())
	{
		std::string cmd = msg["cmd"];
		sim.RunFrame(cmd);
	}
	else if (msg.find("terminate") != msg.end())
	{
		running_sim = false;
	}
	else
	{
		IPC_Print("Message contained no appropriate fields!\n");
		running_sim = false;
	}
}

void IPC_Init()
{
	if (tas_ipc.value != 0) {
		Start_IPC();
	}
	server.AddCallback("cmd", Cmd_Callback, false);
	server.AddCallback("response", Cmd_Callback, true);
	server.AddCallback("sim_response", Sim_Callback, true);
	server.AddPrintFunc(IPC_Print);
}

bool IPC_Active()
{
	return ipc::Winsock_Initialized() && server.ClientConnected();
}

static void Feedback()
{
	nlohmann::json playerData;
	playerData["type"] = "playerdata";
	playerData["pos[0]"] = sv_player->v.origin[0];
	playerData["pos[1]"] = sv_player->v.origin[1];
	playerData["pos[2]"] = sv_player->v.origin[2];
	playerData["vel[0]"] = sv_player->v.velocity[0];
	playerData["vel[1]"] = sv_player->v.velocity[1];
	playerData["vel[2]"] = sv_player->v.velocity[2];
	IPC_Send(playerData);

	int timeout = static_cast<int>(bound(1, tas_ipc_timeout.value, MAX_TIMEOUT));
	bool result = server.BlockForMessages("response", timeout);
	if (!result) {
		tas_ipc_feedback.value = 0;
		Con_Print("Response timed out, turning off feedback mode.\n");
	}
}

void IPC_Loop()
{
	if (ipc::Winsock_Initialized()) {
		ConditionIteration();
		server.Loop();
		if (server.ClientConnected() && tas_ipc_feedback.value != 0 && sv.active) {
			Feedback();
		}
	}
}

void IPC_Send(const nlohmann::json& msg)
{
	if (IPC_Active()) {
		server.SendMsg(msg);
	}
}
