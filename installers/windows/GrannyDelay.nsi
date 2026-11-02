; Granny Delay VST3 - Windows NSIS installer
; Build with:
;   makensis /DVST3_DIR=C:\path\to\GrannyDelay.vst3 /DOUT_DIR=C:\out /DLICENSE_FILE=C:\path\to\LICENSE GrannyDelay.nsi

!define NAME "Granny Delay"
!define FILENAME "GrannyDelay"
!define VERSION "1.0.0"
!define PUBLISHER "Aron H"

!ifndef VST3_DIR
  !error "Pass /DVST3_DIR=<path to GrannyDelay.vst3 folder>"
!endif
!ifndef OUT_DIR
  !define OUT_DIR "."
!endif
!ifndef LICENSE_FILE
  !define LICENSE_FILE "..\..\LICENSE"   ; CI passes an absolute path with /DLICENSE_FILE=...
!endif

Unicode true
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!include "x64.nsh"

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${LICENSE_FILE}"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Name "${NAME} ${VERSION}"
OutFile "${OUT_DIR}\GrannyDelay-Setup.exe"
InstallDir "$COMMONFILES64\VST3\${FILENAME}.vst3"

Section "Install"
    SetRegView 64
    SetOutPath "$INSTDIR"
    File /r "${VST3_DIR}\*.*"

    CreateDirectory "$PROGRAMFILES64\${NAME}"
    WriteUninstaller "$PROGRAMFILES64\${NAME}\Uninstall.exe"

    WriteRegStr HKLM "Software\${PUBLISHER}\${NAME}" "Version" "${VERSION}"
    WriteRegStr HKLM "Software\${PUBLISHER}\${NAME}" "InstallDir" "$INSTDIR"

    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "Publisher" "${PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" '"$PROGRAMFILES64\${NAME}\Uninstall.exe"'
SectionEnd

Section "Uninstall"
    SetRegView 64
    RMDir /r "$COMMONFILES64\VST3\${FILENAME}.vst3"
    Delete "$PROGRAMFILES64\${NAME}\Uninstall.exe"
    RMDir "$PROGRAMFILES64\${NAME}"
    DeleteRegKey HKLM "Software\${PUBLISHER}\${NAME}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
SectionEnd
