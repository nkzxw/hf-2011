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
#pragma once

#include "HookNetExport.h"
#include "HookNet.h"
#include "FunctionInfoEx.h"
#include <Windows.h>
#include <cor.h>
#include <corprof.h>
#include <tchar.h>
#include <stdio.h>

// required lib : corguids.lib
#pragma comment (lib,"corguids")

#ifndef __ICorProfilerCallback2_INTERFACE_DEFINED__

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_corprof_0011_0001
{	COR_PRF_GC_ROOT_STACK	= 1,
COR_PRF_GC_ROOT_FINALIZER	= 2,
COR_PRF_GC_ROOT_HANDLE	= 3,
COR_PRF_GC_ROOT_OTHER	= 0
} 	COR_PRF_GC_ROOT_KIND;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_corprof_0011_0002
{	COR_PRF_GC_ROOT_PINNING	= 0x1,
COR_PRF_GC_ROOT_WEAKREF	= 0x2,
COR_PRF_GC_ROOT_INTERIOR	= 0x4,
COR_PRF_GC_ROOT_REFCOUNTED	= 0x8
} 	COR_PRF_GC_ROOT_FLAGS;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_corprof_0011_0006
{	COR_PRF_GC_INDUCED	= 1,
COR_PRF_GC_OTHER	= 0
} 	COR_PRF_GC_REASON;
typedef UINT_PTR GCHandleID;

interface ICorProfilerCallback2 : public ICorProfilerCallback
{
public:
	virtual HRESULT STDMETHODCALLTYPE ThreadNameChanged( 
		/* [in] */ ThreadID threadId,
		/* [in] */ ULONG cchName,
		/* [in] */ 
		/* __in_ecount_opt(cchName)  */ WCHAR name[  ]) = 0;

	virtual HRESULT STDMETHODCALLTYPE GarbageCollectionStarted( 
		/* [in] */ int cGenerations,
		/* [size_is][in] */ BOOL generationCollected[  ],
		/* [in] */ COR_PRF_GC_REASON reason) = 0;

	virtual HRESULT STDMETHODCALLTYPE SurvivingReferences( 
		/* [in] */ ULONG cSurvivingObjectIDRanges,
		/* [size_is][in] */ ObjectID objectIDRangeStart[  ],
		/* [size_is][in] */ ULONG cObjectIDRangeLength[  ]) = 0;

	virtual HRESULT STDMETHODCALLTYPE GarbageCollectionFinished( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE FinalizeableObjectQueued( 
		/* [in] */ DWORD finalizerFlags,
		/* [in] */ ObjectID objectID) = 0;

	virtual HRESULT STDMETHODCALLTYPE RootReferences2( 
		/* [in] */ ULONG cRootRefs,
		/* [size_is][in] */ ObjectID rootRefIds[  ],
		/* [size_is][in] */ COR_PRF_GC_ROOT_KIND rootKinds[  ],
		/* [size_is][in] */ COR_PRF_GC_ROOT_FLAGS rootFlags[  ],
		/* [size_is][in] */ UINT_PTR rootIds[  ]) = 0;

	virtual HRESULT STDMETHODCALLTYPE HandleCreated( 
		/* [in] */ GCHandleID handleId,
		/* [in] */ ObjectID initialObjectId) = 0;

	virtual HRESULT STDMETHODCALLTYPE HandleDestroyed( 
		/* [in] */ GCHandleID handleId) = 0;
};
#endif


#ifndef __ICorProfilerCallback3_INTERFACE_DEFINED__

interface ICorProfilerCallback3 : public ICorProfilerCallback2
{
public:
	virtual HRESULT STDMETHODCALLTYPE InitializeForAttach( 
		/* [in] */ IUnknown *pCorProfilerInfoUnk,
		/* [in] */ void *pvClientData,
		/* [in] */ UINT cbClientData) = 0;

