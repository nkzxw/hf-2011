#include <windows.h>
#include "../../_Common_Files/GenericFakeAPI.h"
// You just need to edit this file to add new fake api 
// WARNING YOUR FAKE API MUST HAVE THE SAME PARAMETERS AND CALLING CONVENTION AS THE REAL ONE,
//                  ELSE YOU WILL GET STACK ERRORS

///////////////////////////////////////////////////////////////////////////////
// fake API prototype MUST HAVE THE SAME PARAMETERS 
// for the same calling convention see MSDN : 
// "Using a Microsoft modifier such as __cdecl on a data declaration is an outdated practice"
///////////////////////////////////////////////////////////////////////////////
UINT WINAPI mGetDriveTypeA(IN LPCSTR lpRootPathName);
UINT WINAPI mGetDriveTypeW(IN LPCWSTR lpRootPathName);

///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning)
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T("Kernel32.dll"),_T("GetDriveTypeA"),(FARPROC)mGetDriveTypeA,StackSizeOf(LPCSTR),0},
    {_T("Kernel32.dll"),_T("GetDriveTypeW"),(FARPROC)mGetDriveTypeW,StackSizeOf(LPCWSTR),0},
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
UINT WINAPI mGetDriveTypeA(IN LPCSTR lpRootPathName)
{
    UNREFERENCED_PARAMETER(lpRootPathName);
    // return Cdrom type
    return DRIVE_CDROM;
}
UINT WINAPI mGetDriveTypeW(IN LPCWSTR lpRootPathName)
{
    UNREFERENCED_PARAMETER(lpRootPathName);
    // return Cdrom type
    return DRIVE_CDROM;
}


