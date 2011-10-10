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
void __fastcall mButton_Click(PVOID pObject,PVOID sender,PVOID e);// Button_Click(PVOID pObject,object sender,System.EventArgs e)
// TAKE CARE OF X86 .NET __FASTCALL IMPLEMENTATION  !!!
// see MS Common Language Infrastructure (CLI) PARTITION II  15.5.6.1 Standard 80x86 calling convention

BOOL __stdcall m1BeforeButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
BOOL __stdcall m2BeforeButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
BOOL __stdcall m1AfterButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
BOOL __stdcall m2AfterButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);


///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning)
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(".NET@NET_Target.exe@0x06000005"),_T("NET_Target.Form1::button_Click"),(FARPROC)mButton_Click,4,0},
    {_T(""),_T(""),NULL,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// Before API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayBeforeAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(".NET@NET_Target.exe@0x06000005"),_T("NET_Target.Form1::button_Click"),(FARPROC)m1BeforeButton_Click,4,0},
    {_T(".NET@NET_Target.exe@0x06000005"),_T("NET_Target.Form1::button_Click"),(FARPROC)m2BeforeButton_Click,4,0},
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// After API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayAfterAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(".NET@NET_Target.exe@0x06000005"),_T("NET_Target.Form1::button_Click"),(FARPROC)m1AfterButton_Click,4,0},
    {_T(".NET@NET_Target.exe@0x06000005"),_T("NET_Target.Form1::button_Click"),(FARPROC)m2AfterButton_Click,4,0},
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////// NEW API DEFINITION //////////////////////////////
/////////////////////// You don't need to export these functions //////////////
///////////////////////////////////////////////////////////////////////////////

// TAKE CARE OF X86 .NET __FASTCALL IMPLEMENTATION  !!!
// see MS Common Language Infrastructure (CLI) PARTITION II  15.5.6.1 Standard 80x86 calling convention
void __fastcall mButton_Click(PVOID pObject,PVOID sender,PVOID e)
{
    MessageBox(0,_T("Overrided Button Click"),0,MB_ICONINFORMATION);
    return;
}

BOOL __stdcall m1BeforeButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop pre api call chain functions
{
    MessageBox(0,_T("Before Button Click 1"),0,MB_ICONINFORMATION);
    return TRUE;
}
BOOL __stdcall m2BeforeButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop pre api call chain functions
{
    MessageBox(0,_T("Before Button Click 2"),0,MB_ICONINFORMATION);
    return TRUE;
}
BOOL __stdcall m1AfterButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop pre api call chain functions
{
    MessageBox(0,_T("After Button Click 1"),0,MB_ICONINFORMATION);
    return TRUE;
}
BOOL __stdcall m2AfterButton_Click(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)// return FALSE to stop pre api call chain functions
{
    MessageBox(0,_T("After Button Click 2"),0,MB_ICONINFORMATION);
    return TRUE;
}

