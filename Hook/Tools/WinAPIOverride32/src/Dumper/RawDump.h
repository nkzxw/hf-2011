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
// Object: manages the raw dump dialog
//         
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <Tlhelp32.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <stdio.h>

#include "resource.h"
#include "../Tools/APIError/APIError.h"
#include "../Tools/Process/Memory/ProcessMemory.h"
#include "../Tools/Process/ProcessHelper/ProcessHelper.h"
#include "../Tools/KernelMemoryAccess/KernelMemoryAccessInterface.h"


void RawDump(HINSTANCE hInstance,HWND hWndParent);