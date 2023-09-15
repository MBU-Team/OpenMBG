Marble Blast Gold v. 1.4 for Windows Release Notes
Distributed by RealNetworks, Inc.
--------------------------------------------------

Thank you for purchasing Marble Blast Gold and supporting independent
game developers.

Contents
--------

Overview
Controls
About the Torque Game Engine
Problems and Questions

Overview
--------

Roll your marble through a rich cartoon landscape of moving 
platforms and dangerous hazards.  Along the way find power ups 
to increase your speed, jumping ability or flight power, and 
use them to collect the hidden gems and race to the finish for 
the fastest time.

Controls
--------

All controls can be changed in the Options screen.

Roll Marble Forward - W
Roll Marble Backward - S
Roll Marble Left - A
Roll Marble Right - D

Jump - Space Bar
Use PowerUp - Left Mouse Button

Change Camera direction - Move Mouse - or

Look Up - Up Arrow
Look Down - Down Arrow
Turn Left - Left Arrow
Turn Right - Right Arrow

Free-Look Camera - Right Mouse Button

You can turn free look on by default from the 
Options->Controls->Mouse screen.

About the Torque Game Engine
----------------------------

The Torque Game Engine (TGE) is a full featured AAA title 
engine with the latest in scripting, geometry, particle effects, 
animation and texturing, as well as award winning multi-player 
networking code.  For $100 per programmer, you get the source to 
the engine that powers Marble Blast, Orbz, Sierra's Tribes 2, 
and many other titles currently in development.

Problems and Questions
----------------------

Listed below are some common questions and troubleshooting issues,
as well as possible solutions or workarounds:

Problem: How do I use my Logitech WingMan RumblePad controller with Marble Blast?

Solution: Bring up the console by hitting ~ (tilde) and type: 
enableJoystick(); followed by <ENTER>.  You can hide the console by 
hitting ~ again.  To disable the gamepad support bring up the console 
and type disableJoystick();

Problem: I don't have a WingMan, but I do have gamepad XYZ... will that
work?

Solution: Quite possibly!  Your gamepad must have two analog sticks - and
the controls for jumping and using powerups are by default mapped to 
the trigger buttons.  If you're a scripting whiz, you can look in
the file marble/client/scripts/default.bind.cs
                           
Problem: The game brings up a dialog saying it can't find a 
compatible display device.

Solution: Make sure the drivers for you graphics card are up to
date.  Marble Blast requires an OpenGL or D3D compatible graphics accelerator.

Problem: I turned on stencil shadows, and my frame rate is
terrible!

Solution: Make sure your desktop bit depth is set to 32 bits.
If your frame rate problems persist, turn off stencil shadows.

Problem: I get clicks and pops in the sounds when the marble is 
rolling around.

Solution: Some on-board sound processors have problems with 
Marble Blast.  Try updating your audio drivers or get a new 
sound card.

Problem: I have a really big/wide monitor and Marble Blast doesn't
let me access all the different resolution settings of my monitor!!!

Solution: You can manually set your fullscreen resolution from the
in-game console.  Press the tilde key (~) to access the console, and
then type: 
listResolutions();
This should display a list of all the resolutions your monitor and
video card support.  You can then set the resolution manually with
the setResolution() command like:
setResolution(1600, 1200, 32);
Pressing the ~ again will cause the console to go away.

Problem: Your easy listening music is cramping my marble style...
how do I play my own rockin' tunes in this game?

Solution: Marble Blast looks for files with the .ogg extension
in subdirectories of the "My Games/Marble Blast Gold/marble" 
directory.  To play your own tunes, just drop any ogg/vorbis encoded file (.ogg)
into the "My Games/Marble Blast Gold/marble" directory.  
If you want to get rid of the music that ships with the game, remove the 
.ogg files from the "My Games/Marble Blast Gold/marble/data/sound"
directory.

Problem: The Gold Time on level XYZ is way too low!  It is clearly
impossible to get such a low time!

Solution: Try again.  All of the gold times are possible to achieve
without cheating... but some of them are VERY hard to get.

Problem: I have some other problem not listed above.

Solution: Check the RealOne Arcade support site at: 
http://www.realarcade.com

