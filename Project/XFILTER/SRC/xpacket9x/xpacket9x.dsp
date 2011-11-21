# Microsoft Developer Studio Project File - Name="xpacket9x" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=xpacket9x - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xpacket9x.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xpacket9x.mak" CFG="xpacket9x - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xpacket9x - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "xpacket9x - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "xpacket9x - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f xpacket9x.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "xpacket9x.exe"
# PROP BASE Bsc_Name "xpacket9x.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build /f "xpacket.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "xpacket.vxd"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "xpacket9x - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f xpacket9x.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "xpacket9x.exe"
# PROP BASE Bsc_Name "xpacket9x.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build /f "xpacket.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "xpacket.vxd"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "xpacket9x - Win32 Release"
# Name "xpacket9x - Win32 Debug"

!IF  "$(CFG)" == "xpacket9x - Win32 Release"

!ELSEIF  "$(CFG)" == "xpacket9x - Win32 Debug"

!ENDIF 

# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\build.bat
# End Source File
# Begin Source File

SOURCE=.\xpacket.def
# End Source File
# Begin Source File

SOURCE=.\xpacket.mak
# End Source File
# Begin Source File

SOURCE=.\Xprecomp.h
# End Source File
# End Group
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Ndishook.c
# End Source File
# Begin Source File

SOURCE=.\Ndishook.h
# End Source File
# Begin Source File

SOURCE=.\process.c
# End Source File
# Begin Source File

SOURCE=.\process.h
# End Source File
# Begin Source File

SOURCE=.\xpacket.c
# End Source File
# Begin Source File

SOURCE=.\xpacket.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CommonFunction.c
# End Source File
# Begin Source File

SOURCE=.\CommonFunction.h
# End Source File
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
# End Target
# End Project
