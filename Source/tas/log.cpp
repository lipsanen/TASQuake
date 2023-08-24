#include "log.hpp"
#include "script_playback.hpp"
#include <execinfo.h>

static qboolean TAS_Log_Updated(struct cvar_s *var, char *value);

cvar_t TASQuake::tas_log = {"tas_log", "0", 0, TAS_Log_Updated};
cvar_t TASQuake::tas_log_filter = {"tas_log_filter", "all", 0, TAS_Log_Updated};

static bool check_log_parameters = true;
static bool logging = false;
static bool log_player_pos = false;
static bool log_rng = false;
static bool log_rng_index = false;

static qboolean TAS_Log_Updated(struct cvar_s *var, char *value)
{
    check_log_parameters = true;
    return qfalse;
}

static void Check_Params()
{
    logging = TASQuake::tas_log.value != 0;
    log_player_pos = false;
    log_rng = false;
    log_rng_index = false;

    if(!logging)
    {
        return;
    }

    check_log_parameters = false;
    const char* filter_string = TASQuake::tas_log_filter.string;

    if(strstr(filter_string, "all") != NULL)
    {
        log_player_pos = true;
        log_rng = true;
        log_rng_index = true;
    }

    if(strstr(filter_string, "index") != NULL)
    {
        log_rng_index = true;
    }

    if(strstr(filter_string, "player") != NULL)
    {
        log_player_pos = true;
    }
}

extern "C"
{
    void tas_rand_Hook()
    {
        if(log_rng)
        {
            auto* info = GetPlaybackInfo();
            void* array[10];
            size_t size;
            char **strings;

            size = backtrace(array, 10);
            strings = backtrace_symbols(array, size);

            Con_Printf("[%d] tas_rand call\n", info->current_frame);
            if(strings == NULL) {
                Con_Printf("no symbols for tas_rand backtrace\n");
            } else {
                for(size_t i=0; i < size; ++i) {
                    for(size_t j=0; j < i+1; ++j) {
                        Con_Printf("\t");
                    }
                    Con_Printf("%s\n", strings[size-i-1]);
                }
            }

            free(strings);
        }
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

    if(log_rng_index)
    {
        Con_Printf("[%d] RNG %d\n", info->current_frame, Get_RNG_Seed());
    }

}
