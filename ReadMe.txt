NEAQUAKE, based on JoeQuake at http://joequake.runecentral.com/news.html

All JoeQuake demos, maps and saves are compatible with this mod. For simplicitys sake, the mod directory remains "joequake" so an installation is only the executable.

Changes in this binary:

* The application now starts and exits instantly without splash screens and some monitor query code.

* Mouse handling has been redone, there is now only raw mouse input and no mouse acceleration. Mouse4 and Mouse5 are now also bindable

* Window creation is new and properly handles fullscreen and windowed, with borderless if wanted.

* The update rate of show_speed can now be controlled with show_speed_interval command.

* Coop demos no longer get corrupt when any client starts recording and plays back.

Startup parameters:

-window
Start in windowed mode.

-borderless
Have no borders around the window if window mode.

-width
Width for window or display width in noscale.

-height
Height for window or display height in noscale.

-noscale
Do not scale to the highest 4:3 resolution available, use -width and -height to set the desired resolution. Only changes the display size of the game.
