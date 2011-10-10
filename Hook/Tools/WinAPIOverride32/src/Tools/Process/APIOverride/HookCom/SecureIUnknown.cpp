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
// Object: provide secure IUnknown methods to catch atl COM implementation which throws errors
//-----------------------------------------------------------------------------

#include "secureiunknown.h"

// Some lamers use bad AddRef / Release / QueryInterface calling convention
// so we have to not check
#pragma runtime_checks( "", off )

#define SaveStack(EbpBasedVarContainingEsp) __asm     \
{                                            \
    __asm mov [EbpBasedVarContainingEsp],esp               \
}
#define RestoreStack(EbpBasedVarContainingEsp) __asm     \
{                                            \
    __asm mov esp,[EbpBasedVarContainingEsp]               \
}

ULONG CSecureIUnknown::AddRef(IUnknown* pObject)
{
    ULONG SavedEsp;
    ULONG RetValue=0;
    try
    {
#ifndef _DEBUG // function can be hooked, so we may want to let debugger catch errors
        CExceptionHardware::RegisterTry();
#endif

        if (pObject)
        {
            SaveStack(SavedEsp);
            RetValue=pObject->AddRef();
            RestoreStack(SavedEsp);
        }
    }
    catch( CExceptionHardware e )
    {
        RetValue=0;
    }
    catch (...)
    {
    	RetValue=0;
    }
    return RetValue;
}

ULONG CSecureIUnknown::Release(IUnknown* pObject)
{
    ULONG SavedEsp;
    ULONG RetValue=0;
    try
    {
#ifndef _DEBUG // function can be hooked, so we may want to let debugger catch errors
        CExceptionHardware::RegisterTry();
#endif

        if (pObject)
        {
            SaveStack(SavedEsp);
            RetValue=pObject->Release();
            RestoreStack(SavedEsp);
        }
    }
    catch( CExceptionHardware e )
    {
        RetValue=0;
    }
    catch (...)
    {
        RetValue=0;
    }
    return RetValue;
}

HRESULT CSecureIUnknown::QueryInterface(
                       IUnknown* pObject,
                       REFIID iid,
                       void ** ppvObject
                       )
{
    ULONG SavedEsp;
    HRESULT hResult=E_FAIL;
    try
    {
#ifndef _DEBUG // function can be hooked, so we may want to let debugger catch errors
        CExceptionHardware::RegisterTry();
#endif

        if (pObject)
        {
            SaveStack(SavedEsp);
            hResult=pObject->QueryInterface(iid,ppvObject);
            RestoreStack(SavedEsp);

            if (SUCCEEDED(hResult))
            {
                // sometimes query interface return success, but failed...
                if (ppvObject==NULL)
                    hResult=E_FAIL;
            }
        }
    }
    catch( CExceptionHardware e )
    {
        hResult=E_FAIL;
    }
    catch (...)
    {
        hResult=E_FAIL;
    }
    return hResult;
}

#pragma runtime_checks( "", restore )