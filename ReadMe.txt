TASQuake, based on NEAQUAKE, which is based on joequake.

We'll see if this gets anywhere.

TODO:
Specify new rand/srand functions to avoid any issues with different compilers using a different implementation for generating random numbers.
 * Figure out places where RNG is used
 * Relating to this, figure out what can be done to remove inconsistencies in the Host_Frame function. Depending on your fps
 the function(and thus also the rand() function) may be run a different number of times, regardless of host_framerate setting. Options are 
 either to simulate the effect of a running at a constant 72 fps or by just moving the rand() function to a later spot in the function(this seems slightly more suspect, but probably easier to implement).
Basic ability to record and playback actions
Autostrafing
Add functionality to simulate joystick movement
Add savestates if regular saves arent consistent
 * Figure out how to start the game on a certain map
 * Figure out what is contained within the total gamestate
Add an interface to add trackers into the HUD for entity stuff or drawing other TAS related information
Add frame advance to TAS playback
 * Some HUD stuff to display what keys are being pressed down
 * Ability to set strafe angle and viewangle keyframes
 * Can also set viewangle to match strafe angle
 * Automatic smooth transitions between viewangles keyframes, instant ones for strafe angle
 * Lots of binds for toggling stuff
 * A separate bind system might be required for the TASing stuff if the commands have to be executed while the game is paused