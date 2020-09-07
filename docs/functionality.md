TASQuake functionality
======================
1. [Toggles](#toggles)
2. [Commands](#commands)
3. [Console variables for TASing](#console-variables-for-tasing)


# Toggles
|Toggle|Description|
|------|-----------|
|+tas_jump|Autojump|
|+tas_lgagst|LGAGST = leave ground at air-ground speed threshold. Use in TASing to automatically jump when it's faster to strafe in the air. Also always jumps when you walk off an edge.|


# Commands
|Command|Description|
|-------|-----------|
|tas_bookmark_block|Usage: tas_bookmark_block &lt;name&gt;. Bookmarks the current block with the name given as the argument.|
|tas_bookmark_frame|Usage: tas_bookmark_frame &lt;name&gt;. Bookmarks the current frame with the name given as the argument.|
|tas_bookmark_skip|Usage: tas_bookmark_skip &lt;name&gt;. Skips to bookmark with the given name.|
|tas_cancel|Cancel change in editing mode|
|tas_cmd_reset|Resets all toggles and console variables to their default values|
|tas_confirm|Confirms change in editing mode|
|tas_edit_add_empty|Adds empty frameblock|
|tas_edit_delete|Delete current block|
|tas_edit_prune|Removes all blocks with no content|
|tas_edit_random_toggle|Usage: tas_edit_random_toggle &lt;command&gt; &lt;min&gt; &lt;max&gt;. Add a toggle at a random point in the script.|
|tas_edit_save|Usage: tas_edit_save [savename]. Saves the script to file , if used without arguments saves to the same filename where the script was loaded from|
|tas_edit_set_pitch|Enters pitch editing mode|
|tas_edit_set_view|Enters pitch/yaw editing mode|
|tas_edit_set_yaw|Enters yaw editing mode|
|tas_edit_shift|Usage: tas_edit_shift &lt;frames&gt;. Shift current frameblock by frames. Use with a negative argument to shift backwards|
|tas_edit_shift_stack|Shifts all frameblocks after current one|
|tas_edit_shrink|Removes all frameblocks after current one|
|tas_edit_strafe|Enters strafe edit mode|
|tas_edit_swim|Enters swim edit mode|
|tas_ls|Load savestate. Probably don't use this.|
|tas_print_origin|Prints origin on next physics frame|
|tas_print_vel|Prints velocity on next physics frame|
|tas_reset_movement|Resets movement related stuff|
|tas_revert|Revert changes to current editing mode to pre-frame values|
|tas_reward_delete_all|Deletes all gates|
|tas_reward_dump|Dumps rewards to IPC.|
|tas_reward_gate|Creates a reward gate|
|tas_reward_intermission|Add intermission gate|
|tas_reward_load|Load rewards from file|
|tas_reward_pop|Delete reward gate|
|tas_reward_save|Save rewards into file|
|tas_savestate|Make a manual savestate on current frame|
|tas_script_advance|Usage: tas_script_advance &lt;frames&gt;. Advances script by number of frames given as argument. Use negative values to go backwards.|
|tas_script_advance_block|Usage: tas_script_advance &lt;frames&gt;. Advances script by number of blocks given as argument. Use negative values to go backwards.|
|tas_script_init|Usage: tas_script_init &lt;filename&gt; &lt;map&gt; &lt;difficulty&gt;. Initializes a TAS script.|
|tas_script_load|Usage: tas_script_load &lt;filename&gt;. Loads a TAS script from file.|
|tas_script_play|Plays a script.|
|tas_script_skip|Usage: tas_script_skip &lt;frame&gt;. Skips to the frame number given as parameter. Use with negative values to skip to the end, e.g. -1 skips to last frame, -2 skips to second last and so on.|
|tas_script_skip_block|Usage: tas_script_skip_block &lt;block&gt;. Skips to this number of block. Works with negative numbers similarly to regular skip|
|tas_script_stop|Stop a script from playing. This is the "reset everything that the game is doing" command.|
|tas_ss_clear|Clear savestates|
|tas_test_generate|Usage: tas_test_generate &lt;filename&gt;. Generates a test from script.|
|tas_test_run|Usage: tas_test_run &lt;filename&gt;. Runs a test from file.|
|tas_test_script|Usage: tas_test_script &lt;filepath&gt;|
|tas_trace_edict|Prints the edict index that the player is looking at.|

# Console variables for TASing
|Variable|Description|
|--------|-----------|
|cl_maxfps|Use this for FPS trickery. Bounded between 10 and 72.|
|r_norefresh|Makes your screen go black.|
|r_overlay|Displays overlay|
|r_overlay_angles|The angle of the camera in the 2/3 modes.|
|r_overlay_mode|The mode of the overlay. 0-1 for lbug beams, 2 for fixed offset camera and 3 for absolute coordinates camera.|
|r_overlay_offset|The positional offset in the 2/3 modes.|
|r_overlay_pos|Determines in which corner of the screen the overlay is. Set to a number between 0 and 3.|
|r_overlay_width|The width of the overlay in pixels.|
|tas_anglespeed|How fast the player's pitch/yaw angle changes visually. This has no impact on strafing speed which works regardless of where you are looking at.|
|tas_freecam|Turns on freecam mode while paused in a TAS|
|tas_freecam_speed|Camera speed while freecamming|
|tas_hud_angles|View angles element|
|tas_hud_block|Displays block number in HUD|
|tas_hud_frame|Displays frame number in HUD|
|tas_hud_movemessages|Displays movemessages sent|
|tas_hud_particles|Displays the number of particles alive|
|tas_hud_pflags|Displays player flags in HUD|
|tas_hud_pos|Display position in HUD|
|tas_hud_pos_inc|The vertical spacing between TAS HUD elements|
|tas_hud_pos_x|X position of the HUD|
|tas_hud_pos_y|Y position of the HUD|
|tas_hud_rng|Displays the index of the RNG|
|tas_hud_state|Displays lots of TAS info about frameblocks in HUD|
|tas_hud_strafe|Displays a bar that tells you how well you are strafing.|
|tas_hud_strafeinfo|Draws output of strafe algorithm on previous frame|
|tas_hud_vel|Displays xyz-velocity in HUD|
|tas_hud_vel2d|Displays 2d speed in HUD|
|tas_hud_vel3d|Displays 3d speed in HUD|
|tas_hud_velang|Displays velocity angle in HUD|
|tas_hud_waterlevel|Displays waterlevel in HUD|
|tas_ipc|Turns on IPC.|
|tas_ipc_feedback|Turns on IPC feedback mode.|
|tas_ipc_port|IPC port|
|tas_ipc_timeout|IPC timeout in feedback mode|
|tas_ipc_verbose|Enables a bunch of debug messages in IPC|
|tas_predict|Display position prediction while paused in a TAS.|
|tas_predict_amount|Amount of time to predict|
|tas_predict_grenade|Display grenade prediction while paused in a TAS.|
|tas_predict_per_frame|How long the prediction algorithm should run per frame. High values will kill your fps.|
|tas_reward_display|Displays rewards|
|tas_reward_size|Controls the reward gate size|
|tas_savestate_auto|When set to 1, use automatic savestates in level transitions.|
|tas_savestate_enabled|Enable/disable savestates in TASes.|
|tas_strafe|Set to 1 to activate automated strafing|
|tas_strafe_maxlength|Max length of the strafe vectors on each axis|
|tas_strafe_pitch|Pitch angle to swim to. Only relevant while swimming.|
|tas_strafe_type|1 = max accel, 2 = max angle, 3 = w strafing, 4 = swimming, 5 = reverse|
|tas_strafe_yaw|Yaw angle to strafe at|
|tas_view_pitch|Player pitch.|
|tas_view_yaw|When not set to 999, sets the yaw the player should look at. When set to 999 the player will look towards the strafe yaw.|