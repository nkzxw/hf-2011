# Microsoft Developer Studio Project File - Name="WehnTrust" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WehnTrust - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WehnTrust.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WehnTrust.mak" CFG="WehnTrust - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WehnTrust - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WehnTrust - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WehnTrust - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O1 /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX"Precomp.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 psapi.lib comdlg32.lib comctl32.lib ws2_32.lib advapi32.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /fixed:no
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "WehnTrust - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX"Precomp.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 psapi.lib comdlg32.lib comctl32.lib ws2_32.lib advapi32.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "WehnTrust - Win32 Release"
# Name "WehnTrust - Win32 Debug"
# Begin Group "Env"

# PROP Default_Filter ""
# Begin Group "Images"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Env\Images\Kernel32.cpp
# End Source File
# Begin Source File

SOURCE=.\Env\Images\Kernel32.h
# End Source File
# Begin Source File

SOURCE=.\Env\Images\Ntdll.cpp
# End Source File
# Begin Source File

SOURCE=.\Env\Images\Ntdll.h
# End Source File
# Begin Source File

SOURCE=.\Env\Images\User32.cpp
# End Source File
# Begin Source File

SOURCE=.\Env\Images\User32.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Env\Env.cpp
# End Source File
# Begin Source File

SOURCE=.\Env\Env.h
# End Source File
# Begin Source File

SOURCE=.\Env\Image.cpp
# End Source File
# Begin Source File

SOURCE=.\Env\Image.h
# End Source File
# End Group
# Begin Group "Ui"

# PROP Default_Filter ""
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Ui\Dialogs\AboutDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\AboutDialog.h
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\AddEditExemptionDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\AddEditExemptionDialog.h
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\ExemptionsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\ExemptionsDialog.h
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\LicenseDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\LicenseDialog.h
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\StatusDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\StatusDialog.h
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\WehnTrustDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialogs\WehnTrustDialog.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Ui\Dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Dialog.h
# End Source File
# Begin Source File

SOURCE=.\Ui\Locale.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Locale.h
# End Source File
# Begin Source File

SOURCE=.\Ui\SystemTray.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\SystemTray.h
# End Source File
# Begin Source File

SOURCE=.\Ui\Ui.cpp
# End Source File
# Begin Source File

SOURCE=.\Ui\Ui.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resources\DialogBanner.bmp
# End Source File
# Begin Source File

SOURCE=.\Resources\WehnTrustDisabled.ico
# End Source File
# Begin Source File

SOURCE=.\Resources\WehnTrustEnabled.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\Precomp.cpp
# ADD CPP /Yc"Precomp.h"
# End Source File
# Begin Source File

SOURCE=.\Precomp.h
# End Source File
# Begin Source File

SOURCE=.\WehnTrust.cpp
# End Source File
# Begin Source File

SOURCE=.\WehnTrust.h
# End Source File
# Begin Source File

SOURCE=.\WehnTrust.rc
# End Source File
# End Target
# End Project
