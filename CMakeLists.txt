cmake_minimum_required(VERSION 3.10)

project(TASQuake VERSION 1.4.0)

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

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
	 Source/vid_sdl.c
	 Source/view.c
	 Source/wad.c
	 Source/world.c
	 Source/zone.c
	 Source/*.cpp)

add_executable(TASQuake ${SOURCES})
add_compile_definitions(GLQUAKE __linux__)
target_link_libraries(TASQuake PUBLIC GL dl m png jpeg SDL2)
set_target_properties(TASQuake PROPERTIES COMPILE_FLAGS "-m32 -ffp-contract=off -ffloat-store -Wno-write-strings" LINK_FLAGS "-m32")
set_target_properties(TASQuake PROPERTIES LINKER_LANGUAGE CXX)
