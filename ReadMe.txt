NEAQUAKE, based on JoeQuake at http://joequake.runecentral.com/news.html

All JoeQuake demos, maps and saves are compatible with this mod. For simplicitys sake, the mod directory remains "joequake" so an installation is only the executable.

Changes in this binary:

* The application now starts and exits instantly without splash screens and some monitor query code.

* Mouse handling has been redone, there is now only raw mouse input and no mouse acceleration. Mouse4 and Mouse5 are now also bindable

* Window creation is new and properly handles fullscreen. Fullscreen mode will always open at the native primary monitor resolution and add vertical borders as necessary. For windowed mode, borderless mode has also been added. To start in windowed mode, use -window. If you wish to use borderless window mode you need -window and -borderless. In window mode you can specify a resolution you wish to open at with -width and -height. If no window flag is found, the game will open in fullscreen at your native resolution.

* The update rate of show_speed can now be controlled with show_speed_interval command.

* Coop demos no longer get corrupt when any client starts recording and plays back.
