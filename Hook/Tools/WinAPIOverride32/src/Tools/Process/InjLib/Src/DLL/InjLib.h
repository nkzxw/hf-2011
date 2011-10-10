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

#ifndef INJLIB_H
#define INJLIB_H

#include <windows.h>

#define INJLIB_WAITTIMEOUT 30000 // INFINITE can cause DeadLock if host process is in debug mode

#ifdef UNICODE
#define InjectLib InjectLibW
#define EjectLib  EjectLibW
#else
#define InjectLib InjectLibA
#define EjectLib  EjectLibA
#endif   // !UNICODE

extern "C" __declspec(dllexport) BOOL WINAPI InjectLibA (DWORD dwProcessID, LPCSTR lpszLibFile);
extern "C" __declspec(dllexport) BOOL WINAPI InjectLibW (DWORD dwProcessID, LPCWSTR lpszLibFile);
extern "C" __declspec(dllexport) BOOL WINAPI EjectLibA(DWORD dwProcessID, PCSTR pszLibFile);
extern "C" __declspec(dllexport) BOOL WINAPI EjectLibW(DWORD dwProcessID, PCWSTR pszLibFile);



#endif
