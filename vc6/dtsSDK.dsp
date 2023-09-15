# Microsoft Developer Studio Project File - Name="dtsSDK" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dtsSDK - Win32 Hybrid
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dtsSDK.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dtsSDK.mak" CFG="dtsSDK - Win32 Hybrid"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dtsSDK - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dtsSDK - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "dtsSDK - Win32 Hybrid" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dtsSDK - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../lib/out.VC6.RELEASE"
# PROP Intermediate_Dir "../lib/out.VC6.RELEASE/dtsSDK"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "../lib/dtsSDK" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /NODEFAULTLIB:LIBC

!ELSEIF  "$(CFG)" == "dtsSDK - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dtsSDK___Win32_Debug"
# PROP BASE Intermediate_Dir "dtsSDK___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../lib/out.VC6.DEBUG"
# PROP Intermediate_Dir "../lib/out.VC6.DEBUG/dtsSDK"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../lib/dtsSDK" /D "TORQUE_DEBUG" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../lib/out.VC6.DEBUG/dtsSDK_DEBUG.lib" /NODEFAULTLIB:LIBCD

!ELSEIF  "$(CFG)" == "dtsSDK - Win32 Hybrid"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dtsSDK___Win32_Hybrid"
# PROP BASE Intermediate_Dir "dtsSDK___Win32_Hybrid"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../lib/out.VC6.HYBRID"
# PROP Intermediate_Dir "../lib/out.VC6.HYBRID/dtsSDK"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../engine" /I "../lib/dtsSDK" /D "TORQUE_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GR /GX /ZI /Od /I "../lib/dtsSDK" /D "TORQUE_DEBUG" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"../lib/out.VC6.DEBUG/dtsSDK_DEBUG.lib" /NODEFAULTLIB:LIBCD
# ADD LIB32 /nologo /NODEFAULTLIB:LIBCD

!ENDIF 

# Begin Target

# Name "dtsSDK - Win32 Release"
# Name "dtsSDK - Win32 Debug"
# Name "dtsSDK - Win32 Hybrid"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSBrushMesh.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSMatrix.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSMesh.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSNormalTable.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSQuaternion.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSShape.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSBrushMesh.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSEndian.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSInputStream.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSInterpolation.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSMatrix.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSMesh.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSOutputStream.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSPlugin.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSPoint.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSQuaternion.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSShape.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSTypes.h
# End Source File
# Begin Source File

SOURCE=..\lib\dtsSDK\DTSVector.h
# End Source File
# End Group
# End Target
# End Project
