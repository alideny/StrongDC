; Install script for StrongDC++
; This script is based on the DC++ install script, (c) 2001-2010 Jacek Sieka
; You need the ANSI version of the MoreInfo plugin installed in NSIS to be able to compile this script
; Its available from http://nsis.sourceforge.net/MoreInfo_plug-in

; Uncomment the above line if you want to build installer for the 64-bit version
; !define X64

!include "Sections.nsh"


SetCompressor "lzma"

; The name of the installer
Name "StrongDC++"

ShowInstDetails show
ShowUninstDetails show

Page license
Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

; The file to write
OutFile "Sdcxxx.exe"

; The default installation directory
!ifdef X64
  InstallDir $PROGRAMFILES64\StrongDC++
!else
  InstallDir $PROGRAMFILES\StrongDC++
!endif

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\StrongDC++ "Install_Dir"

LicenseText "StrongDC++ is licensed under GPL, you can see the full license text below."
LicenseData "License.txt"
LicenseForceSelection checkbox

; The text to prompt the user to enter a directory
ComponentText "Welcome to the StrongDC++ installer."
; The text to prompt the user to enter a directory
DirText "Choose a directory to install StrongDC++ into. If you upgrade select the same directory where your old version resides and your existing settings will be imported."

; The stuff to install
Section "StrongDC++ (required)" dcpp

!ifdef X64
  ;Check if we have a valid PROGRAMFILES64 enviroment variable
  StrCmp $PROGRAMFILES64 $PROGRAMFILES32 0 _64_ok
  MessageBox MB_YESNO|MB_ICONEXCLAMATION "This installer will try to install the 64-bit version of StrongDC++ but this doesn't seem to be 64-bit operating system. Do you want to continue?" IDYES _64_ok
  Quit
  _64_ok:
!endif

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Check for existing settings in the profile first
  IfFileExists "$DOCUMENTS\StrongDC++\*.xml" 0 check_programdir
  MessageBox MB_YESNO|MB_ICONQUESTION "Settings of a previous installation of StrongDC++ has been found in the user profile, do you want to backup settings and queue? (You can find them in $DOCUMENTS\StrongDC++\BACKUP later)" IDNO no_backup
  CreateDirectory "$DOCUMENTS\StrongDC++\BACKUP\"
  CopyFiles "$DOCUMENTS\StrongDC++\*.xml" "$DOCUMENTS\StrongDC++\BACKUP\"
  goto no_backup

check_programdir:
  ; Maybe we're upgrading from an older version so lets check the old Settings directory
  IfFileExists "$INSTDIR\Settings\*.xml" 0 check_dcpp
  MessageBox MB_YESNO|MB_ICONQUESTION "A previous installation of StrongDC++ has been found in the target folder, do you want to backup settings and queue? (You can find them in $INSTDIR\Settings\BACKUP later)" IDNO no_backup
  CreateDirectory "$INSTDIR\Settings\BACKUP\"
  CopyFiles "$INSTDIR\Settings\*.xml" "$INSTDIR\Settings\BACKUP\"

check_dcpp:
  ; Lets check the profile for possible DC++ settings to import
  IfFileExists "$APPDATA\DC++\*.xml" 0 no_backup
  MessageBox MB_YESNO|MB_ICONQUESTION "Settings of an existing DC++ installation has been found in the user profile, do you want to import settings and queue?" IDNO no_backup
  CreateDirectory "$DOCUMENTS\StrongDC++\"
  CopyFiles "$APPDATA\DC++\*.xml" "$DOCUMENTS\StrongDC++\"
  CopyFiles "$APPDATA\DC++\*.dat" "$DOCUMENTS\StrongDC++\"
  CopyFiles "$APPDATA\DC++\*.txt" "$DOCUMENTS\StrongDC++\"

no_backup:
  ; Put file there
  File "changelog.txt"
  File "changelog-en.txt"
  File "dcppboot.xml"
  File "StrongDC.pdb"
  File "StrongDC.exe"
  File "License.txt"
  File "License-GeoIP.txt"
  File "License-OpenSSL.txt"
  File "EN_Example.xml"
  File "unicows.dll"
  
  ; Uncomment to get DCPP core version from StrongDC.exe we just installed and store in $1
  ;Function GetDCPlusPlusVersion
  ;        Exch $0;
  ;	GetDllVersion "$INSTDIR\$0" $R0 $R1
  ;	IntOp $R2 $R0 / 0x00010000
  ;	IntOp $R3 $R0 & 0x0000FFFF
  ;	IntOp $R4 $R1 / 0x00010000
  ;	IntOp $R5 $R1 & 0x0000FFFF
  ;        StrCpy $1 "$R2.$R3$R4$R5"
  ;        Exch $1
  ;FunctionEnd

  ; Push "StrongDC.exe"
  ; Call "GetDCPlusPlusVersion"
  ; Pop $1

  ; Get StrongDC version we just installed and store in $2
  MoreInfo::GetProductVersion "$INSTDIR\StrongDC.exe"
  Pop $2
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\StrongDC++ "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "DisplayIcon" '"$INSTDIR\StrongDC.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "DisplayName" "StrongDC++ $2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "DisplayVersion" "$2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "Publisher" "Big Muscle"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "URLInfoAbout" "http://strongdc.sourceforge.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "URLUpdateInfo" "http://strongdc.sourceforge.net/download.php"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "HelpLink" "http://strongdc.sourceforge.net/forum/"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++" "NoRepair" "1"
  
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "IP -> Country mappings"
  SetOutPath $INSTDIR
  File "GeoIPCountryWhois.csv"
