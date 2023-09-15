# Microsoft Developer Studio Project File - Name="map2dif" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=map2dif - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "map2dif.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "map2dif.mak" CFG="map2dif - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "map2dif - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "map2dif - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "map2dif - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../tools/out.VC6.RELEASE"
# PROP Intermediate_Dir "../tools/out.VC6.RELEASE/map2dif"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /GR /GX /O2 /I "../engine" /I "../lib/vorbis/include" /I "../lib/zlib" /I "../lib/lungif" /I "../lib/lpng" /I "../lib/ljpeg" /I "../lib/openal/win32" /I "../tools" /YX /FD /c /Tp
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 COMCTL32.LIB COMDLG32.LIB USER32.LIB ADVAPI32.LIB GDI32.LIB WINMM.LIB WSOCK32.LIB ljpeg.lib lpng.lib lungif.lib zlib.lib engine.lib ../lib/vorbis/win32/ogg_static.lib ../lib/vorbis/win32/vorbis_static_mt.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libc" /out:"../tools/map2dif.exe" /libpath:"../lib/out.VC6.RELEASE"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "map2dif - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../tools/out.VC6.DEBUG"
# PROP Intermediate_Dir "../tools/out.VC6.DEBUG/map2dif"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MT /Gm /GR /GX /ZI /Od /I "../engine" /I "../lib/vorbis/include" /I "../lib/zlib" /I "../lib/lungif" /I "../lib/lpng" /I "../lib/ljpeg" /I "../lib/openal/win32" /I "../tools" /D "TORQUE_DEBUG" /YX /FD /GZ /c /Tp
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 COMCTL32.LIB COMDLG32.LIB USER32.LIB ADVAPI32.LIB GDI32.LIB WINMM.LIB WSOCK32.LIB ljpeg_DEBUG.lib lpng_DEBUG.lib lungif_DEBUG.lib zlib_DEBUG.lib engine_DEBUG.lib ../lib/vorbis/win32/ogg_static.lib ../lib/vorbis/win32/vorbis_static_mt.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcd" /out:"../tools/map2dif_DEBUG.exe" /pdbtype:sept /libpath:"../lib/out.VC6.DEBUG"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "map2dif - Win32 Release"
# Name "map2dif - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;cc"
# Begin Source File

SOURCE=..\tools\map2dif\bspNode.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\createLightmaps.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\csgBrush.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\editFloorPlanRes.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\editGeometry.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\editInteriorRes.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\entityTypes.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\exportGeometry.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\lmapPacker.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\main.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\morianUtil.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\navGraph.cc
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\tokenizer.cc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\tools\map2dif\bspNode.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\createLightmaps.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\csgBrush.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\editFloorPlanRes.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\editGeometry.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\editInteriorRes.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\entityTypes.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\lmapPacker.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\morianBasics.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\morianGame.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\morianUtil.h
# End Source File
# Begin Source File

SOURCE=..\tools\map2dif\tokenizer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