	virtual HRESULT STDMETHODCALLTYPE ProfilerAttachComplete( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE ProfilerDetachSucceeded( void) = 0;

};

#endif


class CNetProfiler : public ICorProfilerCallback3
{
public:
	CNetProfiler();
	~CNetProfiler();
// IUnknown interface implementation
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppInterface);
// End of IUnknown interface implementation

// ICorProfilerCallback interface implementation
    STDMETHOD(Initialize)(IUnknown * pICorProfilerInfoUnk);
    STDMETHOD(Shutdown)();
    STDMETHOD(AppDomainCreationStarted)(UINT appDomainId);
    STDMETHOD(AppDomainCreationFinished)(UINT appDomainId, HRESULT hrStatus);
    STDMETHOD(AppDomainShutdownStarted)(UINT appDomainId);
    STDMETHOD(AppDomainShutdownFinished)(UINT appDomainId, HRESULT hrStatus);
    STDMETHOD(AssemblyLoadStarted)(UINT assemblyId);
    STDMETHOD(AssemblyLoadFinished)(UINT assemblyId, HRESULT hrStatus);
    STDMETHOD(AssemblyUnloadStarted)(UINT assemblyId);
    STDMETHOD(AssemblyUnloadFinished)(UINT assemblyId, HRESULT hrStatus);
    STDMETHOD(ModuleLoadStarted)(UINT moduleId);
    STDMETHOD(ModuleLoadFinished)(UINT moduleId, HRESULT hrStatus);
    STDMETHOD(ModuleUnloadStarted)(UINT moduleId);
    STDMETHOD(ModuleUnloadFinished)(UINT moduleId, HRESULT hrStatus);
    STDMETHOD(ModuleAttachedToAssembly)(UINT moduleId, UINT assemblyId);
    STDMETHOD(ClassLoadStarted)(UINT classId);
    STDMETHOD(ClassLoadFinished)(UINT classId, HRESULT hrStatus);
    STDMETHOD(ClassUnloadStarted)(UINT classId);
    STDMETHOD(ClassUnloadFinished)(UINT classId, HRESULT hrStatus);
    STDMETHOD(FunctionUnloadStarted)(UINT functionId);
    STDMETHOD(JITCompilationStarted)(UINT functionId, BOOL fIsSafeToBlock);
    STDMETHOD(JITCompilationFinished)(UINT functionId, HRESULT hrStatus,BOOL fIsSafeToBlock);
    STDMETHOD(JITCachedFunctionSearchStarted)(UINT functionId,BOOL * pbUseCachedFunction);
    STDMETHOD(JITCachedFunctionSearchFinished)(UINT functionId,COR_PRF_JIT_CACHE result);
    STDMETHOD(JITFunctionPitched)(UINT functionId);
    STDMETHOD(JITInlining)(UINT callerId, UINT calleeId,BOOL * pfShouldInline);
    STDMETHOD(ThreadCreated)(UINT threadId);
    STDMETHOD(ThreadDestroyed)(UINT threadId);
    STDMETHOD(ThreadAssignedToOSThread)(UINT managedThreadId,ULONG osThreadId);
    STDMETHOD(RemotingClientInvocationStarted)();
    STDMETHOD(RemotingClientSendingMessage)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(RemotingClientReceivingReply)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(RemotingClientInvocationFinished)();
    STDMETHOD(RemotingServerReceivingMessage)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(RemotingServerInvocationStarted)();
    STDMETHOD(RemotingServerInvocationReturned)();
    STDMETHOD(RemotingServerSendingReply)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(UnmanagedToManagedTransition)(UINT functionId,COR_PRF_TRANSITION_REASON reason);
    STDMETHOD(ManagedToUnmanagedTransition)(UINT functionId,COR_PRF_TRANSITION_REASON reason);
    STDMETHOD(RuntimeSuspendStarted)(COR_PRF_SUSPEND_REASON suspendReason);
    STDMETHOD(RuntimeSuspendFinished)();
    STDMETHOD(RuntimeSuspendAborted)();
    STDMETHOD(RuntimeResumeStarted)();
    STDMETHOD(RuntimeResumeFinished)();
    STDMETHOD(RuntimeThreadSuspended)(UINT threadId);
    STDMETHOD(RuntimeThreadResumed)(UINT threadId);
    STDMETHOD(MovedReferences)(ULONG cMovedObjectIDRanges,UINT oldObjectIDRangeStart[],UINT newObjectIDRangeStart[],ULONG cObjectIDRangeLength[]);
    STDMETHOD(ObjectAllocated)(UINT objectId, UINT classId);
    STDMETHOD(ObjectsAllocatedByClass)(ULONG cClassCount, UINT classIds[],ULONG cObjects[]);
    STDMETHOD(ObjectReferences)(UINT objectId, UINT classId,ULONG cObjectRefs, UINT objectRefIds[]);
    STDMETHOD(RootReferences)(ULONG cRootRefs, UINT rootRefIds[]);
    STDMETHOD(ExceptionThrown)(UINT thrownObjectId);
    STDMETHOD(ExceptionSearchFunctionEnter)(UINT functionId);
    STDMETHOD(ExceptionSearchFunctionLeave)();
    STDMETHOD(ExceptionSearchFilterEnter)(UINT functionId);
    STDMETHOD(ExceptionSearchFilterLeave)();
    STDMETHOD(ExceptionSearchCatcherFound)(UINT functionId);
    STDMETHOD(ExceptionOSHandlerEnter)(UINT functionId);
    STDMETHOD(ExceptionOSHandlerLeave)(UINT functionId);
    STDMETHOD(ExceptionUnwindFunctionEnter)(UINT functionId);
    STDMETHOD(ExceptionUnwindFunctionLeave)();
    STDMETHOD(ExceptionUnwindFinallyEnter)(UINT functionId);
    STDMETHOD(ExceptionUnwindFinallyLeave)();
    STDMETHOD(ExceptionCatcherEnter)(UINT functionId, UINT objectId);
    STDMETHOD(ExceptionCatcherLeave)();
    STDMETHOD(COMClassicVTableCreated)(ClassID wrappedClassId,REFGUID implementedIID, void *pVTable, ULONG cSlots);        
    STDMETHOD(COMClassicVTableDestroyed)(ClassID wrappedClassId,REFGUID implementedIID, void *pVTable);
    STDMETHOD(ExceptionCLRCatcherFound)(void);        
    STDMETHOD(ExceptionCLRCatcherExecute)(void);
// End of ICorProfilerCallback interface implementation

// ICorProfilerCallback2 interface implementation
    STDMETHOD(ThreadNameChanged)(
        ThreadID threadId,
        ULONG cchName,
        WCHAR name[]);
    STDMETHOD(GarbageCollectionStarted)(
        int cGenerations,
        BOOL generationCollected[],
        COR_PRF_GC_REASON reason);
    STDMETHOD(SurvivingReferences)(
        ULONG    cSurvivingObjectIDRanges,
        ObjectID objectIDRangeStart[] ,
        ULONG    cObjectIDRangeLength[] );
    STDMETHOD(GarbageCollectionFinished)();
    STDMETHOD(FinalizeableObjectQueued)(
        DWORD finalizerFlags,
        ObjectID objectID);
    STDMETHOD(RootReferences2)(
        ULONG    cRootRefs,
        ObjectID rootRefIds[],
        COR_PRF_GC_ROOT_KIND rootKinds[],
        COR_PRF_GC_ROOT_FLAGS rootFlags[],
        UINT_PTR rootIds[]);
    STDMETHOD(HandleCreated)(
        GCHandleID handleId,
        ObjectID initialObjectId);
    STDMETHOD(HandleDestroyed)(
        GCHandleID handleId);
// End of ICorProfilerCallback2 interface implementation

// ICorProfilerCallback3 interface implementation
	virtual HRESULT STDMETHODCALLTYPE InitializeForAttach( 
		IUnknown *pCorProfilerInfoUnk,
		void *pvClientData,
		UINT cbClientData);
	virtual HRESULT STDMETHODCALLTYPE ProfilerAttachComplete( void);
	virtual HRESULT STDMETHODCALLTYPE ProfilerDetachSucceeded( void);
// End of ICorProfilerCallback3 interface implementation
private:
    LONG RefCount; // reference count
    ICorProfilerInfo* pICorProfilerInfo;
    BOOL GetObjectClassName(ObjectID ObjectId,TCHAR* ClassName,DWORD ClassNameMaxSize);
};