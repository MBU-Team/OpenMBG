# Microsoft Developer Studio Project File - Name="zlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=zlib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zlib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zlib.mak" CFG="zlib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zlib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "zlib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "zlib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../lib/out.VC6.RELEASE"
# PROP Intermediate_Dir "../lib/out.VC6.RELEASE/zlib"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /O2 /I "zlib" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /NODEFAULTLIB:LIBC

!ELSEIF  "$(CFG)" == "zlib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../lib/out.VC6.DEBUG"
# PROP Intermediate_Dir "../lib/out.VC6.DEBUG/zlib"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /ZI /Od /I "zlib" /D "TORQUE_DEBUG" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../lib/out.VC6.DEBUG/zlib_DEBUG.lib" /NODEFAULTLIB:LIBCD

!ENDIF 

# Begin Target

# Name "zlib - Win32 Release"
# Name "zlib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\infblock.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\infutil.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\zutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\lib\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\lib\zlib\zutil.h
# End Source File
# End Group
# End Target
# End Project
