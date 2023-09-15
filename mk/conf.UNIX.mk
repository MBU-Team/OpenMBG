O=.obj
# there is no "exe" for x86 UNIX, but the makefiles need some extension so use this for now.
EXT.EXE=$(BUILD_SUFFIX).bin
EXT.LIB=$(BUILD_SUFFIX).a
PRE.LIBRARY.LIB=../lib/$(DIR.OBJ)/
PRE.ENGINE.LIB=../engine/$(DIR.OBJ)/

# we won't be needing any DLLs, but just leave the def'n in place.
EXT.DLL=$(BUILD_SUFFIX).dll
EXT.DLE=$(BUILD_SUFFIX).dle
EXT.DEP=.d

MKDIR=mkdir $(@:/=)
RM=rm -f
RMDIR=rm -rf
CP=cp
LN=cp

# these compiler names can be overridden on conf.GCC2.LINUX.mk and 
# conf.GCC3.LINUX.mk.  These two files allow you to build the same source
# tree with two different compilers without introducing conflicts.
COMPILER.c      =gcc
COMPILER.cc     =g++
COMPILER.asm    =nasm
LINK            =ar
LINK.cc         =ld

# Noteworthy compiler options:

# -DUSE_FILE_REDIRECT: enable fileio redirection to home directory.  The 
#   exact location is ~/.PREF_DIR_ROOT/PREF_DIR_GAME_NAME.  These are set in
#   platformX86UNIX.h.  You can disable redirection at run time with the
#   -nohomedir command line paramater.  Files will then be stored in the game
#   directory, like the windows version.
# -MD: generate .d dependency makefiles.  These are necessary if you want
#   to cleanly do partial builds.  
# -w: disable all warnings
# -fno-exceptions is safe to use (if your code doesn't use exceptions), 
#    but it is disabled by default
# -fno-check-new is not tested
CFLAGS.GENERAL    = -DUSE_FILE_REDIRECT -I/usr/X11R6/include/ -MD -march=i586 \
		    -mcpu=i686 -ffast-math -pipe 
		    #-w -fno-exceptions -fno-check-new 
CFLAGS.RELEASE    = -O2 -finline-functions -fomit-frame-pointer 
CFLAGS.DEBUG      = -g -DTORQUE_DEBUG # -DTORQUE_DISABLE_MEMORY_MANAGER
CFLAGS.DEBUGFAST  = -O -g -finline-functions 

ASMFLAGS          = -f elf -dLINUX

LFLAGS.GENERAL    = 
LFLAGS.RELEASE    =
LFLAGS.DEBUG      =

# the included vorbis libs are used by default.  You
# can use your own libs if you want, just comment out the following
# variable and uncomment the one after it.  Note, the executable will not
# automatically use the sdk ogg/vorbis libraries because they are not in the
# system path.  This means that if you don't have vorbis installed, you will
# get runtime errors from the executable unless you use the "runtorque.sh"
# script to start it.
LINK.LIBS.VORBIS  =  ../lib/vorbis/linux/libogg.so.0 ../lib/vorbis/linux/libvorbis.so.0 
# the following uses the system libraries 
#LINK.LIBS.VORBIS = -logg -lvorbis

# GLU must be statically linked, otherwise torque will crash.
# JMQNOTE: aside from gluProject/unProject, GLU doesn't work.  
# calling a GLU function that calls a GL function will cause a 
# crash.  let me know if you have a fix :)
LINK.LIBS.GENERAL = $(LINK.LIBS.VORBIS) -Wl,-static -Wl,-lGLU -Wl,-dy -L/usr/X11R6/lib -lSDL -lpthread -ldl # -lefence

LINK.LIBS.TOOLS   = $(LINK.LIBS.VORBIS) -Wl,-static -Wl,-lGLU -Wl,-dy -L/usr/X11R6/lib -lSDL -lpthread -ldl # -lefence
# -lefence is useful for finding memory corruption problems
LINK.LIBS.SERVER  = -lpthread
LINK.LIBS.RELEASE = 
LINK.LIBS.DEBUG   = 

PATH.H.SYS        =
