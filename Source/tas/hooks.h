#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef QUAKE_GAME
#include "..\quakedef.h"
#endif
	extern cvar_t tas_playing;
	extern cvar_t tas_timescale;

	void SV_Physics_Client_Hook();
	void CL_SendMove_Hook(usercmd_t* cmd);
	void _Host_Frame_Hook();
	void _Host_Frame_After_FilterTime_Hook();
	void Host_Connect_f_Hook();
	void CL_SignonReply_Hook();
	void TAS_Init();
	void TAS_Set_Seed(unsigned int seed);
	qboolean Cmd_ExecuteString_Hook(const char* text);
	void IN_Move_Hook(usercmd_t* cmd);
	void SCR_Draw_TAS_HUD_Hook(void);
	void Draw_Lines_Hook(void);
#ifdef __cplusplus
}
#endif
