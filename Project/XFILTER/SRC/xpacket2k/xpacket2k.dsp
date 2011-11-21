# Microsoft Developer Studio Project File - Name="xpacket2k" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=xpacket2k - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xpacket2k.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xpacket2k.mak" CFG="xpacket2k - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xpacket2k - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "xpacket2k - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "xpacket2k - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f xpacket2k.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "xpacket2k.exe"
# PROP BASE Bsc_Name "xpacket2k.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.bat %DDKROOT% free F: F:\xfilter2\xpacket2k %SystemRoot%"
# PROP Rebuild_Opt "/a"
# PROP Target_File "xpacket2k.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "xpacket2k - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f xpacket2k.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "xpacket2k.exe"
# PROP BASE Bsc_Name "xpacket2k.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.bat %DDKROOT% checked F: F:\XFILTER2\xpacket2k %SystemRoot%"
# PROP Rebuild_Opt "/a"
# PROP Target_File "xpacket2k.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "xpacket2k - Win32 Release"
# Name "xpacket2k - Win32 Debug"

!IF  "$(CFG)" == "xpacket2k - Win32 Release"

!ELSEIF  "$(CFG)" == "xpacket2k - Win32 Debug"

!ENDIF 

# Begin Group "build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Build.bat
# End Source File
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# Begin Source File

SOURCE=.\Xprecomp.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BaseHook.c
# End Source File
# Begin Source File

SOURCE=.\NdisHook.c
# End Source File
# Begin Source File

SOURCE=.\NdisHook.h
# End Source File
# Begin Source File

SOURCE=.\NtApi.c
# End Source File
# Begin Source File

SOURCE=.\NtApi.h
# End Source File
# Begin Source File

SOURCE=.\Process.c
# End Source File
# Begin Source File

SOURCE=.\Process.h
# End Source File
# Begin Source File

SOURCE=.\XFilter.c
# End Source File
# Begin Source File

SOURCE=.\XFilter.h
# End Source File
# Begin Source File

SOURCE=.\Xpacket.c
# End Source File
# Begin Source File

SOURCE=.\Xpacket.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\ControlCode.h
# End Source File
# Begin Source File

SOURCE=..\Common\FILT.h
# End Source File
# Begin Source File

SOURCE=.\MemoryAcl.c
# End Source File
# Begin Source File

SOURCE=.\MemoryAcl.h
# End Source File
# Begin Source File

SOURCE=.\NetBios.c
# End Source File
# Begin Source File

SOURCE=.\NetBios.h
# End Source File
# Begin Source File

SOURCE=.\Packet.c
# End Source File
# Begin Source File

SOURCE=.\Packet.h
# End Source File
# Begin Source File

SOURCE=.\PacketBuffer.c
# End Source File
# Begin Source File

SOURCE=.\PacketBuffer.h
# End Source File
# Begin Source File

SOURCE=.\xfilter.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\Session\Session.c
# End Source File
# End Target
# End Project
