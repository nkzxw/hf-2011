/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2006  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/


/******************************************************************/
/* Build configuration                                            */
/******************************************************************/

#define TRACE_LEVEL	2 /* warning */


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "HookSys.h"

// project's headers
#include "DrvFilter.h"
#include "DrvInterface.h"
#include "FileInfo.h"
#include "HookCommon.h"
#include "Hooks.h"
#include "Link.h"
#include "Malloc.h"
#include "NtUndoc.h"
#include "ProcInfo.h"
#include "ProcList.h"
#include "Trace.h"
#include "WatchedObjects.h"


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define STATIC_UNICODE_STRING(symbol,value) \
  static UNICODE_STRING symbol = {sizeof(value)-sizeof(WCHAR),sizeof(value),value} ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

STATIC_UNICODE_STRING (g_usUnknownFile, L"!! Unknown file !!" ) ;


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS HookSys_ProcessCreated (HANDLE hProcess, LPCWSTR wszFilePath)
{          
  PROCSTRUCT	*pProc ; 
  PROCADDR	nProcessAddress ;
  PROCID	nProcessId ;
  BOOL		bNoNotification ;
  NTSTATUS	nStatus ;

  // get information on new process
  ProcInfo_GetAddress (hProcess, &nProcessAddress) ;
  ProcInfo_GetProcessId (hProcess, &nProcessId) ;
  
  // alloc a new process descriptor
  pProc = ProcList_New (nProcessAddress, nProcessId, wszFilePath) ;

  bNoNotification = pProc->nFlags & PROCESS_NO_NOTIFICATION ;

  // get associated filters
  nStatus = DrvFilter_LockMutex () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;
  DrvFilter_GetFiltersForProgram (pProc->wszPath, pProc->aFilters, 
				  &pProc->nFilters, MAX_FILTERS) ;
  DrvFilter_UnlockMutex () ;  
  
  // add process descriptor to process list
  nStatus = ProcList_Lock () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;
  ProcList_Add (pProc) ;
  ProcList_Unlock () ;
  
  // notify application
  if( ! bNoNotification )
    HookCommon_SendProcessCreatedNotification (nProcessAddress, nProcessId, wszFilePath) ;

  return STATUS_SUCCESS ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WINAPI Hook_NtCreateProcess (PHANDLE ProcessHandle,
				      ACCESS_MASK DesiredAccess, 
				      POBJECT_ATTRIBUTES ObjectAttributes,
				      HANDLE ParentProcess,
				      BOOLEAN InheritObjectTable,
				      HANDLE SectionHandle, 
				      HANDLE DebugPort,
				      HANDLE ExceptionPort)
{
  PROC		pfnStub = Hooks_GetStubAddress (HOOKS_NTCREATEPROCESS) ;
  NTSTATUS	nStatus ;
  UINT		nReaction, nOptions ;
  WCHAR		wszFilePath[MAX_PATH] ;
  LARGE_INTEGER liFileTime ;
  WOTSECTION	*pWotSectionData ;
  ULONG		nWotDataSize ;
  PVOID		pObjectSection = NULL ;
  

  //DbgPrint ("/ Enter \\ irql = %d\n", KeGetCurrentIrql()) ;  

  TRACE ;

  nStatus = ObReferenceObjectByHandle (SectionHandle, GENERIC_ALL, NULL, KernelMode, &pObjectSection, NULL) ;    
  if( nStatus!=STATUS_SUCCESS || pObjectSection==NULL )
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }
  
  nStatus = WatchObjs_Lock () ;
  if( nStatus != STATUS_SUCCESS )
    {
      ObDereferenceObject (pObjectSection) ;
      return nStatus ;
    }

  nStatus = WatchObjs_GetFromPointer (pObjectSection, WOT_SECTION, 
				     (void**)&pWotSectionData, &nWotDataSize) ;  
  if( nStatus!=STATUS_SUCCESS )
    {
      TRACE_WARNING (TEXT("SectionObject is not in watched object list\n")) ;
      
      wcscpy (wszFilePath, g_usUnknownFile.Buffer) ; 
      liFileTime.QuadPart = 0 ;
    }
  else
    {
      wcscpy (wszFilePath, pWotSectionData->wszFilePath) ;
      liFileTime = pWotSectionData->liFileTime ;
    }

  WatchObjs_Unlock () ;
  ObDereferenceObject (pObjectSection) ;

  TRACE_INFO (TEXT("File = %ls\n"), wszFilePath) ;

  HookCommon_CatchCall (&nReaction, &nOptions,
			FILTREASON_SYS_EXECUTE, 
			TEXT("s"), wszFilePath) ;

  if( (nOptions&RULE_SCAN)!=0 && nReaction==RULE_ACCEPT && HookCommon_ShouldScanFile(wszFilePath))
    {
      HookCommon_ScanFile (&nReaction, wszFilePath, &liFileTime) ;
    }

  if( nReaction == RULE_REJECT ) 
    {
      *ProcessHandle = INVALID_HANDLE_VALUE ;
      return STATUS_FILE_INVALID ;
    }

  if( nReaction == RULE_FEIGN ) 
    {
      *ProcessHandle = INVALID_HANDLE_VALUE ;
      return STATUS_SUCCESS ; 
      }

  //DbgPrint ("Calling NtCreateProcess... %ls\n", wszFilePath) ;
  
  nStatus = pfnStub (ProcessHandle, DesiredAccess, 
		     ObjectAttributes,ParentProcess,
		     InheritObjectTable,SectionHandle, 
		     DebugPort, ExceptionPort) ;

  //DbgPrint ("Calling NtCreateProcess... result = 0x%08X\n", nStatus); 
  
  if( SUCCEEDED (nStatus) )
    HookSys_ProcessCreated (*ProcessHandle, wszFilePath) ;

  //DbgPrint ("\\ Leave /\n") ;

  return nStatus ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WINAPI Hook_NtCreateProcessEx (PHANDLE ProcessHandle,
					ACCESS_MASK DesiredAccess, 
					POBJECT_ATTRIBUTES ObjectAttributes,
					HANDLE ParentProcess,
					BOOLEAN InheritObjectTable,
					HANDLE SectionHandle, 
					HANDLE DebugPort,
					HANDLE ExceptionPort,
					HANDLE Unknown)
{
  PROC		pfnStub = Hooks_GetStubAddress (HOOKS_NTCREATEPROCESSEX) ;
  NTSTATUS	nStatus ;
  UINT		nReaction, nOptions ;
  WCHAR		wszFilePath[MAX_PATH] ;
  LARGE_INTEGER liFileTime ;
  WOTSECTION	*pWotSectionData ;
  ULONG		nWotDataSize ;
  PVOID		pObjectSection = NULL ;

  //DbgPrint ("/ Enter \\ irql = %d\n", KeGetCurrentIrql()) ;  

  TRACE ;

  nStatus = ObReferenceObjectByHandle (SectionHandle, GENERIC_ALL, NULL, KernelMode, &pObjectSection, NULL) ;    
  if( nStatus!=STATUS_SUCCESS || pObjectSection==NULL )
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }
  
  nStatus = WatchObjs_Lock () ;
  if( nStatus != STATUS_SUCCESS ) 
    {
      ObDereferenceObject (pObjectSection) ;
      return nStatus ;
    }

  nStatus = WatchObjs_GetFromPointer (pObjectSection, WOT_SECTION, 
				     (void**)&pWotSectionData, &nWotDataSize) ;  
  if( nStatus!=STATUS_SUCCESS )
    {
      TRACE_WARNING (TEXT("SectionObject is not in watched object list\n")) ;
      
      wcscpy (wszFilePath, g_usUnknownFile.Buffer) ; 
      liFileTime.QuadPart = 0 ;
    }
  else
    {
      wcscpy (wszFilePath, pWotSectionData->wszFilePath) ;
      liFileTime = pWotSectionData->liFileTime ;
    }

  WatchObjs_Unlock () ;
  ObDereferenceObject (pObjectSection) ;

  TRACE_INFO (TEXT("File = %ls\n"), wszFilePath) ;

  HookCommon_CatchCall (&nReaction, &nOptions,
			FILTREASON_SYS_EXECUTE, 
			TEXT("s"), wszFilePath) ;

  if( (nOptions&RULE_SCAN)!=0 && nReaction==RULE_ACCEPT && HookCommon_ShouldScanFile(wszFilePath) )
    {
      HookCommon_ScanFile (&nReaction, wszFilePath, &liFileTime) ;
    }

  if( nReaction == RULE_REJECT ) 
    {
      *ProcessHandle = INVALID_HANDLE_VALUE ;
      return STATUS_FILE_INVALID ;
    }

  if( nReaction == RULE_FEIGN ) 
    {
      *ProcessHandle = INVALID_HANDLE_VALUE ;
      return STATUS_SUCCESS ; 
    }

  //DbgPrint ("Calling CreateProcessEx... %ls\n", wszFilePath) ;
  
  nStatus = pfnStub (ProcessHandle, DesiredAccess, 
		     ObjectAttributes,ParentProcess,
		     InheritObjectTable,SectionHandle, 
		     DebugPort, ExceptionPort, Unknown) ;

  //DbgPrint ("Calling CreateProcessEx... result = 0x%08X\n", nStatus); 
  
  if( SUCCEEDED (nStatus) )
    HookSys_ProcessCreated (*ProcessHandle, wszFilePath) ;

  //DbgPrint ("\\ Leave /\n") ;

  return nStatus ;
}



