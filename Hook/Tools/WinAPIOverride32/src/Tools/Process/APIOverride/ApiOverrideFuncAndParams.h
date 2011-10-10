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
// Object: class helper for doing remote function call in other processes with apioverride
//         it manages encoding and decoding of function and parameters
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "InterProcessCommunication.h"

class CApiOverrideFuncAndParams
{
private:
    void FreeDecodedMembers();
    void FreeEncodedMembers();
public:
    // decoded members
    LPTSTR DecodedLibName;
    LPTSTR DecodedFuncName;
    DWORD DecodedNbParams;
    PSTRUCT_FUNC_PARAM DecodedParams;
    PBYTE DecodedReturnedValue;
    REGISTERS DecodedRegisters;
    double DecodedFloatingReturn;
    BOOL DecodedCallSuccess;
    PVOID DecodedID; // id for the call request
    DWORD DecodedThreadID;  // thread id in which we want to do the call
    DWORD DecodedTimeOut;
    tagCALLING_CONVENTION DecodedCallingConvention;

    // encoded members
    PBYTE EncodedBuffer;
    DWORD EncodedBufferSize;

    CApiOverrideFuncAndParams(void);
    ~CApiOverrideFuncAndParams(void);
    BOOL Encode(PVOID ID,LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,DWORD ThreadID,DWORD dwTimeOut,tagCALLING_CONVENTION CallingConvention);
    BOOL Encode(PVOID ID,LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE ReturnValue,double FloatingReturn,BOOL CallSuccess,DWORD ThreadID,DWORD dwTimeOut,tagCALLING_CONVENTION CallingConvention);
    BOOL Decode(PBYTE pBuffer);
};
