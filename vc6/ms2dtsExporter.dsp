# Microsoft Developer Studio Project File - Name="ms2dtsExporter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ms2dtsExporter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ms2dtsExporter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ms2dtsExporter.mak" CFG="ms2dtsExporter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ms2dtsExporter - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ms2dtsExporter - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ms2dtsExporter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../tools/out.VC6.RELEASE/ms2dtsExporter"
# PROP Intermediate_Dir "../tools/out.VC6.RELEASE/ms2dtsExporter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MS2DTSEXPORTER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../lib/dtsSDK" /I "../tools/ms2dtsExporter/msLib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MS2DTSEXPORTER_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 msModelLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib dtsSDK.lib /nologo /dll /incremental:yes /debug /machine:I386 /nodefaultlib:"LIBC" /out:"../tools/ms2dtsExporter.dll" /libpath:"../tools/ms2dtsExporter/msLib" /libpath:"../lib/out.VC6.RELEASE"

!ELSEIF  "$(CFG)" == "ms2dtsExporter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../tools/out.VC6.DEBUG/ms2dtsExporter"
# PROP Intermediate_Dir "../tools/out.VC6.DEBUG/ms2dtsExporter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MS2DTSEXPORTER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../lib/dtsSDK" /I "../tools/ms2dtsExporter/msLib" /D "WIN32" /D "DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MS2DTSEXPORTER_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msModelLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib dtsSDK_DEBUG.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"../tools/ms2dtsExporter_DEBUG.dll" /pdbtype:sept /libpath:"../tools/ms2dtsExporter/msLib" /libpath:"../lib/out.VC6.DEBUG"

!ENDIF 

# Begin Target

# Name "ms2dtsExporter - Win32 Release"
# Name "ms2dtsExporter - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "msLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\msLIB\msLib.h
# End Source File
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\msLIB\msPlugIn.h
# End Source File
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\msLIB\msModelLib.lib
# End Source File
# End Group
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\DTSMilkshapeMesh.cpp
# End Source File
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\DTSMilkshapeMesh.h
# End Source File
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\DTSMilkshapeShape.cpp
# End Source File
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\DTSMilkshapeShape.h
# End Source File
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\DTSPlugin.cpp
# End Source File
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\ms2dtsExporter.def
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\tools\ms2dtsExporter\resource.rc
# End Source File
# End Group
# End Target
# End Project