SectionEnd

; optional section
Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\StrongDC++"
  CreateShortCut "$SMPROGRAMS\StrongDC++\StrongDC++.lnk" "$INSTDIR\StrongDC.exe" "" "$INSTDIR\StrongDC.exe" 0 "" "" "StrongDC++ File Sharing Application"
  CreateShortCut "$SMPROGRAMS\StrongDC++\License.lnk" "$INSTDIR\License.txt"
  ;CreateShortCut "$SMPROGRAMS\StrongDC++\Help.lnk" "$INSTDIR\DCPlusPlus.chm"
  CreateShortCut "$SMPROGRAMS\StrongDC++\Changelog.lnk" "$INSTDIR\changelog-en.txt"
  CreateShortCut "$SMPROGRAMS\StrongDC++\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Kobolok Emoticon Pack" 
  SetOutPath $INSTDIR
  File /r "EmoPacks"
SectionEnd

Section "Czech language file" 
  SetOutPath $INSTDIR
  File "CZ_Example.xml"
SectionEnd

Section "Store settings in your user profile folder" loc
  ; Change to nonlocal dcppboot if the checkbox left checked
  SetOutPath $INSTDIR
  File /oname=dcppboot.xml "dcppboot.nonlocal.xml" 
SectionEnd

Function .onSelChange
  ; Do not show the warning on XP or older
  StrCmp $R8 "0" skip
  ; Show the warning only once
  StrCmp $R9 "1" skip
  SectionGetFlags ${loc} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCmp $0 ${SF_SELECTED} skip
    StrCpy $R9 "1"
    MessageBox MB_OK|MB_ICONEXCLAMATION "If you want to keep your settings in the program directory using Windows Vista or later make sure that you DO NOT install StrongDC++ to the 'Program files' folder!!! This can lead to abnormal behaviour like loss of settings or downloads!"
skip:
FunctionEnd


Function .onInit
  ; Check for Vista+
  ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  IfErrors xp_or_below nt
nt:
  StrCmp $R8 '6.0' vistaplus
  StrCmp $R8 '6.1' 0 xp_or_below
vistaplus:
  StrCpy $R8 "1"
  goto end
xp_or_below:
  StrCpy $R8 "0"
end:
  ; Set the program component really required (read only)
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${dcpp} $0
FunctionEnd

; uninstall stuff

UninstallText "This will uninstall StrongDC++. Hit the Uninstall button to continue."

; special uninstall section.
Section "un.Uninstall"
  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\StrongDC++"
  DeleteRegKey HKLM SOFTWARE\StrongDC++
  ; remove files
  Delete "$INSTDIR\changelog.txt"
  Delete "$INSTDIR\changelog-en.txt"
  Delete "$INSTDIR\dcppboot.xml"
  Delete "$INSTDIR\StrongDC.pdb"
  Delete "$INSTDIR\StrongDC.exe"
  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\License-GeoIP.txt"
  Delete "$INSTDIR\License-OpenSSL.txt"
  Delete "$INSTDIR\CZ_Example.xml"
  Delete "$INSTDIR\EN_Example.xml"
  Delete "$INSTDIR\unicows.dll"
  Delete "$INSTDIR\GeoIPCountryWhois.csv"
  ; remove EmoPacks dir
  RMDir /r "$INSTDIR\EmoPacks"
  
  ; Remove registry entries
  ; Assume they are all registered to us
  DeleteRegKey HKCU "Software\Classes\dchub"
  DeleteRegKey HKCU "Software\Classes\adc"
  DeleteRegKey HKCU "Software\Classes\adcs"
  DeleteRegKey HKCU "Software\Classes\magnet"

  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe

  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\StrongDC++\*.*"
  ; remove directories used.
  RMDir "$SMPROGRAMS\StrongDC++"

  MessageBox MB_YESNO|MB_ICONQUESTION "Also remove queue and all settings?" IDYES kill_dir

  RMDir "$INSTDIR"
  goto end_uninstall

kill_dir:
  ; delete program directory
  RMDir /r "$INSTDIR"
  ; delete profile data directories
  RMDir /r "$DOCUMENTS\StrongDC++"
  RMDir /r "$LOCALAPPDATA\StrongDC++"

end_uninstall:

SectionEnd

; eof
