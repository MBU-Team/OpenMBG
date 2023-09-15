# Microsoft Developer Studio Project File - Name="lpng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lpng - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lpng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lpng.mak" CFG="lpng - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lpng - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lpng - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lpng - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../lib/out.VC6.RELEASE"
# PROP Intermediate_Dir "../lib/out.VC6.RELEASE/lpng"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /O2 /I "..\lib\lpng" /I "..\lib\zlib" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /NODEFAULTLIB:LIBC

!ELSEIF  "$(CFG)" == "lpng - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../lib/out.VC6.DEBUG"
# PROP Intermediate_Dir "../lib/out.VC6.DEBUG/lpng"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /ZI /Od /I "..\lib\lpng" /I "..\lib\zlib" /D "TORQUE_DEBUG" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../lib/out.VC6.DEBUG/lpng_DEBUG.lib" /NODEFAULTLIB:LIBCD

!ENDIF 

# Begin Target

# Name "lpng - Win32 Release"
# Name "lpng - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\lpng\png.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngerror.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pnggccrd.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngget.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngmem.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngpread.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngread.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngrio.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngrtran.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngrutil.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngset.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngtrans.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngvcrd.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngwio.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngwrite.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngwtran.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngwutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\lib\lpng\png.h
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngasmrd.h
# End Source File
# Begin Source File

SOURCE=..\lib\lpng\pngconf.h
# End Source File
# End Group
# End Target
# End Project
