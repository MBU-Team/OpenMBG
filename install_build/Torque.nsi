; TorqueDemo.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The instalelr simply 
; prompts the user asking them where to install, and drops of notepad.exe
; there. If your Windows directory is not C:\windows, change it below.
;

; The name of the installer
Name "Torque Game Engine SDK"
BrandingText " "

; The file to write
OutFile "TorqueGameEngineSDK-Installer.exe"

; The default installation directory
InstallDir "c:\TGE"
AutoCloseWindow true
Icon ".\main.ico"
LicenseText "Source Code License Agreement for Torque Game Engine"
LicenseData ".\torqueLicense.txt"

; The text to prompt the user to enter a directory
DirText "This will install the Torque Game Engine SDK on your computer. Choose a directory"

; The stuff to install
Section "Torque Game Engine SDK Files (required)"
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File ".\getdxver.exe"
  File /r "..\SDKStagingArea\*.*"

  WriteUninstaller $INSTDIR\uninst-td.exe
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "The Torque Game Engine SDK has been installed.  Would you like to add shortcuts in the start menu?" \
             IDNO NoStartMenu
    SetOutPath "$SMPROGRAMS\Torque Game Engine SDK"
    WriteINIStr "$SMPROGRAMS\Torque Game Engine SDK\Torque Game Engine Home Page.url" \
                "InternetShortcut" "URL" "http://www.garagegames.com/pg/product/view.php?id=1"
    SetOutPath "$INSTDIR\example"
    CreateShortCut "$SMPROGRAMS\Torque Game Engine SDK\Uninstall Torque Game Engine SDK.lnk" \
                   "$INSTDIR\uninst-td.exe"
    SetOutPath $INSTDIR
    CreateShortCut "$SMPROGRAMS\Torque Game Engine SDK\Torque Game Engine Demo.lnk" \
                   "$INSTDIR\example\torqueDemo.exe"
  NoStartMenu:
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Would you like to add a desktop icon for the Torque Game Engine SDK Demo?" IDNO NoDesktopIcon
    SetOutPath "$INSTDIR\example"
    CreateShortCut "$DESKTOP\Torque Game Engine SDK Demo.lnk" "$INSTDIR\example\torqueDemo.exe"
    SetOutPath $INSTDIR
  NoDesktopIcon:
  ExecWait "$INSTDIR\getdxver.exe" $R1
  IntCmp 2048 $R1 NoDXInstall NoDXInstall
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "The Torque Game Engine SDK requires a newer version of DirectX than you have installed on your computer.  Would you like to go to the Microsoft DirectX Downloads page now?" IDNO NoDXInstall
  ExecShell "open" "http://www.microsoft.com/windows/directx/downloads/default.asp"
  NoDXInstall:
  WriteRegStr HKLM SOFTWARE\TorqueGameEngineSDK "" $INSTDIR
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TorqueGameEngineSDK" \
                   "DisplayName" "TorqueGameEngineSDK (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TorqueGameEngineSDK" \
                   "UninstallString" '"$INSTDIR\uninst-td.exe"'
  WriteRegStr HKCU "Software\Microsoft\Devstudio\6.0\Build System\Components\Platforms\Win32 (x86)\Tools\32-bit C/C++ Compiler for 80x86" \
                   "Input_Spec" "*.c;*.cpp;*.cxx;*.cc"
  WriteRegStr HKCU "Software\Microsoft\Devstudio\6.0\Build System\Components\Platforms\Win32 (x86)\Tools\32-bit C/C++ Compiler für 80x86" \
                   "Input_Spec" "*.c;*.cpp;*.cxx;*.cc"
  WriteRegStr HKCU "Software\Microsoft\Devstudio\6.0\Text Editor\Tabs/Language Settings\C/C++" \
                   "FileExtensions"="cpp;cxx;c;h;hxx;hpp;inl;tlh;tli;rc;rc2;cc"

  MessageBox MB_YESNO|MB_ICONQUESTION \
              "The Torque Game Engine SDK installation has completed.  Would you like to view the README file now?" \
              IDNO NoReadme
        ExecShell open '$INSTDIR\ReadMe.html'
        NoReadme:
SectionEnd ; end the section

Section Uninstall
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Are you sure you wish to uninstall the Torque Game Engine SDK?" \
             IDNO Removed

  Delete "$SMPROGRAMS\Torque Game Engine SDK\*.*"
  Delete "$DESKTOP\Torque Game Engine SDK Demo.lnk"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TorqueGameEngineSDK"
  DeleteRegKey HKLM SOFTWARE\TorqueGameEngineSDK
  RMDir "$SMPROGRAMS\Torque Game Engine SDK"
  RMDir /r $INSTDIR
  IfFileExists $INSTDIR 0 Removed 
      MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
Removed:
SectionEnd
  


; eof
