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
// fake API prototype MUST HAVE THE SAME PARAMETERS 
// for the same calling convention see MSDN : 
// "Using a Microsoft modifier such as __cdecl on a data declaration is an outdated practice"
///////////////////////////////////////////////////////////////////////////////

int WINAPI mMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);
int WINAPI mMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType);

///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning)
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T("User32.dll"),_T("MessageBoxA"),(FARPROC)mMessageBoxA,StackSizeOf(HWND)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR)+StackSizeOf(UINT),0},
    {_T("User32.dll"),_T("MessageBoxW"),(FARPROC)mMessageBoxW,StackSizeOf(HWND)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(UINT),0},
    {_T(""),_T(""),NULL,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// Before API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayBeforeAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// After API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayAfterAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};
///////////////////////////////////////////////////////////////////////////////
///////////////////////////// NEW API DEFINITION //////////////////////////////
/////////////////////// You don't need to export these functions //////////////
///////////////////////////////////////////////////////////////////////////////

int WINAPI mMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType)
{
    UNREFERENCED_PARAMETER(lpCaption);

    char szMsg[2*MAX_PATH];
    strcpy(szMsg,"Oups !! It's not your text !!!\r\n[Begin of] Original Text :\r\n");
    strncat(szMsg,lpText,MAX_PATH-strlen(szMsg)-1);
    szMsg[MAX_PATH-1]=0;// assume end of string
    return MessageBoxA(NULL,szMsg,"API Override:MessageBoxA",uType);
}


int WINAPI mMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType)
{
    UNREFERENCED_PARAMETER(lpCaption);
    wchar_t szMsg[2*MAX_PATH];

    wcscpy(szMsg,L"Oups !! It's not your text !!!\r\n[Begin of] Original Text :\r\n");
    wcsncat(szMsg,lpText,MAX_PATH-wcslen(szMsg)-1);
    szMsg[MAX_PATH-1]=0;// assume end of string

    return MessageBoxW(NULL,szMsg,L"API Override:mMessageBoxW",uType);
}