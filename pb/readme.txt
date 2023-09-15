README for OSX build

Note that only the PB 2.1 project file is currently being kept up to date.
Previous projects aren't guaranteed to work properly, and have issues not
listed herein.


IN ORDER FOR RUNNING/DEBUGGING TO WORK PROPERLY, YOU MUST DO THE FOLLOWING STEPS:

- launch the PB project.
- select/click the topmost group (should be named the same as the project...)
- from menu choose Project=>Show Info
- Select "Place build products for this project in a separate location"
- enter "../example" (without the quotes, and replace "example" with your runtime
folder if using a different naming...)
- close the window.

Your project should now directly build the target executable directly into the
runtime folder, which also means it is properly located for running or debugging
from within PB.


Some known issues:  (lots have been fixed and removed! yay!)

- No external console window.  in debug in PB, printfs will shunt to the gdb pane.
At some point, will likely create a Carbon-based simple console library to be used
by both Carbon and Mach-O builds.
  
- ADDENDUM: printfs may also output to the Console, and will output to the Terminal if
you actually get the mach-o app to launch directly from the commandline... ;)

- windowed performance is slower than OS9 in quick tests, due to extra copy.

- haven't yet looked hard at fullscreen performance.  should be better than windowed,
and isn't yet.

- OSX can now handle cmdline args like the PC, but the workaround of using maccmdline.txt
still exists and is preferable (since, again, you'd have to be launching from the cmdline).
Note that this means that you can add args in the project file for debug vs release, etc.
 
- there's an old mac issue of some hiccupping due to processing landscape -- or at least,
that's what I infer is the prob.  I've heard reports recently, but haven't seen it in a
long while.  generally will happen if large new sets of textures need to be generated.

- mac performance is still way under PC, especially on X for a whole host of reasons.  could
probably use to revamp the system to use Apple's new Vertex Objects implementation for best
GL performance on X.

- general issue of GUI garbage when moving mouse around (seen it on Windows on Rage128,
so it could be an ATI issue generally)
