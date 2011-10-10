#pragma once

#include "../GenericFakeAPI.h"
#include "HookComExport.h"

#define HOOK_COM_QUERY_INTERFACE_VTBL_INDEX                 0 // 0 based index
#define HOOK_COM_RELEASE_VTBL_INDEX                         2 // 0 based index

#define HOOK_COM_ICLASSFACTORY_CREATEINSTANCE_VTBL_INDEX    3 // 0 based index

#define COM_DEFINITION_CURRENT_IID_VTBL_INDEX   _T("VTBLIndex=") // VTBLIndex of current IID // used for dynamic vtbl hooking
#define COM_DEFINITION_EXPOSED_THROW_IDISPATCH  _T("ExposedThrowIDispatch") // names exposed throw IDispatch
#define COM_DEFINITION_BASE_IID                 _T("BaseIID=")              // for base classe interface
#define COM_DEFINITION_OBJECT_VTBL_INDEX        _T("IIDVTBLIndex=")         // IIDVTBLIndex={IID}:Index  // used for static monitoring files
#define COM_DEFINITION_OBJECT_CREATION          _T("ObjectCreation")

#define STRING_GUID_SIZE 40
#define STRING_PROGID_MAX_SIZE MAX_PATH
#define STRING_IID_MAX_SIZE MAX_PATH

enum tagVTBLInfoType
{
    VTBL_INFO_TYPE_EXPOSED_THROW_IDISPATCH,
    VTBL_INFO_TYPE_OBJECT_FULL_IID,
    VTBL_INFO_TYPE_OBJECT_IID,
};

typedef struct tagHookDefinitionInfos
{
    tagVTBLInfoType VTBLInfoType;
    IID             InterfaceID;
    DWORD           VTBLIndex;
}HOOK_DEFINITION_INFOS,*PHOOK_DEFINITION_INFOS;

typedef struct tagFakingHookInformations
{
    CLSID Clsid;
    tagHookFakingType FakingType;
    DWORD FakingDllIndex;
    HOOK_DEFINITION_INFOS HookDefinitionInfos;
    STRUCT_FAKE_API_WITH_USERPARAM FakeApiInfos;
    FAKING_DLL_INFOS* pFakingDllInfos;
}FAKING_HOOK_INFORMATIONS,*PFAKING_HOOK_INFORMATIONS;


enum LinkListPostApiCallHookDataValueInfo
{
    HOOK_DATA_VALUE_INFO_NONE=0,
    HOOK_DATA_VALUE_INFO_BY_STACK,
    HOOK_DATA_VALUE_INFO_BY_STACK_REF,
    HOOK_DATA_VALUE_INFO_BY_VALUE
};

class CCOMCreationPostApiCallHooks;
typedef struct tagLinkListPostApiCallHookData
{
    CLinkListItem* pItemAPIInfo;        // pointer to associated hook information struct
    PBYTE PostApiCallFunctionPointer;   // pointer to callback function (COMObjectsArrayCreationPostHook, COMObjectCreationPostHook)

    // if com object pointer is in returned value
    BOOL COMObjectPtrInReturnedValue;
    // else if function as a MULTI_QI* param
    BOOL COMArrayStackIndexSet;
    DWORD COMArrayStackIndex;// position in bytes where the array address is in the stack
    DWORD COMArrayCountStackIndex;// position in bytes where the array elements count address is in the stack
    // else
    DWORD COMObjectStackIndex;
    BOOL COMObjectStackIndexSet;

    // IID infos
    LinkListPostApiCallHookDataValueInfo IIDInfos;
    DWORD IIDStackIndex;
    IID   IIDValue;

    // CLSID infos
    LinkListPostApiCallHookDataValueInfo CLSIDInfos;
    DWORD CLSIDStackIndex;
    CLSID CLSIDValue;

    CCOMCreationPostApiCallHooks* pCCOMCreationPostApiCallHooks;

}LINKLIST_POSTAPICALL_HOOKDATA,*PLINKLIST_POSTAPICALL_HOOKDATA;