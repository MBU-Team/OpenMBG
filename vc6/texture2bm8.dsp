# Microsoft Developer Studio Project File - Name="texture2bm8" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=texture2bm8 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "texture2bm8.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "texture2bm8.mak" CFG="texture2bm8 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "texture2bm8 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "texture2bm8 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "texture2bm8 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../tools/out.VC6.RELEASE"
# PROP Intermediate_Dir "../tools/out.VC6.RELEASE/texture2bm8"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /GR /GX /O2 /I "../engine" /I "../lib/zlib" /I "../lib/lungif" /I "../lib/lpng" /I "../lib/ljpeg" /I "../tools" /YX /FD /c /Tp
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 COMCTL32.LIB COMDLG32.LIB USER32.LIB ADVAPI32.LIB GDI32.LIB WINMM.LIB WSOCK32.LIB ljpeg.lib lpng.lib lungif.lib zlib.lib engine.lib /nologo /subsystem:console /machine:I386 /out:"../tools/texture2bm8.exe" /libpath:"../lib/out.VC6.RELEASE"

!ELSEIF  "$(CFG)" == "texture2bm8 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../tools/out.VC6.DEBUG"
# PROP Intermediate_Dir "../tools/out.VC6.DEBUG/texture2bm8"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /Gm /GR /GX /ZI /Od /I "../engine" /I "../lib/zlib" /I "../lib/lungif" /I "../lib/lpng" /I "../lib/ljpeg" /I "../tools" /D "TORQUE_DEBUG" /YX /FD /GZ /c /Tp
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 COMCTL32.LIB COMDLG32.LIB USER32.LIB ADVAPI32.LIB GDI32.LIB WINMM.LIB WSOCK32.LIB ljpeg_DEBUG.lib lpng_DEBUG.lib lungif_DEBUG.lib zlib_DEBUG.lib engine_DEBUG.lib /nologo /subsystem:console /debug /machine:I386 /out:"../tools/texture2bm8_DEBUG.exe" /pdbtype:sept /libpath:"../lib/out.VC6.DEBUG"

!ENDIF 

# Begin Target

# Name "texture2bm8 - Win32 Release"
# Name "texture2bm8 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\tools\texture2bm8\main.cc
# End Source File
# Begin Source File

SOURCE=..\tools\texture2bm8\svector.cc
# End Source File
# Begin Source File

SOURCE=..\tools\texture2bm8\vector.asm

!IF  "$(CFG)" == "texture2bm8 - Win32 Release"

# Begin Custom Build
IntDir=.\../tools/out.VC6.RELEASE/texture2bm8
WkspDir=.
InputPath=..\tools\texture2bm8\vector.asm
InputName=vector

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(WkspDir)\..\bin\nasm\nasmw.exe" -f win32 $(InputPath) -o "$(IntDir)/$(InputName).obj"

# End Custom Build

!ELSEIF  "$(CFG)" == "texture2bm8 - Win32 Debug"

# Begin Custom Build
IntDir=.\../tools/out.VC6.DEBUG/texture2bm8
WkspDir=.
InputPath=..\tools\texture2bm8\vector.asm
InputName=vector

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(WkspDir)\..\bin\nasm\nasmw.exe" -f win32 $(InputPath) -o "$(IntDir)/$(InputName).obj"

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\tools\texture2bm8\svector.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
