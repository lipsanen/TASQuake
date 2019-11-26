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
	void TAS_Init();
#ifdef __cplusplus
}
#endif
