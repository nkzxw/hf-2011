# Microsoft Developer Studio Project File - Name="xfilter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=xfilter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xfilter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xfilter.mak" CFG="xfilter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xfilter - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "xfilter - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xfilter - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Mpr.lib ws2_32.lib rasapi32.lib /nologo /subsystem:windows /machine:I386 /out:"../Release/xfilter.exe"
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "xfilter - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Mpr.lib ws2_32.lib rasapi32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../Release/xfilter.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "xfilter - Win32 Release"
# Name "xfilter - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\About.cpp
# End Source File
# Begin Source File

SOURCE=.\AclApp.cpp
# End Source File
# Begin Source File

SOURCE=.\AclDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\AclIcmp.cpp
# End Source File
# Begin Source File

SOURCE=.\AclIcmpSet.cpp
# End Source File
# Begin Source File

SOURCE=.\AclNet.cpp
# End Source File
# Begin Source File

SOURCE=.\AclNetSet.cpp
# End Source File
# Begin Source File

SOURCE=.\AclNnb.cpp
# End Source File
# Begin Source File

SOURCE=.\AclNnbSet.cpp
# End Source File
# Begin Source File

SOURCE=.\AclQuery.cpp
# End Source File
# Begin Source File

SOURCE=.\AclSet.cpp
# End Source File
# Begin Source File

SOURCE=.\AclSub.cpp
# End Source File
# Begin Source File

SOURCE=.\AclTime.cpp
# End Source File
# Begin Source File

SOURCE=.\AclTorjan.cpp
# End Source File
# Begin Source File

SOURCE=.\AclWeb.cpp
# End Source File
# Begin Source File

SOURCE=.\AclWebSet.cpp
# End Source File
# Begin Source File

SOURCE=.\LogSub.cpp
# End Source File
# Begin Source File

SOURCE=.\MainDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MonitorSub.cpp
# End Source File
# Begin Source File

SOURCE=.\OnLine.cpp
# End Source File
# Begin Source File

SOURCE=.\ParameterSub.cpp
# End Source File
# Begin Source File

SOURCE=.\serviceControl.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\SystemSet.cpp
# End Source File
# Begin Source File

SOURCE=.\xfilter.cpp
# End Source File
# Begin Source File

SOURCE=.\xfilter.rc
# End Source File
# Begin Source File

SOURCE=.\xfilterDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\About.h
# End Source File
# Begin Source File

SOURCE=.\AclApp.h
# End Source File
# Begin Source File

SOURCE=.\AclDialog.h
# End Source File
# Begin Source File

SOURCE=.\AclIcmp.h
# End Source File
# Begin Source File

SOURCE=.\AclIcmpSet.h
# End Source File
# Begin Source File

SOURCE=.\AclNet.h
# End Source File
# Begin Source File

SOURCE=.\AclNetSet.h
# End Source File
# Begin Source File

SOURCE=.\AclNnb.h
# End Source File
# Begin Source File

SOURCE=.\AclNnbSet.h
# End Source File
# Begin Source File

SOURCE=.\AclQuery.h
# End Source File
# Begin Source File

SOURCE=.\AclSet.h
# End Source File
# Begin Source File

SOURCE=.\AclSub.h
# End Source File
# Begin Source File

SOURCE=.\AclTime.h
# End Source File
# Begin Source File

SOURCE=.\AclTorjan.h
# End Source File
# Begin Source File

SOURCE=.\AclWeb.h
# End Source File
# Begin Source File

SOURCE=.\AclWebSet.h
# End Source File
# Begin Source File

SOURCE=.\LogSub.h
# End Source File
# Begin Source File

SOURCE=.\MainDlg.h
# End Source File
# Begin Source File

SOURCE=.\MonitorSub.h
# End Source File
# Begin Source File

SOURCE=.\OnLine.h
# End Source File
# Begin Source File

SOURCE=.\ParameterSub.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\serviceControl.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SystemSet.h
# End Source File
# Begin Source File

SOURCE=.\xfilter.h
# End Source File
# Begin Source File

SOURCE=.\xfilterDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\RES\AclApp.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclApp1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclApp2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclIcmp.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclIcmp1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclIcmp2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclNetbt.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclNnb.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclNnb1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclNnb2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclWeb.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclWeb1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\AclWeb2.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Alert.ico
# End Source File
# Begin Source File

SOURCE=.\RES\ALERTSET.ICO
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap10.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap11.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap3.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap4.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap5.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap6.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap7.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap8.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bitmap9.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00003.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00004.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00005.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00006.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00007.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00008.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00009.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00010.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\bmp00011.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Button.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Button100.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Button101.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Button102.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Button103.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Button_Mask.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\CloseButton.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\CloseButton.ico
# End Source File
# Begin Source File

