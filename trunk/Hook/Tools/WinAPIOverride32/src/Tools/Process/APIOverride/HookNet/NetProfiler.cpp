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
#include "NetProfiler.h"
extern DWORD NetExceptionTlsIndex;
extern HOOK_NET_INIT HookNetInfos;
extern BOOL bApiOverrideDllLoaded;
extern LONG volatile ObjectsCount;
extern HOOK_NET_OPTIONS HookNetOptions;
extern ICorProfilerInfo* pCurrentCorProfiler;
extern CLinkListSimple* pCompiledFunctionList;
extern DWORD NetEnterLeaveTlsIndex;


CLinkListSimple* GetTlsEnterLeaveLinkedList()
{
    return (CLinkListSimple*)TlsGetValue(NetEnterLeaveTlsIndex);
}

void FreeTlsEnterLeaveLinkedList()
{
    CLinkListSimple* pLinkList=GetTlsEnterLeaveLinkedList();
    if (!pLinkList)
        return;
    HANDLE HeapHandle=pLinkList->GetHeap();
    HeapDestroy(HeapHandle);
    pLinkList->ReportHeapDestruction();
    delete pLinkList;
}

CLinkListSimple* GetOrCreateTlsEnterLeaveLinkedList()
{
    CLinkListSimple* pLinkList;
    pLinkList=GetTlsEnterLeaveLinkedList();
    if (pLinkList)
        return pLinkList;
    HANDLE HeapHandle=HeapCreate(0,4096,0);
    pLinkList=new CLinkListSimple();
    pLinkList->SetHeap(HeapHandle);
    TlsSetValue(NetEnterLeaveTlsIndex,(LPVOID)pLinkList);
    return pLinkList;
}

class CNetProfilerExceptionInfos
{
public:
    FunctionID functionId;
    BOOL ExceptionCatched;
    CNetProfilerExceptionInfos(UINT functionId)
    {
        this->functionId=functionId;
        this->ExceptionCatched=FALSE;
    }
    ~CNetProfilerExceptionInfos()
    {
    }
};

// see corprof.idl, cor.h, corhdr.h, corhlpr.h for documentation on interfaces
void __stdcall EnterStub( FunctionID functionID )
{
    if (!bApiOverrideDllLoaded)
        return;

    DWORD LastError=GetLastError();

    //TCHAR psz[MAX_PATH];
    //_stprintf(psz,_T("Enter function %x\r\n"),functionID);
    //OutputDebugString(psz);

    CLinkListSimple* pLinkList=GetOrCreateTlsEnterLeaveLinkedList();
    pLinkList->AddItem((PVOID)functionID);

End:
    SetLastError(LastError);
}

void __stdcall LeaveStub( FunctionID functionID )
{
    if (!bApiOverrideDllLoaded)
        return;
    DWORD LastError=GetLastError();

    //TCHAR psz[MAX_PATH];
    //_stprintf(psz,_T("Leave function %x\r\n"),functionID);
    //OutputDebugString(psz);

    CLinkListSimple* pLinkList=GetTlsEnterLeaveLinkedList();
    if (!pLinkList)
        goto End_EmptyList;
    if(!pLinkList->Head)
        goto End_EmptyList;
    API_INFO* pApiInfo=GetApiInfoFromFuncId(functionID);
    if (!pApiInfo)
        goto End;

    BOOL CallIsHooked=FALSE;
    // if secure hook, all function call are hooked --> ret address must be restored
    if (pApiInfo->FirstBytesCanExecuteAnywhereSize
        || pApiInfo->bFunctionPointer
        )
    {
        CallIsHooked=TRUE;
    }
    else
    {
        CallIsHooked=TRUE;

        // If |BlockingCall func can be hooked more than once
        //else function can be hooked only once in the stack
        if (!pApiInfo->BlockingCall)
        {
            // check if function is present more than once in stack
            CLinkListItem* pItem;
            for(pItem=pLinkList->Tail->PreviousItem;pItem;pItem=pItem->PreviousItem)
            {
                if ((FunctionID)pItem->ItemData==functionID)
                    CallIsHooked=FALSE;
            }
        }
    }
    if (CallIsHooked)
    {
        // restore hook return address
        HookNetInfos.NetTlsRestoreHookAddress(pApiInfo);
    }

End:
    pLinkList->RemoveItem(pLinkList->Tail);
End_EmptyList:
    SetLastError(LastError);
}

void __stdcall TailcallStub( FunctionID functionID )
{
    if (!bApiOverrideDllLoaded)
        return;
    DWORD LastError=GetLastError();

    //TCHAR psz[MAX_PATH];
    //_stprintf(psz,_T("Tailcall function %x\r\n"),functionID);
    //OutputDebugString(psz);

End:
    SetLastError(LastError);
}

