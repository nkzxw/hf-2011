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


///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning)
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
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
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

/*
BOOL __stdcall PreApiCallCallBack(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);// return FALSE to stop pre api call chain functions
BOOL __stdcall PostApiCallCallBack(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);// return FALSE to stop calling post api call chain functions
*/


///////////////////////////////////////////////////////////////////////////////
// Export COMObjectCreationCallBack function only if you want to monitor COM object creation
// parameters :
//          in : CLSID* pClsid : pointer to CLSID used for object creation
//               IID* pIid : pointer to IID used for object creation
//               PVOID pObject : pointer to newly created com object
//               PRE_POST_API_CALL_HOOK_INFOS* pHookInfos : struct defined in ExportedStructs.h see definition for help
// return : tells to continue or stop callback chain for this object creation
//          TRUE to continue to report this object creation in other COMObjectCreationCallBack functions of other overriding dll
//          FALSE to stop reporting this object creation in other COMObjectCreationCallBack functions of other overriding dll
///////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) 
BOOL __stdcall COMObjectCreationCallBack(CLSID* pClsid,IID* pIid,PVOID pObject,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos)
{
    // your filters for pClsid
    
    // your filters for pIid

    // you can change object properties or call object methods here

    MessageBox(0,_T("Com Object Created"),_T("Information"),MB_OK|MB_TOPMOST|MB_ICONINFORMATION);

    return TRUE;
}