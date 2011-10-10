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
// You just need to edit this file to add new fake api 
// WARNING YOUR FAKE API MUST HAVE THE SAME PARAMETERS AND CALLING CONVENTION AS THE REAL ONE,
//                  ELSE YOU WILL GET STACK ERRORS

///////////////////////////////////////////////////////////////////////////////
// hook functions declaration
///////////////////////////////////////////////////////////////////////////////
BOOL __stdcall MessageBoxesPreApiCall(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
BOOL __stdcall MessageBoxAPostApiCall(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
BOOL __stdcall MessageBoxWPostApiCall(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
BOOL __stdcall MessageBoxWPostApiCall2(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);

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
// Before API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayBeforeAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T("User32.dll"),_T("MessageBoxA"),(FARPROC)MessageBoxesPreApiCall,StackSizeOf(HWND)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR)+StackSizeOf(UINT),0,0},
    {_T("User32.dll"),_T("MessageBoxW"),(FARPROC)MessageBoxesPreApiCall,StackSizeOf(HWND)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(UINT),0,(PVOID)1},
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// After API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayAfterAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T("User32.dll"),_T("MessageBoxA"),(FARPROC)MessageBoxAPostApiCall,StackSizeOf(HWND)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR)+StackSizeOf(UINT),0,0},
    {_T("User32.dll"),_T("MessageBoxW"),(FARPROC)MessageBoxWPostApiCall,StackSizeOf(HWND)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(UINT),0,0},
    // the following is added to show you the chain mechanism, this declaration could have been in another overriding dll
    {_T("User32.dll"),_T("MessageBoxW"),(FARPROC)MessageBoxWPostApiCall2,StackSizeOf(HWND)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(UINT),0,0},
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};



// In this example, for pre api call, we have decided to use a single callback function.
// Original hooked function (MessageBoxA or MessageBoxW) is found thanks to UserParam (0 or 1)
// For post api call we have used 2 different callback functions.
// It has been done to show you the different ways you can use
//
// Notice : to see OutputDebugString report if your application is not running inside a debugger, 
//          use DbgView freeware (DebugView of Sysinternals - www.sysinternals.com)


BOOL __stdcall MessageBoxesPreApiCall(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop pre api call chain functions
{
    // to change parameters you can directly act on the stack or value pointed by the stack

    
    // Reminder : MessageBox(HWND hWnd,LPCTSTR lpText,LPCSTR lpCaption,UINT uType);
    HWND Hwnd=(HWND)(*((PBYTE*)pEspArgs));
    PBYTE TextString=*((PBYTE*)(pEspArgs+1*sizeof(PBYTE)));
    PBYTE TextCaption=*((PBYTE*)(pEspArgs+2*sizeof(PBYTE)));
    UINT Type=(UINT)(*((PBYTE*)(pEspArgs+3*sizeof(PBYTE))));

    // Warning you have to do your own memory protection
    if (IsBadReadPtr(TextString,sizeof(CHAR)))
        return TRUE;
    if (IsBadReadPtr(TextCaption,sizeof(CHAR)))
        return TRUE;

  
    // change messagebox Type by acting on stack
    PBYTE* pArgType=(PBYTE*)(pEspArgs+3*sizeof(PBYTE));
    Type&=~MB_ICONERROR;
    Type&=~MB_ICONWARNING;
    Type|=MB_ICONINFORMATION;
    *pArgType=(PBYTE)Type;

    // you can change some registers if you want
    pBeforeCallRegisters->eax=12;

    if (UserParam==0)// messageBoxA
    {
        OutputDebugString(_T("MessageBoxesPreApiCall for MessageBoxA\r\n"));
        OutputDebugStringA((LPCSTR)TextString);
        OutputDebugStringA((LPCSTR)TextCaption);

        // avoid to override statically allocated string
        if (!IsBadWritePtr(TextString,3*sizeof(CHAR)))
        {
            // change messagebox text by acting on value pointed by stack
            PCHAR pcTextString=(PCHAR)TextString;
            pcTextString[0]='O';
            pcTextString[1]='K';
            pcTextString[2]=' ';
        }
    }
    else // messageBoxW
    {
        OutputDebugString(_T("MessageBoxesPreApiCall for MessageBoxW\r\n"));
        OutputDebugStringW((LPCWSTR)TextString);
        OutputDebugStringW((LPCWSTR)TextCaption);

        // avoid to override statically allocated string
        if (!IsBadWritePtr(TextString,3*sizeof(WCHAR)))
        {
            // change messagebox text by acting on value pointed by stack
            PWCHAR pwTextString=(PWCHAR)TextString;
            pwTextString[0]='O';
            pwTextString[1]='K';
            pwTextString[2]=' ';
        }
        
    }

    // continue chain (if any other func)
    return TRUE;
}
BOOL __stdcall MessageBoxAPostApiCall(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop calling post api call chain functions
{
    // to change parameters you can directly act on the stack or value pointed by the stack
    // can be used to change output parameter values

    // you can change after call register, by the way to modify the returned value
    pAfterCallRegisters->eax=0;// always set returned value to 0

    OutputDebugString(_T("MessageBoxAPostApiCall\r\n"));
    // continue chain (if any other func)
    return TRUE;
}

BOOL __stdcall MessageBoxWPostApiCall(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop calling post api call chain functions
{
    OutputDebugString(_T("MessageBoxWPostApiCall\r\n"));

    // to continue chain (allow MessageBoxWPostApiCall2 call)
    return TRUE;

    // to stop chain (don't allow MessageBoxWPostApiCall2 call)
//  return FALSE;
}

BOOL __stdcall MessageBoxWPostApiCall2(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop calling post api call chain functions
{
    OutputDebugString(_T("MessageBoxWPostApiCall2\r\n"));
    // continue chain
    return TRUE;
}