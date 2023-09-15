# Microsoft Developer Studio Project File - Name="ljpeg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ljpeg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ljpeg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ljpeg.mak" CFG="ljpeg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ljpeg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ljpeg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ljpeg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../lib/out.VC6.RELEASE"
# PROP Intermediate_Dir "../lib/out.VC6.RELEASE/ljpeg"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /O2 /I "ljpeg" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /NODEFAULTLIB:LIBC

!ELSEIF  "$(CFG)" == "ljpeg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ljpeg___Win32_Debug"
# PROP BASE Intermediate_Dir "ljpeg___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../lib/out.VC6.DEBUG"
# PROP Intermediate_Dir "../lib/out.VC6.DEBUG/ljpeg"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /ZI /Od /I "ljpeg" /D "TORQUE_DEBUG" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../lib/out.VC6.DEBUG/ljpeg_DEBUG.lib" /NODEFAULTLIB:LIBCD

!ENDIF 

# Begin Target

# Name "ljpeg - Win32 Release"
# Name "ljpeg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\ljpeg\jcapimin.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcapistd.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jccoefct.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jccolor.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jchuff.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcinit.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcmainct.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcmarker.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcmaster.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcomapi.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcparam.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcphuff.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcprepct.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jcsample.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jctrans.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdapimin.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdapistd.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdatadst.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdcolor.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdhuff.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdinput.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdmainct.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdmarker.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdmaster.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdmerge.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdphuff.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdpostct.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdsample.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdtrans.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jerror.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jfdctflt.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jfdctint.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jidctflt.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jidctfst.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jidctint.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jidctred.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jquant1.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jquant2.c
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jutils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\lib\ljpeg\jchuff.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jconfig.vc.win.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdct.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jdhuff.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jerror.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\lib\ljpeg\jversion.h
# End Source File
# End Group
# End Target
# End Project