SOURCE=.\RES\CloseButton1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\CloseButton2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\COMMONSET.ICO
# End Source File
# Begin Source File

SOURCE=.\RES\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\Res\DenyEx1.ico
# End Source File
# Begin Source File

SOURCE=.\RES\Green1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\green2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\green3.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\Res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\Res\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\Res\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\Res\ico00006.ico
# End Source File
# Begin Source File

SOURCE=.\Res\ico00007.ico
# End Source File
# Begin Source File

SOURCE=.\RES\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\RES\idr_main.ico
# End Source File
# Begin Source File

SOURCE=.\RES\Info.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Info.ico
# End Source File
# Begin Source File

SOURCE=.\RES\LogApp.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogApp1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogApp2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogIcmp.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogIcmp1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogIcmp2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogNetbt.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogNnb.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogNnb1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\LogNnb2.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Message.ico
# End Source File
# Begin Source File

SOURCE=.\RES\minButton.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorApp.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorApp1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorApp2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorIcmp.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorIcmp1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorIcmp2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorLine.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorLine1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorLine2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorNetbt.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorNnb.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorNnb1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorNnb2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorPort.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorPort1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\MonitorPort2.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\NULL.ico
# End Source File
# Begin Source File

SOURCE=.\Res\NullApp.ico
# End Source File
# Begin Source File

SOURCE=.\RES\Passeck1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\Passeck2.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\PassEx1.ico
# End Source File
# Begin Source File

SOURCE=.\Res\QueryEx1.ico
# End Source File
# Begin Source File

SOURCE=.\RES\radio1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\radio2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\red1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\red2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\red3.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\small_de.ico
# End Source File
# Begin Source File

SOURCE=.\Res\small_pa.ico
# End Source File
# Begin Source File

SOURCE=.\Res\small_qu.ico
# End Source File
# Begin Source File

SOURCE=.\Res\splash.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\SystemSet.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\TopMost.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\TopMost1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\TopMost2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\xfilter.bmp
# End Source File
# Begin Source File

SOURCE=.\res\xfilter.ico
# End Source File
# Begin Source File

SOURCE=.\res\xfilter.rc2
# End Source File
# Begin Source File

SOURCE=.\RES\XFILTER0.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\XFILTER00.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\XFILTER1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\XFILTER_ACL.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\XFILTER_EX.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\XFILTER_QUERY.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\yellow1.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\yellow2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\yellow3.bmp
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\BtnST.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\BtnST.h
# End Source File
# Begin Source File

SOURCE=..\Common\ColorStatic.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ColorStatic.h
# End Source File
# Begin Source File

SOURCE=..\Common\ControlCode.h
# End Source File
# Begin Source File

SOURCE=..\Common\Debug.h
# End Source File
# Begin Source File

SOURCE=..\Common\FILT.h
# End Source File
# Begin Source File

SOURCE=..\Common\GuiRes.h
# End Source File
# Begin Source File

SOURCE=..\Common\PacketMonitor.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\PacketMonitor.h
# End Source File
# Begin Source File

SOURCE=..\Common\PasseckDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\PasseckDialog.h
# End Source File
# Begin Source File

SOURCE=..\Common\Process.cxx
# End Source File
# Begin Source File

SOURCE=..\Common\Process.h
# End Source File
# Begin Source File

SOURCE=..\Common\TypeStruct.h
# End Source File
# Begin Source File

SOURCE=..\Common\XCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\XCommon.h
# End Source File
# Begin Source File

SOURCE=..\Common\XFile.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\XFile.h
# End Source File
# Begin Source File

SOURCE=..\Common\XFileRes.h
# End Source File
# Begin Source File

SOURCE=..\Common\XInstall.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\XInstall.h
# End Source File
# Begin Source File

SOURCE=..\Common\XLogFile.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\XLogFile.h
# End Source File
# End Group
# Begin Group "Other Gui Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\HtmlHelp\htmlhelp.h
# End Source File
# Begin Source File

SOURCE=.\Hyperlink.cpp
# End Source File
# Begin Source File

SOURCE=.\Hyperlink.h
# End Source File
# Begin Source File

SOURCE=.\Internet.cpp
# End Source File
# Begin Source File

SOURCE=.\Internet.h
# End Source File
# Begin Source File

SOURCE=.\Register.cpp
# End Source File
# Begin Source File

SOURCE=.\Register.h
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\SystemTray.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemTray.h
# End Source File
# Begin Source File

SOURCE=.\HtmlHelp\htmlhelp.lib
# End Source File
# End Group
# Begin Group "Shell Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Shell\Shell.h
# End Source File
# Begin Source File

SOURCE=.\Shell\Shell.lib
# End Source File
# End Group
# End Target
# End Project
