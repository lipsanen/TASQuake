#pragma once



#ifdef __cplusplus
extern "C"
{
#endif
#ifndef QUAKE_GAME
#include "..\quakedef.h"
#endif
	void SV_Physics_Client_Hook();
	void CL_SendMove_Hook(usercmd_t* cmd);
	void _Host_Frame_Hook();
	void Host_Connect_f_Hook();
	void CL_SignonReply_Hook();
	void TAS_Init();
	void TAS_Set_Seed(unsigned int seed);
#ifdef __cplusplus
}
#endif
