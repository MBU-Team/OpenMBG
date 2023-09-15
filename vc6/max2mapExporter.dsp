# Microsoft Developer Studio Project File - Name="max2mapExporter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=max2mapExporter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "max2mapExporter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "max2mapExporter.mak" CFG="max2mapExporter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "max2mapExporter - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "max2mapExporter - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "max2mapExporter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../tools/out.VC6.RELEASE"
# PROP Intermediate_Dir "../tools/out.VC6.RELEASE/max2mapExporter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MAX2MAPEXPORTER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /GR /GX /O2 /I "../engine" /I "../lib/maxsdk31" /I "../tools" /YX /FD /c /Tp
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 COMCTL32.LIB COMDLG32.LIB USER32.LIB ADVAPI32.LIB GDI32.LIB WINMM.LIB WSOCK32.LIB Mesh.lib Geom.lib MaxUtil.lib Gfx.lib Core.lib Bmm.lib ljpeg.lib lpng.lib lungif.lib zlib.lib engine.lib ../lib/vorbis/win32/vorbis_static_mt.lib /nologo /dll /machine:I386 /nodefaultlib:"LIBC" /out:"../tools/max2mapExporter.dle" /libpath:"../lib/maxsdk31" /libpath:"../lib/out.VC6.RELEASE"

!ELSEIF  "$(CFG)" == "max2mapExporter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "max2mapExporter___Win32_Debug"
# PROP BASE Intermediate_Dir "max2mapExporter___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../tools/out.VC6.DEBUG"
# PROP Intermediate_Dir "../tools/out.VC6.DEBUG/max2mapExporter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MAX2MAPEXPORTER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MT /Gm /GR /GX /ZI /Od /I "../engine" /I "../lib/maxsdk31" /I "../tools" /D "TORQUE_DEBUG" /YX /FD /GZ /c /Tp
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 COMCTL32.LIB COMDLG32.LIB USER32.LIB ADVAPI32.LIB GDI32.LIB WINMM.LIB WSOCK32.LIB Mesh.lib Geom.lib MaxUtil.lib Gfx.lib Core.lib Bmm.lib ljpeg_DEBUG.lib lpng_DEBUG.lib lungif_DEBUG.lib zlib_DEBUG.lib engine_DEBUG.lib ../lib/vorbis/win32/vorbis_static_mt.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"../tools/max2mapExporter_DEBUG.dle" /pdbtype:sept /libpath:"../lib/maxsdk31" /libpath:"../lib/out.VC6.DEBUG" /DEF:../tools/max2mapExporter/exporter.def
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "max2mapExporter - Win32 Release"
# Name "max2mapExporter - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\tools\max2mapExporter\main.cc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\tools\max2mapExporter\exporter.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\tools\max2mapExporter\exporter.def
# End Source File
# End Group
# End Target
# End Project
