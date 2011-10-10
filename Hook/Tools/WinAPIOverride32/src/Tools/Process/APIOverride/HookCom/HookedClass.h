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
// Object: manage informations on a hooked class
//-----------------------------------------------------------------------------

#pragma once

#include "COM_include.h"
#include "DefinesAndStructs.h"
#include "HookCom.h"
#include "HookedInterface.h"
#include "HookedObject.h"
#include "postapicallhooks.h"
#include "../../../Com/GUIDStringConvert.h"
#include "../../../LinkList/LinkListSimple.h"
#include "idispatchresultselection.h"

class CHookedClass
{
private:
    typedef enum tagDllType
    {
        DLL_TYPE_UNKNOWN,
        DLL_TYPE_STANDARD,
        DLL_TYPE_JIT_COMPILED
    }DLL_TYPE;



    // parameter provide to ParseCOMMonitoringFileLine function
    typedef struct  tagParseCOMMonitoringFileLineParam
    {
        CHookedClass*   pHookedClass;
        CHookedObject*  pHookedObject;
        IUnknown*       pInterfaceAssociatedToIID;
        BOOL            bAtLeastOneMethodHasBeenHooked;
        IID*            pIid;
        CLinkList*      pLinkListOfBaseInterfacesID;
        BOOL            HookQueriedByAutoMonitoring;
        MONITORING_FILE_INFOS* SpecifiedpMonitoringFileInfo; // forced pMonitoringFileInfo
        MONITORING_FILE_INFOS* pAlreadyHookingMonitoringFileInfo;
    }PARSE_COM_MONITORING_FILE_LINE_PARAM,*PPARSE_COM_MONITORING_FILE_LINE_PARAM;

    typedef struct tagComOptionalParameters
    {
        BOOL HookDataSet;
        LINKLIST_POSTAPICALL_HOOKDATA HookData;
    }COM_OPTIONAL_PARAMETERS,*PCOM_OPTIONAL_PARAMETERS;

    HANDLE HookedInterfacesHeap;
    HANDLE HookedObjectsHeap;
    BOOL bHookedByAutoMonitoring;// to know if class has already been hooked by auto monitoring
    BOOL bSupportIDispatchEx;// To know if class support the fucking IDispatchEx Interface
    DLL_TYPE DllType;// dll owning object information (unknown,JIT compiled, standard)
    BYTE* IUnknownVTBLAddress;// vtbl address of IUnknown interface of interface having created
    HMODULE AssociatedModuleBaseAddress;// module handle associated to class (can be wrong as pIUnknown vtbl address is used, and it can be that pIUnknown vtbl is in another module) 

