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
// Object: class helper to find stack size of a function by calling it
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "..\..\Tools\String\AnsiUnicodeConvert.h"
#include "..\..\Tools\PE\PE.h"
#include "..\..\Tools\Exception\HardwareException.h"

class CFindStackSizeByCall
{
public:
    enum CONSTS
    {
        MAX_NUMBER_OF_PARAM         =0x00000FFF,
        SUCCESS_RESULT_MASK         =0xFFFFF000,
        RESULT_MASK                 =0x00000FFF,
        SUCCESS                     =0xBAFFE000,
        FAILURE                     =0x0BAD0000
    };

    enum ERROR_CODES
    {
        ERROR_CODE_NO_ERROR=0,
        ERROR_CODE_ADDRESS_IS_NOT_IN_AN_EXECUTABLE_SECTION=1,
        ERROR_CODE_SOFTWARE_EXCEPTION_INSIDE_CALL=2,
        ERROR_CODE_HARDWARE_EXCEPTION_INSIDE_CALL=3,
        ERROR_CODE_LOAD_LIBRARY_ERROR=4,
        ERROR_CODE_GET_PROC_ADDRESS_ERROR=5,
        ERROR_CODE_MEMORY_ALLOCATION_ERROR=6,
        ERROR_CODE_PARSING_PE_ERROR=7,
    };

    static ERROR_CODES FindStackSizeByCall(TCHAR* pszLibName,TCHAR* pszFuncName,DWORD Ordinal,BOOL bOrdinal,DWORD* pStackSize);

private:
    static ERROR_CODES FindStackSizeByCall(FARPROC Function,DWORD* pStackSize);
};
