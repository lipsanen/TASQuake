cmake_minimum_required(VERSION 3.10)

project(TASQuake VERSION 1.4.0)

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
SET(CMAKE_C_FLAGS_DEBUG "-fsanitize=address -ggdb")
SET(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -ggdb")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

FILE(GLOB_RECURSE SOURCES
	 Source/cd_linux.c
	 Source/chase.c
	 Source/cl_demo.c
	 Source/cl_input.c
	 Source/cl_main.c
	 Source/cl_parse.c
	 Source/cl_slist.c
	 Source/cl_tent.c
	 Source/cmd.c
	 Source/common.c
	 Source/console.c
	 Source/crc.c
	 Source/cvar.c
	 Source/gl_decals.c
	 Source/gl_draw.c
	 Source/gl_mesh.c
	 Source/gl_model.c
	 Source/gl_refrag.c
	 Source/gl_rlight.c
	 Source/gl_rmain.c
	 Source/gl_rmisc.c
	 Source/gl_rpart.c
	 Source/gl_rsurf.c
	 Source/gl_screen.c
	 Source/gl_warp.c
	 Source/host.c
	 Source/host_cmd.c
	 Source/image.c
	 Source/in_sdl.c
	 Source/iplog.c
	 Source/keys.c
	 Source/mathlib.c
	 Source/menu.c
	 Source/nehahra.c
	 Source/net_bsd.c
	 Source/net_dgrm.c
	 Source/net_loop.c
	 Source/net_main.c
	 Source/net_udp.c
	 Source/net_vcr.c
	 Source/pr_cmds.c
	 Source/pr_edict.c
	 Source/pr_exec.c
	 Source/r_part.c
	 Source/sbar.c
	 Source/security.c
	 Source/snd_dma.c
	 Source/snd_mem.c
	 Source/snd_mix.c
	 Source/snd_linux.c
	 Source/sv_main.c
	 Source/sv_move.c
	 Source/sv_phys.c
	 Source/sv_user.c
	 Source/sys_linux.c
	 Source/version.c
	 Source/vid_common_gl.c
	 Source/view.c
	 Source/wad.c
	 Source/world.c
	 Source/zone.c
	 Source/tas/afterframes.cpp
	 Source/tas/bookmark.cpp
	 Source/tas/camera.cpp
	 Source/tas/data_export.cpp
	 Source/tas/drag_editing.cpp
	 Source/tas/draw.cpp
	 Source/tas/hooks.cpp
	 Source/tas/hud.cpp
	 Source/tas/ipc_main.cpp
	 Source/tas/ipc_prediction.cpp
	 Source/tas/ipc2.cpp
	 Source/tas/log.cpp
	 Source/tas/optimizer_quake.cpp
	 Source/tas/prediction.cpp
	 Source/tas/real_prediction.cpp
	 Source/tas/reset.cpp
	 Source/tas/rewards.cpp
	 Source/tas/savestate.cpp
	 Source/tas/script_playback.cpp
	 Source/tas/simulate.cpp
	 Source/tas/state_test.cpp
	 Source/tas/strafing.cpp
	 Source/tas/test_runner.cpp
	 Source/tas/utils.cpp)

add_compile_definitions(GLQUAKE __linux__)
add_compile_options(-m32 -ffp-contract=off -mfpmath=sse -Wno-write-strings -fno-omit-frame-pointer)
add_link_options(-m32 -rdynamic)
add_library(_TASQuake ${SOURCES})
target_link_libraries(_TASQuake PUBLIC GL png jpeg SDL2 libtasquake)

add_executable(TASQuake Source/game.c Source/vid_sdl.c)
target_link_libraries(TASQuake _TASQuake)

add_executable(TASQuakeSim Source/game_sim.c Source/vid_no.c)
target_link_libraries(TASQuakeSim PUBLIC _TASQuake)

enable_testing()
add_subdirectory(libtasquake)
