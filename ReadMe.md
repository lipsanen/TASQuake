TASQuake is based on NEAQUAKE, which is based on joequake.

TODO:
* Basic ability to record and playback actions
* Add savestates if regular saves arent consistent
    * Figure out how to start the game on a certain map
    * Figure out what is contained within the total gamestate
* Add an interface to add trackers into the HUD for entity stuff or drawing other TAS related information
* Add frame advance to TAS playback
    * Some HUD stuff to display what keys are being pressed down
    * Ability to set strafe angle and viewangle keyframes
    * Can also set viewangle to match strafe angle
    * Automatic smooth transitions between viewangles keyframes, instant ones for strafe angle
    * Lots of binds for toggling stuff