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

// to avoid rebasing, use the linker/advanced/BaseAddress option 
#include "../../_Common_Files/GenericFakeAPI.h"

// You just need to edit this file to 
//   - add new fake api 
//   - add pre/post api call hook
//   - add COM object creation hooks
//
// WARNING YOUR FAKE API MUST HAVE THE SAME PARAMETERS AND CALLING CONVENTION AS THE REAL ONE,
//                  ELSE YOU WILL GET STACK ERRORS

// what is done in this example : 
// Install a post hook for Microsoft TreeView Control 6 SP4 (CLSID {C74190B6-8589-11D1-B16A-00C0F0283628})
// to avoid user to retrieve ISpecifyPropertyPages Interface from IUnknown (this interface may will be retrieved using other interfaces)
// Install a fake hook for Microsoft TreeView Control 6 SP4 (CLSID {C74190B6-8589-11D1-B16A-00C0F0283628})
// to fake the AboutBox function
#include <ocidl.h>//for IID_ISpecifyPropertyPages

BOOL __stdcall PostApiCallCallBackQueryInterface(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
HRESULT __stdcall mAboutBox(IDispatch* pIDispatch);

///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning)
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)


    // reminder: - __stdcall AboutBox(IDispatch* pObject) vtbl index=56 <-- definition and vtbl index provided by WinApiOverride "Com Tools"/"Show Methods addresses"
    //           - Com definition : COM@CLSID@IIDVTBLIndex=IID:Index
    // Microsoft TreeView Control 6 SP4 (CLSID {C74190B6-8589-11D1-B16A-00C0F0283628})
    // IDispatch Interface ({00020400-0000-0000-C000-000000000046})
    // AboutBox VTBL Index : 56
    {_T("COM@{C74190B6-8589-11D1-B16A-00C0F0283628}@IIDVTBLIndex={00020400-0000-0000-C000-000000000046}:56"),
        _T("AboutBox"),
        (FARPROC)mAboutBox,
        StackSizeOf(IDispatch*),
        0 // FirstBytesCanExecuteAnywhereSize is no use for com
    },

    {_T(""),_T(""),NULL,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// Before API call array. Pre API call hooks are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayBeforeAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// After API call array. Post API call hooks are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayAfterAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)

    // reminder: - QueryInterface(IUnknown* pObject,IID* pIid,PVOID* ppInterface) vtbl index=0
    //           - Com definition : COM@CLSID@IIDVTBLIndex=IID:Index
    // Microsoft TreeView Control 6 SP4 (CLSID {C74190B6-8589-11D1-B16A-00C0F0283628})
    // IUnknown Interface ({00000000-0000-0000-C000-000000000046})
    // QueryInterface IUnknown VTBL Index : 0
    {_T("COM@{C74190B6-8589-11D1-B16A-00C0F0283628}@IIDVTBLIndex={00000000-0000-0000-C000-000000000046}:0"),
        _T("QueryInterface"),
        (FARPROC)PostApiCallCallBackQueryInterface, // post call
        StackSizeOf(IUnknown*)+StackSizeOf(IID*)+StackSizeOf(PVOID*),
        0, // FirstBytesCanExecuteAnywhereSize is no use for com
        0  // no user param
    },

    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

/*
BOOL __stdcall PreApiCallCallBack(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);// return FALSE to stop pre api call chain functions
BOOL __stdcall PostApiCallCallBack(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);// return FALSE to stop calling post api call chain functions
*/
BOOL __stdcall PostApiCallCallBackQueryInterface(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)
{
    // reminder: - QueryInterface(IUnknown* pObject,IID* pIid,PVOID* ppInterface)
    IID* pIid=*((IID**)(pEspArgs+StackSizeOf(IUnknown*)));
    IUnknown** ppInterface=*((IUnknown***)(pEspArgs+StackSizeOf(IUnknown*)+StackSizeOf(IID*)));

    // first of all do parameter checking
    if (IsBadReadPtr(pIid,sizeof(IID)))
        return TRUE;

    if (IsBadReadPtr(ppInterface,sizeof(IUnknown*)))
        return TRUE;

    if (IsBadReadPtr(*ppInterface,sizeof(IUnknown)))
        return TRUE;

    // fake only QueryInterface(IID_ISpecifyPropertyPages)
    if (!IsEqualIID(*pIid,IID_ISpecifyPropertyPages))
        return TRUE;

    // query interface has increase internal object count, we have to release it
    (*ppInterface)->Release();

    // we can set content of ppInterface to NULL but it's quite dangerous
    // *ppInterface=NULL;

    // set result of QueryInterface to E_NOINTERFACE
    pAfterCallRegisters->eax=E_NOINTERFACE;

    return TRUE;
}

HRESULT __stdcall mAboutBox(IDispatch* pIDispatch)
{
    UNREFERENCED_PARAMETER(pIDispatch);
    MessageBox(NULL,_T("I don't remember :D"),_T("AboutBox [Overrided]"),MB_OK|MB_TOPMOST|MB_ICONINFORMATION);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Export COMObjectCreationCallBack function only if you want to monitor COM object creation
// parameters :
//          in : CLSID* pClsid : pointer to CLSID used for object creation
//               IID* pIid : pointer to IID used for object creation
//               PVOID pObject : pointer to newly created com object
//               PRE_POST_API_CALL_HOOK_INFOS* pHookInfos : struct defined in GenericFakeAPI.h see definition for help
// return : tells to continue or stop callback chain for this object creation
//          TRUE to continue to report this object creation in other COMObjectCreationCallBack functions of other overriding dll
//          FALSE to stop reporting this object creation in other COMObjectCreationCallBack functions of other overriding dll
///////////////////////////////////////////////////////////////////////////////
//extern "C" __declspec(dllexport) 
//BOOL __stdcall COMObjectCreationCallBack(CLSID* pClsid,IID* pIid,PVOID pObject,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos)// return FALSE to stop calling COMObjectCreationCallBack chain functions
//{
//    // your filters for pClsid
//    
//    // your filters for pIid
//
//    // you can change object properties or call object methods here
//
//    MessageBox(0,_T("Com Object Created"),_T("Information"),MB_OK|MB_TOPMOST|MB_ICONINFORMATION);
//
//    return TRUE;
//}