void __declspec( naked ) EnterNaked()
{
    __asm
    {
        pushfd
        push eax
        push ecx
        push edx
        push [esp+20]
        call EnterStub
        pop edx
        pop ecx
        pop eax
        popfd
        ret 4
    }
}

void __declspec( naked ) LeaveNaked()
{
    __asm
    {
        pushfd
        push eax
        push ecx
        push edx
        push [esp+20]
        call LeaveStub
        pop edx
        pop ecx
        pop eax
        popfd
        ret 4
    }
}

void __declspec(naked) TailcallNaked()
{
    __asm
    {
        pushfd
        push eax
        push ecx
        push edx
        push [esp+20]
        call TailcallStub
        pop edx
        pop ecx
        pop eax
        popfd
        ret 4
    }
}





CNetProfiler::CNetProfiler()
{
    this->RefCount=0;
    InterlockedIncrement(&ObjectsCount);
}

CNetProfiler::~CNetProfiler()
{
    InterlockedDecrement(&ObjectsCount);
}

/////////////////////////
// IUnknown Interfaces
/////////////////////////
ULONG STDMETHODCALLTYPE CNetProfiler::AddRef()
{
	InterlockedIncrement(&this->RefCount);
    return this->RefCount;
}

ULONG STDMETHODCALLTYPE CNetProfiler::Release()
{
    ULONG localRefCount;
	InterlockedDecrement(&this->RefCount);
    localRefCount=this->RefCount;
    if (this->RefCount==0)
        delete this;
    return localRefCount;
}


#ifndef __ICorProfilerCallback2_INTERFACE_DEFINED__

// #define IID_ICorProfilerCallback2 "{8A8CC829-CCF2-49fe-BBAE-0F022228071A}" 
EXTERN_C const GUID IID_ICorProfilerCallback2 = 
{ 0x8A8CC829, 0xCCF2, 0x49fe, { 0xBB, 0xAE, 0x0F, 0x02, 0x22, 0x28, 0x07, 0x1A } };
#endif

#ifndef __ICorProfilerCallback3_INTERFACE_DEFINED__
// "4FD2ED52-7731-4b8d-9469-03D2CC3086C5"
EXTERN_C const IID IID_ICorProfilerCallback3 = 
{ 0x4FD2ED52, 0x7731, 0x4b8d, { 0x94, 0x69, 0x03, 0xD2, 0xCC, 0x30, 0x86, 0xC5 } };
#endif

HRESULT STDMETHODCALLTYPE CNetProfiler::QueryInterface( REFIID riid, void **ppInterface )
{
	if(IsEqualIID(riid,IID_IUnknown))
		*ppInterface = static_cast<ICorProfilerCallback*>(this);
	else if(IsEqualIID(riid,IID_ICorProfilerCallback))
		*ppInterface = static_cast<ICorProfilerCallback*>(this);
    else if(IsEqualIID(riid,IID_ICorProfilerCallback2))
        *ppInterface = static_cast<ICorProfilerCallback2*>(this);
    else if(IsEqualIID(riid,IID_ICorProfilerCallback3))
        *ppInterface = static_cast<ICorProfilerCallback3*>(this);
    else
    {
        *ppInterface=NULL;
        return E_NOINTERFACE;
    }
    this->AddRef();
    return S_OK;
    
}

