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
|tas_add_empty|Adds empty frameblock|
|tas_bookmark_block|Usage: tas_bookmark_block <name>. Bookmarks the current block with the name given as the argument.|
|tas_bookmark_frame|Usage: tas_bookmark_frame <name>. Bookmarks the current frame with the name given as the argument.|
|tas_bookmark_skip|Usage: tas_bookmark_skip <name>. Skips to bookmark with the given name.|
|tas_cancel|Cancel change in editing mode|
|tas_cmd_reset|Resets all toggles and console variables to their default values|
|tas_edit_delete|Delete current block|
|tas_edit_prune|Removes all blocks with no content|
|tas_edit_save|Usage: tas_edit_save [savename]. Saves the script to file , if used without arguments saves to the same filename where the script was loaded from|
|tas_edit_set_pitch|Enters pitch editing mode|
|tas_edit_set_view|Enters pitch/yaw editing mode|
|tas_edit_set_yaw|Enters yaw editing mode|
|tas_edit_shift|Usage: tas_edit_shift <frames>. Shift current frameblock by frames. Use with a negative argument to shift backwards|
|tas_edit_shift_stack|Shifts all frameblocks after current one|
|tas_edit_shrink|Removes all frameblocks after current one|
|tas_edit_strafe|Enters strafe edit mode|
|tas_edit_swim|Enters swim edit mode|
|tas_print_origin|Prints origin on next physics frame|
|tas_print_vel|Prints velocity on next physics frame|
|tas_reset_movement|Resets movement related stuff|
|tas_script_advance|Usage: tas_script_advance <frames>. Advances script by number of frames given as argument. Use negative values to go backwards.|
|tas_script_advance_block|Usage: tas_script_advance <frames>. Advances script by number of blocks given as argument. Use negative values to go backwards.|
|tas_script_init|Usage: tas_script_init <filename> <map> <difficulty>. Initializes a TAS script.|
|tas_script_load|Usage: tas_script_load <filename>. Loads a TAS script from file.|
|tas_script_play|Plays a script.|
|tas_script_skip|Usage: tas_script_skip <frame>. Skips to the frame number given as parameter. Use with negative values to skip to the end, e.g. -1 skips to last frame, -2 skips to second last and so on.|
|tas_script_skip_block|Usage: tas_script_skip_block <block>. Skips to this number of block. Works with negative numbers similarly to regular skip|
|tas_script_stop|Stop a script from playing. This is the "reset everything that the game is doing" command.|

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
|tas_edit_backups|How many backups to keep while saving the script to file.|
|tas_edit_snap_threshold|How much rounding to apply when setting the strafe yaw and pitch|
|tas_hud_angles|View angles element|
|tas_hud_block|Displays block number in HUD|
|tas_hud_frame|Displays frame number in HUD|
|tas_hud_pflags|Displays player flags in HUD|
|tas_hud_pos|Display position in HUD|
|tas_hud_pos_inc|The vertical spacing between TAS HUD elements|
|tas_hud_pos_x|X position of the HUD|
|tas_hud_pos_y|Y position of the HUD|
|tas_hud_state|Displays lots of TAS info about frameblocks in HUD|
|tas_hud_vel|Displays xyz-velocity in HUD|
|tas_hud_vel2d|Displays 2d speed in HUD|
|tas_hud_vel3d|Displays 3d speed in HUD|
|tas_hud_velang|Displays velocity angle in HUD|
|tas_hud_waterlevel|Displays waterlevel in HUD|
|tas_pause_onload|When set to 1, pauses the game on load|
|tas_predict|Display position prediction while paused in a TAS.|
|tas_predict_amount|Amount of time to predict|
|tas_predict_per_frame|How long the prediction algorithm should run per frame. High values will kill your fps.|
|tas_strafe|Set to 1 to activate automated strafing|
|tas_strafe_pitch|Pitch angle to swim to. Only relevant while swimming.|
|tas_strafe_type|1 = max accel, 2 = max angle, 3 = w strafing, 4 = swimming|
|tas_strafe_yaw|Yaw angle to strafe at|
|tas_timescale|Controls the timescale of the game|
|tas_view_pitch|Player pitch.|
|tas_view_yaw|When not set to 999, sets the yaw the player should look at. When set to 999 the player will look towards the strafe yaw.|