/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKAPI Hook_PspTerminateProcess (IN PEPROCESS	Eprocess, 
					  IN NTSTATUS	ExitStatus) 
{
  PROC		pfnStub = Hooks_GetStubAddress (HOOKS_PSPTERMINATEPROCESS) ;
  NTSTATUS	nStatus ;

  TRACE_ALWAYS (TEXT("Eprocess = 0x%08X\n"), Eprocess) ;

  nStatus = (NTSTATUS) pfnStub (Eprocess, ExitStatus) ;

  if( SUCCEEDED(nStatus) )
    {
      PROCSTRUCT *pProc ;

      if( nStatus!=STATUS_SUCCESS )
	TRACE_WARNING(TEXT("PspTerminateProcess returned 0x%08X\n"), nStatus) ;

      nStatus = ProcList_Lock () ;
      if( nStatus != STATUS_SUCCESS ) return nStatus ;

      pProc = ProcList_Remove ((PROCADDR)Eprocess) ;	
      ProcList_Unlock () ;
      
      if( pProc==NULL || (pProc->nFlags&PROCESS_NO_NOTIFICATION)==0 )
	HookCommon_SendProcessTerminatedNotification ((PROCADDR)Eprocess) ;     	

      ProcList_Delete (pProc) ;
    }  
 
  return nStatus ;  
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKAPI Hook_NtTerminateProcess (IN HANDLE ProcessHandle OPTIONAL, 
					 IN NTSTATUS ExitStatus) 
{
  PROC		pfnStub = Hooks_GetStubAddress (HOOKS_NTTERMINATEPROCESS) ;
  NTSTATUS	nStatus ;

  TRACE_INFO (TEXT("ProcessHandle=0x%08X (currentprocess=0x%08X)\n"), ProcessHandle, ProcInfo_GetCurrentProcessAddress()) ;


  if( ProcessHandle!=INVALID_HANDLE_VALUE && ProcessHandle!=NULL )
    {
      PROCSTRUCT	*pProc ;
      PROCADDR		nProcessAddress ;
      UINT		nReaction = RULE_ACCEPT ;
      WCHAR		wszFilePath[MAX_PATH] ;

      ProcInfo_GetAddress (ProcessHandle, &nProcessAddress) ;
      
      TRACE_ALWAYS (TEXT("Process 0x%08X is killing process 0x%08X\n"), 
		    ProcInfo_GetCurrentProcessAddress(), nProcessAddress) ;
      
      wcscpy (wszFilePath, g_usUnknownFile.Buffer) ; 
      
      nStatus = ProcList_Lock () ;
      
      if( nStatus == STATUS_SUCCESS )
	{
	  pProc = ProcList_Get (nProcessAddress) ;
	  
	  if( pProc != NULL ) wcscpy (wszFilePath, pProc->wszPath) ;			
	  
	  ProcList_Unlock () ;
	}
      
      HookCommon_CatchCall (&nReaction, NULL,
			    FILTREASON_SYS_KILLPROCESS, 
			    TEXT("s"), wszFilePath) ;
	  
      if( nReaction == RULE_REJECT ) return STATUS_ACCESS_DENIED ;
      if( nReaction == RULE_FEIGN ) return STATUS_SUCCESS ;
    }

  // if NtTerminateProcess is called with ProcessHandle==0xFFFFFFFF, it will not return
  // in this case, we call it later
  if( ProcessHandle!=INVALID_HANDLE_VALUE )
    nStatus = (NTSTATUS) pfnStub (ProcessHandle, ExitStatus) ;
  else
    nStatus = STATUS_SUCCESS ;

  // we ignore the call if it ProcessHandle==NULL because NtTerminateProcess will be called
  // a second time with a valie ProcessHandle or with 0xFFFFFFFF
  // (this has been observed on Windows XP SP2 and Windows 2000 SP4)
  if( SUCCEEDED(nStatus) && ProcessHandle!=NULL )
    {
      PROCSTRUCT	*pProc ;
      PROCADDR		nProcessAddress ;

      if( nStatus!=STATUS_SUCCESS )
	TRACE_WARNING(TEXT("NtTerminateProcess returned 0x%08X\n"), nStatus) ;

      if( ProcessHandle!=NULL && ProcessHandle!=INVALID_HANDLE_VALUE )
	ProcInfo_GetAddress (ProcessHandle, &nProcessAddress) ;
      else
	nProcessAddress = ProcInfo_GetCurrentProcessAddress() ;

      nStatus = ProcList_Lock () ;
      if( nStatus != STATUS_SUCCESS ) return nStatus ;
      pProc = ProcList_Remove (nProcessAddress) ;	
      ProcList_Unlock () ;

      if( pProc==NULL )
	TRACE_WARNING (TEXT("Unknown process (handle=0x%08X, address=0x%08)\n"), ProcessHandle, nProcessAddress) ;
      
      if( pProc!=NULL && (pProc->nFlags&PROCESS_NO_NOTIFICATION)==0 )
	HookCommon_SendProcessTerminatedNotification (nProcessAddress) ;     	

      ProcList_Delete (pProc) ;
    }  

  // read comment above
  if( ProcessHandle==INVALID_HANDLE_VALUE )
    nStatus = (NTSTATUS) pfnStub (ProcessHandle, ExitStatus) ;
 
  return nStatus ;  
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKAPI Hook_NtCreateSection (PHANDLE  SectionHandle,
				      ACCESS_MASK  DesiredAccess,
				      POBJECT_ATTRIBUTES  ObjectAttributes,
				      PLARGE_INTEGER  MaximumSize,
				      ULONG  SectionPageProtection,
				      ULONG  AllocationAttributes,
				      HANDLE  FileHandle)
{
  PROC		pfnStub = Hooks_GetStubAddress (HOOKS_NTCREATESECTION) ;
  NTSTATUS	nStatus ;

  TRACE ;

  //if( FileHandle==NULL )
  //  TRACE_WARNING (TEXT("FileHandle==NULL\n")) ;

  nStatus = pfnStub (SectionHandle, DesiredAccess,
		     ObjectAttributes, MaximumSize,
		     SectionPageProtection,
		     AllocationAttributes,
		     FileHandle) ;

  if( nStatus==STATUS_SUCCESS && FileHandle )
    {       
      WOTFILE		*pWotFileData ;
      WOTSECTION	*pWotSectionData ;
      ULONG		nWotDataSize ;
      PVOID		pObjectFile = NULL ;
      PVOID		pObjectSection = NULL ;

      nStatus = ObReferenceObjectByHandle (FileHandle, GENERIC_ALL, NULL, KernelMode, &pObjectFile, NULL) ;
      if( nStatus!=STATUS_SUCCESS || pObjectFile==NULL )
	{
	  TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
	  ZwClose (*SectionHandle) ;
	  return nStatus ;
	}

      nStatus = WatchObjs_Lock () ;
      if( nStatus != STATUS_SUCCESS ) 
	{
	  ObDereferenceObject (pObjectFile) ;
	  ZwClose (*SectionHandle) ;
	  return nStatus ;
	}
      
      nStatus = WatchObjs_GetFromPointer (pObjectFile, WOT_FILE, (void**)&pWotFileData, &nWotDataSize) ;
      
      if( nStatus==STATUS_SUCCESS )
	{
	  pWotSectionData = MALLOC (nWotDataSize) ;

	  if( pWotSectionData == NULL )
	    {
	      TRACE_ERROR (TEXT("Failed to allocate structure WOTSECTION (%u bytes)\n"), nWotDataSize) ;
	      WatchObjs_Unlock () ;
	      ObDereferenceObject (pObjectFile) ;
	      ZwClose (*SectionHandle) ;
	      return STATUS_INSUFFICIENT_RESOURCES ;
	    }
	  
	  memcpy (pWotSectionData, pWotFileData, nWotDataSize) ;

	  WatchObjs_Unlock () ;
	  ObDereferenceObject (pObjectFile) ;
	}
      else
	{
	  UNICODE_STRING	usFileName ;

	  WatchObjs_Unlock () ;
	  ObDereferenceObject (pObjectFile) ;
	  
	  nWotDataSize = sizeof(WOTSECTION) + MAX_PATH*sizeof(WCHAR) ;
	  pWotSectionData = MALLOC (nWotDataSize) ;

	  if( pWotSectionData == NULL )
	    {
	      TRACE_ERROR (TEXT("Failed to allocate structure WOTSECTION (%u bytes)\n"), nWotDataSize) ;
	      ZwClose (*SectionHandle) ;
	      return STATUS_INSUFFICIENT_RESOURCES ;
	    }

	  usFileName.Length = 0 ;
	  usFileName.MaximumLength = MAX_PATH *sizeof(WCHAR) ;	
	  usFileName.Buffer = MALLOC(usFileName.MaximumLength) ;

	  if( usFileName.Buffer == NULL )
	    {
	      TRACE_ERROR (TEXT("Failed to allocate buffer for filename (%u bytes)\n"), usFileName.MaximumLength) ;
	      FREE(pWotSectionData) ;
	      ZwClose (*SectionHandle) ;
	      return STATUS_INSUFFICIENT_RESOURCES ;
	    }	  
	  
	  nStatus = FileInfo_GetPath (FileHandle, &usFileName) ;
	  
	  if( nStatus!=STATUS_SUCCESS )
	    {
	      TRACE_ERROR (TEXT("FileInfo_GetDosPath failed (status=0x%08X)\n"), nStatus) ;
	      RtlCopyUnicodeString (&usFileName, &g_usUnknownFile) ;	  
	    }        
	}     

      TRACE_INFO (TEXT("File = %ls\n"), pWotSectionData->wszFilePath) ; 
      
      ASSERT (*SectionHandle!=NULL) ;

      nStatus = ObReferenceObjectByHandle (*SectionHandle, GENERIC_ALL, NULL, KernelMode, &pObjectSection, NULL) ;
      if( nStatus!=STATUS_SUCCESS || pObjectSection==NULL )
	{
	  TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
	  ZwClose (*SectionHandle) ;
	  return nStatus ;
	}

      nStatus = WatchObjs_Lock () ;
      if( nStatus != STATUS_SUCCESS ) 
	{
	  ZwClose (*SectionHandle) ;
	  ObDereferenceObject (pObjectSection) ;
	  return nStatus ;
	}
      
      nStatus = WatchObjs_AddFromPointer (pObjectSection,
					  WOT_SECTION,
					  pWotSectionData,
					  nWotDataSize) ;   
      WatchObjs_Unlock () ;  
      ObDereferenceObject (pObjectSection) ;    

      // restore original status
      nStatus = STATUS_SUCCESS ;
    }

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKFASTAPI Hook_ObpFreeObject_Win2K (PVOID  Object)
{
  NTSTATUS DDKFASTAPI (*pfnStub)(PVOID) ;
  ULONG			nPointerCount ;
  NTSTATUS		nStatus ;
 
  pfnStub = (void*)Hooks_GetStubAddress (HOOKS_OBPFREEOBJECT) ; 
  nPointerCount = *(ULONG*)((BYTE*)Object-0x18) ;
 
  if( nPointerCount>=1 )
    TRACE_WARNING (TEXT("ObpFreeObject but there are %d pointers\n"), nPointerCount) ;

  nStatus = WatchObjs_Lock () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;

  WatchObjs_RemFromPointer (Object) ;
  WatchObjs_Unlock () ;
  
  return pfnStub (Object) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS PARAM_IN_EAX Hook_ObpFreeObject_Vista (PVOID  Object) 
{
  NTSTATUS PARAM_IN_EAX (*pfnStub)() ;
  ULONG			nPointerCount ;
  NTSTATUS		nStatus ;

  pfnStub = (void*)Hooks_GetStubAddress (HOOKS_OBPFREEOBJECT) ; 
  nPointerCount = *(ULONG*)((BYTE*)Object-0x18) ;
 
  if( nPointerCount>=1 )
    TRACE_WARNING (TEXT("ObpFreeObject but there are %d pointers\n"), nPointerCount) ;

  nStatus = WatchObjs_Lock () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;

  WatchObjs_RemFromPointer (Object) ;
  WatchObjs_Unlock () ;
  
  return pfnStub (Object) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS Hook_NtCreateUserProcess (PHANDLE ProcessHandle, PVOID arg2, PVOID arg3, PVOID arg4, 
				   PVOID arg5, PVOID arg6, PVOID arg7, PVOID arg8, 
				   PVOID arg9, PVOID arg10, PVOID arg11, PVOID arg12) 
{
  NTSTATUS	nStatus ;
  PROC		pfnStub = Hooks_GetStubAddress (HOOKS_NTCREATEUSERPROCESS) ;
  LPCWSTR	wszFilePath ;
  UINT		nReaction = RULE_ACCEPT ;
  UINT		nOptions = 0 ; 

  TRACE_INFO (TEXT("0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\n"), arg2, arg3, arg4, arg5, arg6, arg7) ;

  wszFilePath = *(LPWSTR*) ((BYTE*)arg2 + 20) ;

  if( wszFilePath == NULL )
      wszFilePath = *(LPWSTR*) ((BYTE*)arg2 + 8) ;

  TRACE_INFO (TEXT("File = %ls\n"), wszFilePath) ;


  HookCommon_CatchCall (&nReaction, &nOptions,
			FILTREASON_SYS_EXECUTE, 
			TEXT("s"), wszFilePath) ;

  if( (nOptions&RULE_SCAN)!=0 && nReaction==RULE_ACCEPT && HookCommon_ShouldScanFile(wszFilePath) )
    {
      LARGE_INTEGER liFileTime = {{0}} ;
      HookCommon_ScanFile (&nReaction, wszFilePath, &liFileTime) ;
    }

  if( nReaction == RULE_REJECT ) 
    {
      *ProcessHandle = INVALID_HANDLE_VALUE ;
      return STATUS_FILE_INVALID ;
    }

  if( nReaction == RULE_FEIGN ) 
    {
      *ProcessHandle = INVALID_HANDLE_VALUE ;
      return STATUS_SUCCESS ; 
    }

  nStatus = pfnStub (ProcessHandle, arg2, arg3, arg4, 
		     arg5, arg6, arg7, arg8, 
		     arg9, arg10, arg11, arg12) ;

  if( SUCCEEDED (nStatus) )
    HookSys_ProcessCreated (*ProcessHandle, wszFilePath) ;

  return nStatus ;
}
