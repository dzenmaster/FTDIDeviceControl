!include LogicLib.nsh
!include "x64.nsh"
!include "MUI.nsh"

icon ..\FTDIDeviceControl\icon1.ico

Name "FTDIDeviceControl"

# define the name of the installer
Outfile "FTDIDeviceControl_32.exe"
 
# define the directory to install to, the desktop in this case as specified  
# by the predefined $DESKTOP variable

InstallDir "$PROGRAMFILES\FTDIDeviceControl"
 
# default section
Section
 
# define the output path for this file
SetOutPath $INSTDIR

CreateDirectory "$PROGRAMFILES\FTDIDeviceControl"
 
;File /r win32
File /r ..\win32\Release\*.*

;create desktop shortcut
CreateShortCut "$DESKTOP\FTDIDeviceControl 32bit.lnk" "$INSTDIR\FTDIDeviceControl.exe" ""
 
SectionEnd