/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: Process helper class. It allows easy operation on processes
//         like retrieving name or checking if a process is still running
//-----------------------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <Tlhelp32.h>

class CProcessHelper
{

public:
    static BOOL IsAlive(DWORD ProcessID);
    static BOOL GetProcessName(DWORD ProcessID,TCHAR* ProcessName);
    static BOOL GetProcessFullPath(DWORD ProcessID,TCHAR* ProcessName);
    static BOOL IsFirstModuleLoaded(DWORD ProcessID);
    static BOOL SuspendProcess(DWORD ProcessID);
    static BOOL SuspendProcess(HANDLE hSnapshot,DWORD ProcessID);
    static BOOL ResumeProcess(DWORD ProcessID);
    static BOOL ResumeProcess(HANDLE hSnapshot,DWORD ProcessID);
    static BOOL Is32bitsProcess(DWORD ProcessID,OUT BOOL* pbIs32bits);
    static BOOL Is32bitsProcessHandle(HANDLE hProcess,OUT BOOL* pbIs32bits);
};