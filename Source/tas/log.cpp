#include "log.hpp"
#include "script_playback.hpp"

static qboolean TAS_Log_Updated(struct cvar_s *var, char *value);

cvar_t TASQuake::tas_log = {"tas_log", "0", 0, TAS_Log_Updated};
cvar_t TASQuake::tas_log_filter = {"tas_log_filter", "all", 0, TAS_Log_Updated};

static bool check_log_parameters = true;
static bool logging = false;
static bool log_player_pos = false;
static bool log_rng = false;

static qboolean TAS_Log_Updated(struct cvar_s *var, char *value)
{
    check_log_parameters = true;
    return qfalse;
}

static void Check_Params()
{
    logging = TASQuake::tas_log.value != 0;
    check_log_parameters = false;
    const char* filter_string = TASQuake::tas_log_filter.string;

    if(strstr(filter_string, "all") != NULL)
    {
        log_player_pos = true;
        log_rng = true;
    }
}

void TASQuake::Log_Frame_Hook()
{
    auto* info = GetPlaybackInfo();

    if(check_log_parameters)
    {
        Check_Params();
    }

    if(!info->script_running || !logging)
    {
        return;
    }

    if(log_player_pos && sv_player)
    {
        Con_Printf("[%d] player (%f, %f, %f)\n", info->current_frame, sv_player->v.origin[0], sv_player->v.origin[1], sv_player->v.origin[2]);
    }

    if(log_rng)
    {
        Con_Printf("[%d] RNG %d\n", info->current_frame, Get_RNG_Index());
    }

}
