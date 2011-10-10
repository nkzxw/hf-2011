#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include "defines.h"
#include "Struct.h"
#include "../GenericFakeAPI.h"

typedef CLinkListItem* (__stdcall *pfQueryEmptyItemAPIInfo)();
typedef CLinkListItem* (__stdcall *pfGetAssociatedItemAPIInfo)(PBYTE pbAPI,BOOL* pbAlreadyHooked);
typedef BOOL (__stdcall *pfHookAPIFunction)(API_INFO *pAPIInfo);
typedef BOOL (__stdcall *pfInitializeApiInfo)(API_INFO *pAPIInfo,TCHAR* ModuleName,TCHAR* FunctionName);
typedef BOOL (__stdcall *pfFreeApiInfoItem)(CLinkListItem* pItemAPIInfo);

typedef BOOL (__stdcall *pfParseFunctionDescription)(TCHAR* pszFunctionDescription,OUT TCHAR** ppszFunctionName, OUT TCHAR** ppszParameters, OUT TCHAR** ppszOptions,OUT tagCALLING_CONVENTION* pCallingConvention);
typedef BOOL (__stdcall *pfParseParameters)(API_INFO *pAPIInfo,TCHAR* pszParameters,TCHAR* pszFileName,DWORD dwLineNumber);
typedef BOOL (__stdcall *pfParseOptions)(API_INFO *pAPIInfo,TCHAR* pszOptions,BOOL bAlreadyHooked,TCHAR* pszFileName,DWORD dwLineNumber);
typedef int (__stdcall *pfDynamicMessageBoxInDefaultStation)(IN HWND hWnd,IN TCHAR* lpText,IN TCHAR* lpCaption,IN UINT uType);

typedef BOOL (__stdcall *pfAddPostApiCallCallBack)(CLinkListItem* pItemAPIInfo,BOOL bNeedToBeHooked,HMODULE OwnerModule,pfPostApiCallCallBack FunctionPointer,PVOID UserParam);
typedef BOOL (__stdcall *pfRemovePostApiCallCallBack)(CLinkListItem* pItemAPIInfo,pfPostApiCallCallBack FunctionPointer,BOOL bRestoreOriginalBytes);
typedef BOOL (__stdcall *pfAddPreApiCallCallBack)(CLinkListItem* pItemAPIInfo,BOOL bNeedToBeHooked,HMODULE OwnerModule,pfPostApiCallCallBack FunctionPointer,PVOID UserParam);
typedef BOOL (__stdcall *pfRemovePreApiCallCallBack)(CLinkListItem* pItemAPIInfo,pfPostApiCallCallBack FunctionPointer,BOOL bRestoreOriginalBytes);

typedef BOOL (__stdcall *pfUnHookIfPossible)(CLinkListItem* pItemAPIInfo,BOOL bRestoreOriginalBytes);
typedef PBYTE (__stdcall *pfGetWinAPIOverrideFunctionDescriptionAddress)(TCHAR* pszModuleName,TCHAR* pszAPIName,BOOL* pbExeDllInternalHook,BOOL* pbPointerHook);

typedef CLinkList* (__stdcall *pfCreateParameterConditionalLogContentListIfDoesntExist)(PARAMETER_INFOS* pParameter);
typedef CLinkList* (__stdcall *pfCreateParameterConditionalBreakContentListIfDoesntExist)(PARAMETER_INFOS* pParameter);
typedef void (__stdcall *pfFreeOptionalParametersMemory)(API_INFO *pAPIInfo);

typedef BOOL (__stdcall *pfReportMessage)(tagReportMessageType ReportMessageType,TCHAR* pszMsg);

typedef BOOL (__stdcall *pfGetModuleNameAndRelativeAddressFromCallerAbsoluteAddress)(IN PBYTE pOriginAddress,
                                                                                     OUT HMODULE* pCallingModuleHandle,
                                                                                     OUT TCHAR* pszModuleName,
                                                                                     OUT PBYTE* pRelativeAddress,
                                                                                     OUT BOOL* pbShouldLog,
                                                                                     BOOL TakeAnotherSnapshotIfNotFound,
                                                                                     BOOL ContinueEvenIfShouldNotBeLogged);

typedef BOOL (__stdcall *pfSetDefaultStation)(OUT HWINSTA* pCurrentStation, OUT HWINSTA* pOldStation,OUT HDESK* pCurrentDesktop,OUT HDESK* pOldDesktop);
typedef BOOL (__stdcall *pfRestoreStation)(IN HWINSTA CurrentStation,IN HWINSTA OldStation,IN HDESK CurrentDesktop,IN HDESK OldDesktop);
typedef BOOL (__stdcall *pfCanWindowInteract)();
typedef HANDLE (__stdcall *pfAdjustThreadSecurityAndLaunchDialogThread)(LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter);
typedef BOOL (__stdcall *pfProcessInternalCallRequestEx)(FARPROC pFunc,tagCALLING_CONVENTION CallingConvention,int NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pRet,double* pFloatingResult,DWORD ThreadId,DWORD dwTimeOut);
typedef void (__stdcall *pfNetExceptionSearchFunctionLeaveCallBack)(BOOL ExceptionCatchedInsideFunction,API_INFO* pApiInfo);
typedef void (__stdcall *pfNetExceptionCatcherEnterCallBack)();
typedef void (__stdcall *pfNetTlsRestoreHookAddress)(API_INFO* pAPIInfo);