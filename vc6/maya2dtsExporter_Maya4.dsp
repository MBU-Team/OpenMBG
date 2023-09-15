# Microsoft Developer Studio Project File - Name="maya2dtsExporter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=maya2dtsExporter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "maya2dtsExporter_Maya4.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "maya2dtsExporter_Maya4.mak" CFG="maya2dtsExporter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "maya2dtsExporter - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "maya2dtsExporter - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "maya2dtsExporter - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../tools/out.VC6.RELEASE"
# PROP Intermediate_Dir "../tools/out.VC6.RELEASE/maya2dtsExporter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\lib\mayasdk4\include" /I "..\lib\dtsSdk" /I "..\lib\dtsSdkPlus" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_PLUGIN" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 dtsSdk.lib dtsSdkPlus.lib Foundation.lib OpenMaya.lib OpenMayaAnim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\tools\maya2dtsExporter.mll" /libpath:"..\lib\mayasdk4\lib" /libpath:"..\lib\out.VC6.RELEASE" /export:initializePlugin /export:uninitializePlugin

!ELSEIF  "$(CFG)" == "maya2dtsExporter - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../tools/out.VC6.DEBUG"
# PROP Intermediate_Dir "../tools/out.VC6.DEBUG/maya2dtsExporter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I "..\lib\mayasdk4\include" /I "..\lib\dtsSdk" /I "..\lib\dtsSdkPlus" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_PLUGIN" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dtsSdk_DEBUG.lib dtsSdkPlus_DEBUG.lib Foundation.lib OpenMaya.lib OpenMayaAnim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\tools\maya2dtsExporter.mll" /pdbtype:sept /libpath:"..\lib\mayasdk4\lib" /libpath:"..\lib\out.VC6.DEBUG" /export:initializePlugin /export:uninitializePlugin

!ENDIF 

# Begin Target

# Name "maya2dtsExporter - Win32 Release"
# Name "maya2dtsExporter - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\tools\maya2dtsExporter\exportDTSCmd.cpp
# End Source File
# Begin Source File

SOURCE=..\tools\maya2dtsExporter\mayaAppMesh.cpp
# End Source File
# Begin Source File

SOURCE=..\tools\maya2dtsExporter\mayaAppNode.cpp
# End Source File
# Begin Source File

SOURCE=..\tools\maya2dtsExporter\mayaSceneEnum.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\tools\maya2dtsExporter\mayaAppMesh.h
# End Source File
# Begin Source File

SOURCE=..\tools\maya2dtsExporter\mayaAppNode.h
# End Source File
# Begin Source File

SOURCE=..\tools\maya2dtsExporter\mayaSceneEnum.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
