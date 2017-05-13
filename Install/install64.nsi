!include LogicLib.nsh
!include "x64.nsh"
!include "MUI.nsh"

icon ..\FTDIDeviceControl\icon1.ico

Name "FTDIDeviceControl"

# define the name of the installer
Outfile "FTDIDeviceControl_64.exe"
 
# define the directory to install to, the desktop in this case as specified  
# by the predefined $DESKTOP variable

InstallDir "$PROGRAMFILES64\FTDIDeviceControl"
 
# default section
Section
 
# define the output path for this file
SetOutPath $INSTDIR

CreateDirectory "$PROGRAMFILES64\FTDIDeviceControl"
 
;File /r win32
File /r ..\x64\Release\*.*

;create desktop shortcut
CreateShortCut "$DESKTOP\FTDIDeviceControl 64bit.lnk" "$INSTDIR\FTDIDeviceControl.exe" ""
 
SectionEnd