/////////////////////////
// ICorProfilerCallback Interfaces
/////////////////////////
HRESULT STDMETHODCALLTYPE CNetProfiler::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON reason)
{
    UNREFERENCED_PARAMETER(reason);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::RuntimeSuspendFinished()
{
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::RuntimeResumeStarted()
{
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::MovedReferences(ULONG cMovedObjectIDRanges,
                                        UINT oldObjectIDRangeStart[],
                                        UINT newObjectIDRangeStart[],
                                        ULONG cObjectIDRangeLength[])
{
    UNREFERENCED_PARAMETER(cMovedObjectIDRanges);
    UNREFERENCED_PARAMETER(oldObjectIDRangeStart);
    UNREFERENCED_PARAMETER(newObjectIDRangeStart);
    UNREFERENCED_PARAMETER(cObjectIDRangeLength);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ObjectAllocated(UINT objectId, UINT classId)
{
    UNREFERENCED_PARAMETER(objectId);
    UNREFERENCED_PARAMETER(classId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ObjectsAllocatedByClass(ULONG cClassCount,
                                                UINT classIds[],
                                                ULONG cObjects[])
{
    UNREFERENCED_PARAMETER(cClassCount);
    UNREFERENCED_PARAMETER(classIds);
    UNREFERENCED_PARAMETER(cObjects);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ObjectReferences(UINT objectId, UINT classId,
                                        ULONG cObjectRefs, UINT objectRefIds[])
{
    UNREFERENCED_PARAMETER(objectId);
    UNREFERENCED_PARAMETER(classId);
    UNREFERENCED_PARAMETER(cObjectRefs);
    UNREFERENCED_PARAMETER(objectRefIds);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::RootReferences(ULONG cRootRefs, UINT rootRefIds[])
{
    UNREFERENCED_PARAMETER(cRootRefs);
    UNREFERENCED_PARAMETER(rootRefIds);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AppDomainCreationStarted(UINT appDomainId)
{
    UNREFERENCED_PARAMETER(appDomainId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AppDomainCreationFinished(UINT appDomainId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(appDomainId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AppDomainShutdownStarted(UINT appDomainId)
{
    UNREFERENCED_PARAMETER(appDomainId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AppDomainShutdownFinished(UINT appDomainId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(appDomainId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AssemblyLoadStarted(UINT assemblyId)
{   
    UNREFERENCED_PARAMETER(assemblyId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AssemblyLoadFinished(UINT assemblyId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(assemblyId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AssemblyUnloadStarted(UINT assemblyId)
{
    UNREFERENCED_PARAMETER(assemblyId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::AssemblyUnloadFinished(UINT assemblyId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(assemblyId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ModuleLoadStarted(UINT moduleId)
{
    UNREFERENCED_PARAMETER(moduleId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ModuleLoadFinished(UINT moduleId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(moduleId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ModuleUnloadStarted(UINT moduleId)
{
    UNREFERENCED_PARAMETER(moduleId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ModuleUnloadFinished(UINT moduleId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(moduleId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ModuleAttachedToAssembly(UINT moduleId, UINT assemblyId)
{
    UNREFERENCED_PARAMETER(moduleId);
    UNREFERENCED_PARAMETER(assemblyId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ClassLoadStarted(UINT classId)
{
    UNREFERENCED_PARAMETER(classId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ClassLoadFinished(UINT classId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(classId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ClassUnloadStarted(UINT classId)
{
    UNREFERENCED_PARAMETER(classId);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ClassUnloadFinished(UINT classId, HRESULT hrStatus)
{
    UNREFERENCED_PARAMETER(classId);
    UNREFERENCED_PARAMETER(hrStatus);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::FunctionUnloadStarted(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ThreadCreated(UINT threadId)
{
    UNREFERENCED_PARAMETER(threadId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ThreadDestroyed(UINT threadId)
{
    UNREFERENCED_PARAMETER(threadId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ThreadAssignedToOSThread(UINT managedThreadId, ULONG osThreadId)
{
    UNREFERENCED_PARAMETER(managedThreadId);
    UNREFERENCED_PARAMETER(osThreadId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingClientInvocationStarted()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingClientSendingMessage(GUID* pCookie, BOOL fIsAsync)
{
    UNREFERENCED_PARAMETER(pCookie);
    UNREFERENCED_PARAMETER(fIsAsync);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingClientReceivingReply(GUID* pCookie, BOOL fIsAsync)
{
    UNREFERENCED_PARAMETER(pCookie);
    UNREFERENCED_PARAMETER(fIsAsync);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingClientInvocationFinished()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingServerReceivingMessage(GUID* pCookie, BOOL fIsAsync)
{
    UNREFERENCED_PARAMETER(pCookie);
    UNREFERENCED_PARAMETER(fIsAsync);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingServerInvocationStarted()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingServerInvocationReturned()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RemotingServerSendingReply(GUID* pCookie, BOOL fIsAsync)
{
    UNREFERENCED_PARAMETER(pCookie);
    UNREFERENCED_PARAMETER(fIsAsync);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::UnmanagedToManagedTransition(UINT functionId, COR_PRF_TRANSITION_REASON reason)
{
    UNREFERENCED_PARAMETER(functionId);
    UNREFERENCED_PARAMETER(reason);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ManagedToUnmanagedTransition(UINT functionId, COR_PRF_TRANSITION_REASON reason)
{
    UNREFERENCED_PARAMETER(functionId);
    UNREFERENCED_PARAMETER(reason);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RuntimeSuspendAborted()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RuntimeResumeFinished()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RuntimeThreadSuspended(UINT threadId)
{
    UNREFERENCED_PARAMETER(threadId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::RuntimeThreadResumed(UINT threadId)
{
    UNREFERENCED_PARAMETER(threadId);
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionThrown(UINT thrownObjectId)
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionThrown\r\n"));
#endif
    if (!bApiOverrideDllLoaded)
        return S_OK;
    if (!HookNetOptions.MonitorException)
        return S_OK;

    TCHAR ExeceptionClassName[MAX_PATH]=_T("");
    TCHAR szMsg[2*MAX_PATH];

    // if we can get object class name
    if (this->GetObjectClassName(thrownObjectId,ExeceptionClassName,MAX_PATH))
        _sntprintf(szMsg,2*MAX_PATH,_T("Exception of type %s thrown in .NET module"),ExeceptionClassName);
    else
        _sntprintf(szMsg,2*MAX_PATH,_T("Exception thrown in .NET module"));

    szMsg[2*MAX_PATH-1]=0;
    ReportMessage(REPORT_MESSAGE_ERROR,szMsg);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionSearchFunctionEnter(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionSearchFunctionEnter\r\n"));
#endif

    if (!bApiOverrideDllLoaded)
        // return
        return S_OK;

    CNetProfilerExceptionInfos* pHookNetExceptionInfo=new CNetProfilerExceptionInfos(functionId);
    // store infos
    TlsSetValue(NetExceptionTlsIndex,pHookNetExceptionInfo);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionSearchFunctionLeave()
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionSearchFunctionLeave\r\n"));
#endif

    // if api override dll not loaded
    if (!bApiOverrideDllLoaded)
        // return
        return S_OK;

    // api override is loaded
    // --> if function is hooked unwind hook
    API_INFO* pApiInfo;
    CNetProfilerExceptionInfos* pHookNetExceptionInfo=(CNetProfilerExceptionInfos*)TlsGetValue(NetExceptionTlsIndex);
    if (!pHookNetExceptionInfo)
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())
            DebugBreak();
#endif
        return S_OK;
    }
    // get hook information for compiled function
    pApiInfo=(API_INFO*)GetApiInfoFromFuncId(pHookNetExceptionInfo->functionId);

    // do work
    HookNetInfos.NetExceptionSearchFunctionLeaveCallBack(pHookNetExceptionInfo->ExceptionCatched,pApiInfo);

    // delete pointer now and reset exception infos
    delete pHookNetExceptionInfo;
    TlsSetValue(NetExceptionTlsIndex,0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionSearchCatcherFound(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionSearchCatcherFound\r\n"));
#endif
    // if api override dll not loaded
    if (!bApiOverrideDllLoaded)
        // return
        return S_OK;

    CNetProfilerExceptionInfos* pHookNetExceptionInfo=(CNetProfilerExceptionInfos*)TlsGetValue(NetExceptionTlsIndex);
    if (!pHookNetExceptionInfo)
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())
            DebugBreak();
#endif
        return S_OK;
    }

    pHookNetExceptionInfo->ExceptionCatched=TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionSearchFilterEnter(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionSearchFilterEnter\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionSearchFilterLeave()
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionSearchFilterLeave\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionOSHandlerEnter(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionOSHandlerEnter\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionOSHandlerLeave(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionOSHandlerLeave\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionUnwindFunctionEnter(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionUnwindFunctionEnter\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionUnwindFunctionLeave()
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionUnwindFunctionLeave\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionUnwindFinallyEnter(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionUnwindFinallyEnter\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionUnwindFinallyLeave()
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionUnwindFinallyLeave\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionCatcherEnter(UINT functionId,UINT objectId)
{
    UNREFERENCED_PARAMETER(objectId);
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionCatcherEnter\r\n"));
#endif

    if (!bApiOverrideDllLoaded)
        return S_OK;

    // signal exception search has been finished --> allow to reset some flags in case an exception occurs in catcher
    HookNetInfos.NetExceptionCatcherEnterCallBack();

    if (!HookNetOptions.MonitorException)
        return S_OK;

    TCHAR ExeceptionClassName[MAX_PATH]=_T("");
    TCHAR FunctionName[MAX_PATH]=_T("");
    TCHAR szMsg[3*MAX_PATH];

    // get function name from our CFunctionInfo object instead of querying IMetadaImport
    pCompiledFunctionList->Lock();
    CLinkListItem* pItem;
    CFunctionInfoEx* pFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        if (pFunctionInfo->FunctionId==functionId)
        {
            pFunctionInfo->GetName(FunctionName,MAX_PATH);
            break;
        }

    }
    pCompiledFunctionList->Unlock();

    // if we can get object class name
    if (this->GetObjectClassName(objectId,ExeceptionClassName,MAX_PATH))
        _sntprintf(szMsg,3*MAX_PATH,_T("Exception of type %s thrown in .NET module catched by %s"),ExeceptionClassName,FunctionName);
    else
    {
        // if function name not found, that means function doesn't belong to the compiled ones
        if (FunctionName[0]==0)
        {
            _sntprintf(szMsg,3*MAX_PATH,_T("Exception thrown in .NET module catched by an unknwon function (may not catched) "));
        }
        else
            _sntprintf(szMsg,3*MAX_PATH,_T("Exception thrown in .NET module catched by %s"),FunctionName);
    }

    szMsg[3*MAX_PATH-1]=0;
    ReportMessage(REPORT_MESSAGE_INFORMATION,szMsg);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionCatcherLeave()
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionCatcherLeave\r\n"));
#endif
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionCLRCatcherFound(void)
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionCLRCatcherFound\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::ExceptionCLRCatcherExecute(void)
{
#ifdef _DEBUG
    OutputDebugString(_T("ExceptionCLRCatcherExecute\r\n"));
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::COMClassicVTableCreated(ClassID wrappedClassId, REFGUID implementedIID, VOID* pUnk, ULONG cSlots)
{
    UNREFERENCED_PARAMETER(wrappedClassId);
    UNREFERENCED_PARAMETER(implementedIID);
    UNREFERENCED_PARAMETER(pUnk);
    UNREFERENCED_PARAMETER(cSlots);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::COMClassicVTableDestroyed(ClassID wrappedClassId, REFGUID implementedIID, VOID* pUnk)
{
    UNREFERENCED_PARAMETER(wrappedClassId);
    UNREFERENCED_PARAMETER(implementedIID);
    UNREFERENCED_PARAMETER(pUnk);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::JITFunctionPitched(UINT functionId)
{
    UNREFERENCED_PARAMETER(functionId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::JITInlining(UINT callerId, UINT calleeId, BOOL* pfShouldInline)
{
    UNREFERENCED_PARAMETER(callerId);
    UNREFERENCED_PARAMETER(calleeId);
    UNREFERENCED_PARAMETER(pfShouldInline);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::JITCachedFunctionSearchStarted(UINT functionId, BOOL* pbUseCachedFunction)
{
    UNREFERENCED_PARAMETER(functionId);
    UNREFERENCED_PARAMETER(pbUseCachedFunction);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::JITCachedFunctionSearchFinished(UINT functionId, COR_PRF_JIT_CACHE result)
{
#ifdef _DEBUG
//////////////////////////////////////////////
if (IsDebuggerPresent())
    DebugBreak();
//////////////////////////////////////////////
    TCHAR sz[2*MAX_LENGTH];
    CFunctionInfoEx* pFunctionInfo=CFunctionInfoEx::Parse(this->pICorProfilerInfo,functionId,FALSE);
    if (!pFunctionInfo)
    {
        if (IsDebuggerPresent())
            DebugBreak();
        return S_OK;
    }
    // display func info
    _stprintf(sz,_T("JIT Cached Function Search Finished for function id: 0x%X, class id: 0x%X, module id: 0x%X, asm code start: 0x%X, asm code size: 0x%X\r\n"),functionId,pFunctionInfo->ClassId,pFunctionInfo->ModuleId,pFunctionInfo->AsmCodeStart,pFunctionInfo->AsmCodeSize);
    OutputDebugString(sz);

    pFunctionInfo->GetName(sz,MAX_LENGTH);
    OutputDebugString(sz);
    OutputDebugString(_T("\r\n"));
    delete pFunctionInfo;
#endif
    UNREFERENCED_PARAMETER(functionId);
    UNREFERENCED_PARAMETER(result);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::JITCompilationStarted(UINT functionId, BOOL fIsSafeToBlock)
{
#ifdef _DEBUG
    TCHAR sz[2*MAX_LENGTH];
    CFunctionInfoEx* pFunctionInfo=CFunctionInfoEx::Parse(this->pICorProfilerInfo,functionId,FALSE);
    if (!pFunctionInfo)
    {
        if (IsDebuggerPresent())
            DebugBreak();
        return S_OK;
    }
    // display func info
    _stprintf(sz,_T("JIT Compilation started for function id: 0x%X, class id: 0x%X, module id: 0x%X\r\n"),functionId,pFunctionInfo->ClassId,pFunctionInfo->ModuleId);
    OutputDebugString(sz);

    pFunctionInfo->GetName(sz,MAX_LENGTH);
    OutputDebugString(sz);
    OutputDebugString(_T("\r\n"));
    delete pFunctionInfo;
#endif

    UNREFERENCED_PARAMETER(functionId);
    UNREFERENCED_PARAMETER(fIsSafeToBlock);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::JITCompilationFinished(UINT functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    UNREFERENCED_PARAMETER(fIsSafeToBlock);
    // assume compilation was successful
    if (FAILED(hrStatus))
        return S_OK;

    CFunctionInfoEx* pFunctionInfo=CFunctionInfoEx::Parse(this->pICorProfilerInfo,functionId,TRUE);

#ifdef _DEBUG
    if (!pFunctionInfo)
    {
        if (IsDebuggerPresent())
            DebugBreak();
        return S_OK;
    }

    // display compiled informations

    TCHAR sz[2*MAX_LENGTH];
    TCHAR szTmp[10];
    ULONG cnt;
    // display func info
    _stprintf(sz,_T("JIT Compilation finished for function id: 0x%X, class id: 0x%X, module id: 0x%X ,start 0x%p, size: %u \r\n"),functionId,pFunctionInfo->ClassId,pFunctionInfo->ModuleId,pFunctionInfo->AsmCodeStart,pFunctionInfo->AsmCodeSize);
    OutputDebugString(sz);

    ///////////////////////////////////////////
    // display calling convention, return, func name and parameters
    ///////////////////////////////////////////

    // calling convention
    pFunctionInfo->GetStringCallingConvention(sz,MAX_LENGTH);
    OutputDebugString(sz);

    // return
    pFunctionInfo->pReturnInfo->GetName(sz,MAX_LENGTH);
    OutputDebugString(sz);

    // space between return and func name
    OutputDebugString(_T(" "));

    // func name
    pFunctionInfo->GetName(sz,MAX_LENGTH);
    OutputDebugString(sz);
    OutputDebugString(_T("\r\n"));

    // parameters
    CLinkListItem* pItem;
    CParameterInfo* pParamInfo;
    pFunctionInfo->pParameterInfoList->Lock();
    for (pItem=pFunctionInfo->pParameterInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        if (pItem!=pFunctionInfo->pParameterInfoList->Head)
            OutputDebugString(_T(","));

        pParamInfo=(CParameterInfo*)pItem->ItemData;
        pParamInfo->GetName(sz,MAX_LENGTH);
        OutputDebugString(sz);
    }
    pFunctionInfo->pParameterInfoList->Unlock();
    OutputDebugString(_T("\r\n"));

    // display func first bytes
    _tcscpy(sz,_T("Function First ASM Bytes: "));
    for (cnt=0;(cnt<pFunctionInfo->AsmCodeSize) && (cnt<10);cnt++)
    {
        _stprintf(szTmp,_T("0x%.2X "),pFunctionInfo->AsmCodeStart[cnt]);
        _tcscat(sz,szTmp);
    }
    _tcscat(sz,_T("\r\n"));
    OutputDebugString(sz);
#endif

    if (!pFunctionInfo)
        return S_OK;

    // report compiled function info
    CompiledFunctionCallBack(pFunctionInfo);
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNetProfiler::Initialize(IUnknown * pICorProfilerInfoUnk )
{
#ifdef _DEBUG
    OutputDebugString(_T("Cor Initialize\r\n"));
#endif
    // get the ICorProfilerInfo interface
    HRESULT hr = pICorProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo,(void**)&this->pICorProfilerInfo);
    if ( FAILED(hr) )
        return E_INVALIDARG;

    // set pCurrentCorProfiler (current profiler global var)
    // (must be set before calling CorInitialize())
    pCurrentCorProfiler=this->pICorProfilerInfo;

    // load options and initialize current dll for hooking mode
    if (!CorInitialize())
        return E_FAIL;

    // Indicate which events we're interested in.
    DWORD EventMask = 0;

    /*
    COR_PRF_MONITOR_NONE	= 0,
    COR_PRF_MONITOR_FUNCTION_UNLOADS	= 0x1,
    COR_PRF_MONITOR_CLASS_LOADS	= 0x2,
    COR_PRF_MONITOR_MODULE_LOADS	= 0x4,
    COR_PRF_MONITOR_ASSEMBLY_LOADS	= 0x8,
    COR_PRF_MONITOR_APPDOMAIN_LOADS	= 0x10,
    COR_PRF_MONITOR_JIT_COMPILATION	= 0x20,
    COR_PRF_MONITOR_EXCEPTIONS	= 0x40,
    COR_PRF_MONITOR_GC	= 0x80,
    COR_PRF_MONITOR_OBJECT_ALLOCATED	= 0x100,
    COR_PRF_MONITOR_THREADS	= 0x200,
    COR_PRF_MONITOR_REMOTING	= 0x400,
    COR_PRF_MONITOR_CODE_TRANSITIONS	= 0x800,
    COR_PRF_MONITOR_ENTERLEAVE	= 0x1000,
    COR_PRF_MONITOR_CCW	= 0x2000,
    COR_PRF_MONITOR_REMOTING_COOKIE	= 0x4000 | COR_PRF_MONITOR_REMOTING,
    COR_PRF_MONITOR_REMOTING_ASYNC	= 0x8000 | COR_PRF_MONITOR_REMOTING,
    COR_PRF_MONITOR_SUSPENDS	= 0x10000,
    COR_PRF_MONITOR_CACHE_SEARCHES	= 0x20000,
    COR_PRF_MONITOR_CLR_EXCEPTIONS	= 0x1000000,
    COR_PRF_MONITOR_ALL	= 0x107ffff,
    COR_PRF_ENABLE_REJIT	= 0x40000,
    COR_PRF_ENABLE_INPROC_DEBUGGING	= 0x80000,
    COR_PRF_ENABLE_JIT_MAPS	= 0x100000,
    COR_PRF_DISABLE_INLINING	= 0x200000,
    COR_PRF_DISABLE_OPTIMIZATIONS	= 0x400000,
    COR_PRF_ENABLE_OBJECT_ALLOCATED	= 0x800000,
    COR_PRF_ALL	= 0x1ffffff,
    COR_PRF_MONITOR_IMMUTABLE	= COR_PRF_MONITOR_CODE_TRANSITIONS | COR_PRF_MONITOR_REMOTING | COR_PRF_MONITOR_REMOTING_COOKIE | COR_PRF_MONITOR_REMOTING_ASYNC | COR_PRF_MONITOR_GC | COR_PRF_ENABLE_REJIT | COR_PRF_ENABLE_INPROC_DEBUGGING | COR_PRF_ENABLE_JIT_MAPS | COR_PRF_DISABLE_OPTIMIZATIONS | COR_PRF_DISABLE_INLINING | COR_PRF_ENABLE_OBJECT_ALLOCATED
    } 	COR_PRF_MONITOR;

    */

    // report compilation events
    EventMask |=COR_PRF_MONITOR_JIT_COMPILATION;

    // avoid inlining to catch all sub functions calls
    EventMask |=COR_PRF_DISABLE_INLINING;

    // according to options
    if (HookNetOptions.DisableOptimization)
    {
        // disable optimization to get better chance to install asm hooks
        // optimization reduce generated code size, so you can get code size less than (1+sizeof(PBYTE))
        // which is not enough to install our hook, disabling optimization gives use sufficient size even for empty function
        EventMask |=COR_PRF_DISABLE_OPTIMIZATIONS;
    }

    // Due to .NET exception handling way, we have to always monitor exceptions for our hooking algorithm
    // HookNetOptions.MonitorException will only act on report sending or not
    EventMask |=(COR_PRF_MONITOR_CLR_EXCEPTIONS | COR_PRF_MONITOR_EXCEPTIONS);


/*
before FrameWork 2.0
SetEnterLeaveFunctionHooks is adequate for profilers who simply cared whether a function was entered or exited. 
However, to find out which parameters the function was called with or what the return value was on exit,
the profiler was supposed to use the in-process debugging interface (ICorDebug). 
In-process debugging is not optimal for this purpose as there is no way for a profiler to detect these values.
*/
this->pICorProfilerInfo->SetEnterLeaveFunctionHooks ( (FunctionEnter *)&EnterNaked,
        (FunctionLeave *)&LeaveNaked,
        (FunctionTailcall *)&TailcallNaked );   

    if (HookNetOptions.EnableFrameworkMonitoring)
        EventMask |=COR_PRF_MONITOR_ENTERLEAVE;
//*/
    // why asm hook instead of IL code changes
    // asm hook: 
    // - if optimization is disable, all functions can be hooked
    // - if optimization is enable, only small function with code generated size less than (1+sizeof(PBYTE) can't be hook
    // IL hook :
    // for IMAGE_COR_ILMETHOD_TINY, you may can't install IL hook according to it's size (either optimization is disabled or not)
    //
    // so IL hook as no particular advantage, and asm hook code is already written for all other api --> we will use asm code hook

    // set the event mask
    this->pICorProfilerInfo->SetEventMask(EventMask);
    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNetProfiler::Shutdown()
{
#ifdef _DEBUG
    OutputDebugString(_T("Cor Shutdown\r\n"));
#endif
    CorShutdown();
    pCurrentCorProfiler=NULL;
    this->pICorProfilerInfo->Release();
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: GetObjectClassName
// Object: get class name from an ObjectID
// Parameters :
//     in  : ObjectID ObjectId : object id of which we want class name
//           DWORD ClassNameMaxSize : ClassName buffer max len in TCHAR
//     in/out : TCHAR* ClassName : object class name
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetProfiler::GetObjectClassName(ObjectID ObjectId,TCHAR* ClassName,DWORD ClassNameMaxSize)
{
    ClassID ClassId;
    ModuleID ModuleId;
    mdTypeDef TypeDefToken;
    HRESULT hResult;

    *ClassName=NULL;

    // get exception class id
    hResult=this->pICorProfilerInfo->GetClassFromObject(ObjectId,&ClassId);
    if (FAILED(hResult))
        return FALSE;

    // get exception class name
    // class name is like "System.Security.SecurityException"
    hResult=this->pICorProfilerInfo->GetClassIDInfo(ClassId,&ModuleId,&TypeDefToken);
    if (FAILED(hResult))
        return FALSE;

    IMetaDataImport* pMetaDataImport;
    // get IMetaDataImport interface
    hResult=this->pICorProfilerInfo->GetModuleMetaData(ModuleId,ofRead,IID_IMetaDataImport,(IUnknown **)&pMetaDataImport);
    if (FAILED(hResult) || (pMetaDataImport==NULL))
        return FALSE;


    WCHAR* wClassName;
    DWORD wClassNameSize;
    wClassName=(WCHAR*)_alloca(ClassNameMaxSize*sizeof(WCHAR));

    // if a class is specified
    if ( TypeDefToken != mdTypeDefNil )
    {
        // get class name
        hResult = pMetaDataImport->GetTypeDefProps(TypeDefToken,
                                                    wClassName,
                                                    ClassNameMaxSize,
                                                    &wClassNameSize,
                                                    NULL,
                                                    NULL); 
        wClassName[ClassNameMaxSize-1]=0;
        if (FAILED(hResult)
            || (wClassNameSize==0)
            )
        {
            wsprintfW(wClassName,L"ClassToken%u",TypeDefToken);
        }

#if (defined(UNICODE)||defined(_UNICODE))
        _tcsncpy(ClassName,wClassName,ClassNameMaxSize);
#else
        wcstombs(ClassName,wClassName,ClassNameMaxSize);
#endif
        ClassName[ClassNameMaxSize-1]=0;

    }
    pMetaDataImport->Release();

    return TRUE;
}



// ICorProfilerCallback2 interface implementation
HRESULT STDMETHODCALLTYPE CNetProfiler::ThreadNameChanged(
                         ThreadID threadId,
                         ULONG cchName,
                         WCHAR name[])
{
    UNREFERENCED_PARAMETER(threadId);
    UNREFERENCED_PARAMETER(cchName);
    UNREFERENCED_PARAMETER(name);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::GarbageCollectionStarted(
                                 int cGenerations,
                                 BOOL generationCollected[],
                                 COR_PRF_GC_REASON reason)
{
    UNREFERENCED_PARAMETER(cGenerations);
    UNREFERENCED_PARAMETER(generationCollected);
    UNREFERENCED_PARAMETER(reason);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::SurvivingReferences(
                            ULONG    cSurvivingObjectIDRanges,
                            ObjectID objectIDRangeStart[] ,
                            ULONG    cObjectIDRangeLength[] )
{
    UNREFERENCED_PARAMETER(cSurvivingObjectIDRanges);
    UNREFERENCED_PARAMETER(objectIDRangeStart);
    UNREFERENCED_PARAMETER(cObjectIDRangeLength);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::GarbageCollectionFinished()
{
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::FinalizeableObjectQueued(
                                 DWORD finalizerFlags,
                                 ObjectID objectID)
{
    UNREFERENCED_PARAMETER(finalizerFlags);
    UNREFERENCED_PARAMETER(objectID);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::RootReferences2(
                        ULONG    cRootRefs,
                        ObjectID rootRefIds[],
                        COR_PRF_GC_ROOT_KIND rootKinds[],
                        COR_PRF_GC_ROOT_FLAGS rootFlags[],
                        UINT_PTR rootIds[])
{
    UNREFERENCED_PARAMETER(cRootRefs);
    UNREFERENCED_PARAMETER(rootRefIds);
    UNREFERENCED_PARAMETER(rootKinds);
    UNREFERENCED_PARAMETER(rootFlags);
    UNREFERENCED_PARAMETER(rootIds);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::HandleCreated(
                      GCHandleID handleId,
                      ObjectID initialObjectId)
{
    UNREFERENCED_PARAMETER(handleId);
    UNREFERENCED_PARAMETER(initialObjectId);

    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::HandleDestroyed(
                        GCHandleID handleId)
{
    UNREFERENCED_PARAMETER(handleId);
    return E_NOTIMPL;
}



// ICorProfilerCallback3 interface implementation
HRESULT STDMETHODCALLTYPE CNetProfiler::InitializeForAttach( 
															IUnknown *pCorProfilerInfoUnk,
															void *pvClientData,
															UINT cbClientData)
{
	UNREFERENCED_PARAMETER(pCorProfilerInfoUnk);
	UNREFERENCED_PARAMETER(pvClientData);
	UNREFERENCED_PARAMETER(cbClientData);
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ProfilerAttachComplete( void)
{
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CNetProfiler::ProfilerDetachSucceeded( void)
{
	return E_NOTIMPL;
}