; TorqueDemo.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The instalelr simply 
; prompts the user asking them where to install, and drops of notepad.exe
; there. If your Windows directory is not C:\windows, change it below.
;

; The name of the installer
Name "Torque Demo"
BrandingText " "

; The file to write
OutFile "TorqueDemo-Installer.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Torque Demo"
AutoCloseWindow true
Icon ".\main.ico"
LicenseText "End User License Agreement (EULA) for Torque Game Engine Demo"
LicenseData ".\licenseDemo.txt"

; The text to prompt the user to enter a directory
DirText "This will install the Torque Game Engine Demo on your computer. Choose a directory"

; The stuff to install
Section "Torque Game Engine Demo Files (required)"
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File ".\getdxver.exe"
  File /r "..\StagingArea\*.*"

  WriteUninstaller $INSTDIR\uninst-td.exe
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "The Torque Game Engine Demo has been installed.  Would you like to add shortcuts in the start menu?" \
             IDNO NoStartMenu
    SetOutPath "$SMPROGRAMS\Torque Game Engine"
    WriteINIStr "$SMPROGRAMS\Torque Game Engine\Torque Game Engine Home Page.url" \
                "InternetShortcut" "URL" "http://www.garagegames.com/pg/product/view.php?id=1"
    CreateShortCut "$SMPROGRAMS\Torque Game Engine\Uninstall Torque Game Engine Demo.lnk" \
                   "$INSTDIR\uninst-td.exe"
    SetOutPath $INSTDIR
    CreateShortCut "$SMPROGRAMS\Torque Game Engine\Torque Game Engine Demo.lnk" \
                   "$INSTDIR\demo.exe"
  NoStartMenu:
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Would you like to add a desktop icon for the Torque Game Engine Demo?" IDNO NoDesktopIcon
    SetOutPath $INSTDIR
    CreateShortCut "$DESKTOP\Torque Game Engine Demo.lnk" "$INSTDIR\demo.exe"
  NoDesktopIcon:
  ExecWait "$INSTDIR\getdxver.exe" $R1
  IntCmp 2048 $R1 NoDXInstall NoDXInstall
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "The Torque Game Engine Demo requires a newer version of DirectX than you have installed on your computer.  Would you like to go to the Microsoft DirectX Downloads page now?" IDNO NoDXInstall
  ExecShell "open" "http://www.microsoft.com/windows/directx/downloads/default.asp"
  NoDXInstall:
  WriteRegStr HKLM SOFTWARE\TorqueGameEngineDemo "" $INSTDIR
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TorqueGameEngineDemo" \
                   "DisplayName" "TorqueGameEngineDemo (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TorqueGameEngineDemo" \
                   "UninstallString" '"$INSTDIR\uninst-td.exe"'
  MessageBox MB_YESNO|MB_ICONQUESTION \
              "The Torque Game Engine Demo installation has completed.  Would you like to view the README file now?" \
              IDNO NoReadme
        ExecShell open '$INSTDIR\ReadMe.html'
        NoReadme:
SectionEnd ; end the section

Section Uninstall
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Are you sure you wish to uninstall the Torque Game Engine Demo?" \
             IDNO Removed

  Delete "$SMPROGRAMS\Torque Game Engine\*.*"
  Delete "$DESKTOP\Torque Game Engine Demo.lnk"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TorqueGameEngineDemo"
  DeleteRegKey HKLM SOFTWARE\TorqueGameEngineDemo
  RMDir "$SMPROGRAMS\Torque Game Engine"
  RMDir /r $INSTDIR
  IfFileExists $INSTDIR 0 Removed 
      MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
Removed:
SectionEnd
  


; eof