    static BOOL __stdcall PostQueryInterfaceCallHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
    static BOOL __stdcall PostReleaseCallHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
    static BOOL __stdcall PreReleaseCallHook(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
    static BOOL ParseCOMMonitoringFileLine(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);

    void ReportNotReleasedObject(CHookedObject* pHookedObject);

    BOOL AddIUnknownPostHooks(IUnknown* pInterface,IID* pIid);
    BOOL AddPostReleaseCallHook(IUnknown* pInterface,CHookedInterface* pHookedInterface);
    BOOL AddPostQueryInterfaceCallHook(IUnknown* pInterface,CHookedInterface* pHookedInterface);
    BOOL SetMonitoringHookFromIDispatchParsing();

    BOOL ReleaseCallBack(IUnknown* pIUnknown);
    BOOL QueryInterfaceCallBack(IUnknown* pIUnknown,IUnknown* pInterface,IID* pIid);

    CHookedObject* GetHookedObject(IUnknown* pIUnknown,BOOL* pIsGoingToBeDestroyed,BOOL* pbLocked);

    MONITORING_FILE_INFOS* GetOrCreateMonitoringFileInfo(TCHAR* pszMonitoringFileName);
    CHookedInterface* GetOrCreateHookedInterface(IUnknown* pInterfaceAssociatedToIID,IID* pIid);
    BOOL IsInterfaceHookedByAutoMonitoring(IUnknown* pInterfaceAssociatedToIID,IID* pIid);
    BOOL FindAssociatedDllName(CHookedObject* pHookedObject);
    BOOL IsAssociatedDllInSameSpaceAddress();
    BOOL AreHooksStillInstalled();
    BOOL CheckAndRestoreClassHooks(CHookedObject* pHookedObject);
    void ReInitialize();
    void Initialize();
    BOOL Unhook();

    BOOL ComOptionalParametersSplit(TCHAR* pszAPIOptionalParameters,TCHAR** ppszComOptionalParameters);
    BOOL ComOptionalParametersParse(TCHAR* pszComOptionalParameters,COM_OPTIONAL_PARAMETERS* pComOptionalParameters);
    BOOL ComOptionalParametersApply(API_INFO* pApiInfo,COM_OPTIONAL_PARAMETERS* pComOptionalParameters);
    
public:
    CLSID Clsid;
    DWORD NbReleaseProcessing;
    BOOL bIDispatchParsingHasBeenTried;// flag to know if IDispatch parsing has been tried
    BOOL bIDispatchParsingSuccessFull;// flag to know if IDispatch parsing has been successful
    CHookedInterface* pInterfaceExposedByIDispatch;// direct pointer to the result of IDispatch parsing interface (avoid to search inside pLinkListHookedInterfaces)

    CLinkListSimple* pLinkListHookedInterfaces;// link list of CHookedInterface*
    CLinkListSimple* pLinkListHookedObjects;// link list of CHookedObject*

    TCHAR AssociatedModuleName[MAX_PATH];// module name associated to class (can be wrong as pIUnknown vtbl address is used, and it can be that pIUnknown vtbl is in another module)
    BOOL ParseIDispatch(IUnknown* pObject,IID* pIID);
    CHookedInterface* GetHookedInterface(IUnknown* pInterfaceAssociatedToIID,IID* pIid);
    BOOL GetMethodInfoForHook(  CHookedObject* pHookedObject,
                                IUnknown* pInterfaceAssociatedToIID,
                                CLinkList* pLinkListOfBaseInterfacesID,
                                HOOK_DEFINITION_INFOS* pHookDefinitionInfos,
                                TCHAR* FunctionName,
                                TCHAR* pszFileName,
                                DWORD dwLineNumber,
                                tagHookType HookType,
                                BOOL bAutoHook,
                                OUT CMethodInfo** ppMethodInfo,
                                OUT BOOL* pbMethodInfoWasAlreadyExisting);

    CHookedClass(CLSID* pClsid);
    ~CHookedClass(void);
    HMODULE GetAssociatedModuleBaseAddress();
    BOOL IsAssociatedDllLoaded();

    BOOL AddAutoHookedObject(IUnknown* pObject,IID* pIid);
    CHookedObject* AddObject(IUnknown* pObject,IID* pIid);
    BOOL RemoveObject(IUnknown* pObject);
    
    BOOL Unhook(BOOL bLinkListHookedClassesLocked);
    
    DWORD GetNumberOfHookedObjects();
    CHookedObject* GetInternallyCreatedObject();
    void FreeAllHookedObjectsAndReportNotReleased();
    CHookedObject* GetObject(IUnknown* pObject,IID* pIid);

    BOOL SetMonitoringHookFromFile(CHookedObject* pHookedObject,
                                    IUnknown* pInterface,
                                    IID* pIid,
                                    IID* pFileIid,
                                    CLinkList* pLinkListOfBaseInterfacesID,
                                    BOOL bHookedByAutoMonitoring,
                                    MONITORING_FILE_INFOS* SpecifiedpMonitoringFileInfo,
                                    MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo);
    BOOL AddMonitoringHookForObjectMethod(CHookedObject* pHookedObject,
                                        IUnknown* pInterfaceAssociatedToIID,
                                        CLinkList* pLinkListOfBaseInterfacesID,
                                        MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                        BOOL bAutoHook,
                                        HOOK_DEFINITION_INFOS* pHookDefinitionInfos,
                                        TCHAR* pszFunctionDescription,
                                        TCHAR* pszFileName,
                                        DWORD dwLineNumber,
                                        MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                        BOOL* pbParsingError);
    BOOL AddFakingHookForObjectMethod(FAKING_DLL_INFOS* pFakingDllInfos,
                                        CHookedObject* pHookedObject,
                                        IUnknown* pInterfaceAssociatedToIID,
                                        FAKING_HOOK_INFORMATIONS* pFakingHookInfos,
                                        FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos);

};
