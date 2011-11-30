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

#define	TRACE_LEVEL	2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "ProcInfo.h"

// project's headers
#include "Trace.h"
#include "NtUndoc.h"



/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS _ProcInfo_GenericGetString (HANDLE		hProcess,
				     PUNICODE_STRING	pusString,				     
				     int		*pOffsets,
				     int		nOffsets)
{
  NTSTATUS			nStatus ;
  PROCESS_BASIC_INFORMATION	pbi ;
  int				iOffset ;

  // pointers in target process space
  INT_PTR		t_pCurrentBlock ;
  UNICODE_STRING	t_usString ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (pusString!=NULL) ;
  ASSERT (pusString->Buffer!=NULL) ;

  nStatus = ZwQueryInformationProcess (hProcess,
				       ProcessBasicInformation,
				       &pbi, sizeof(pbi), NULL) ;
  
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("NtQueryInformationProcess failed (handle=0x%08X, status=0x%08X)\n"), hProcess, nStatus) ;
      return nStatus ;
    }

  TRACE_INFO (TEXT("PEB = 0x%08X\n"), pbi.PebBaseAddress);

  if( pbi.PebBaseAddress==NULL )
    {
      TRACE_INFO (TEXT("PEB of process 0x%08X is NULL\n"), hProcess) ;
      return STATUS_UNSUCCESSFUL ;
    }

  for( iOffset=0 ; iOffset<nOffsets-1 ; iOffset++ )
    {
      t_pCurrentBlock = (INT_PTR)pbi.PebBaseAddress ;

      TRACE_INFO (TEXT("Indirection %d : 0x%08X\n"), iOffset, t_pCurrentBlock+pOffsets[iOffset]);
      
      nStatus = ntundoc.ZwReadVirtualMemory (hProcess, 
					     (LPVOID)(t_pCurrentBlock+pOffsets[iOffset]),
					     &t_pCurrentBlock, sizeof(t_pCurrentBlock), NULL) ;
      
      if( nStatus != STATUS_SUCCESS )
	{
	  TRACE_ERROR (TEXT("ZwReadVirtualMemory failed (status=0x%08X)\n"), nStatus) ;
	  return nStatus ;
	}

      if( ! t_pCurrentBlock )
	{
	  TRACE_INFO (TEXT("Indirection %d (0x%02X bytes) leads to a NULL pointer\n"), 
		       iOffset, pOffsets[iOffset]) ;
	  return STATUS_UNSUCCESSFUL ;
	}
    }

  TRACE_INFO (TEXT("Reading UNICODE_STRING at 0x%08X\n"), t_pCurrentBlock+pOffsets[iOffset]);

  nStatus = ntundoc.ZwReadVirtualMemory (hProcess, 
					 (LPVOID)(t_pCurrentBlock+pOffsets[iOffset]),
					 &t_usString, sizeof(t_usString), NULL) ;

  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("NtReadVirtualMemory failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }  

  pusString->Length = min (pusString->MaximumLength, t_usString.Length) ;

  nStatus = ntundoc.ZwReadVirtualMemory (hProcess, t_usString.Buffer,
					 pusString->Buffer, pusString->Length, NULL) ;

  if( nStatus!=STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("ZwReadVirtualMemory failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  pusString->Buffer[pusString->Length/2] = 0 ;
  
  TRACE_INFO (TEXT("String = %ls\n"), pusString->Buffer) ;
  
  return nStatus ;
}

/*
NTSTATUS _ProcInfo_GenericGetString (HANDLE		hProcess,
				     PUNICODE_STRING	pusString,
				     INT_PTR		nOffset1,
				     INT_PTR		nOffset2)
{
  NTSTATUS		nStatus ;
  PROCESS_BASIC_INFORMATION pbi ;

  // pointer in target process space
  INT_PTR		pProcParam ;
  UNICODE_STRING	usImagePath ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (pusString!=NULL) ;

  nStatus = NtQueryInformationProcess (hProcess,
				       ProcessBasicInformation,
				       &pbi, sizeof(pbi), NULL) ;
  
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("NtQueryInformationProcess failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  //TRACE_INFO (TEXT("PEB = 0x%08X\n"), pbi.PebBaseAddress);

  if( pbi.PebBaseAddress==NULL )
    {
      TRACE_ERROR (TEXT("PEB of process 0x%08X is NULL\n"), hProcess) ;
      return STATUS_UNSUCCESSFUL ;
    }

  //TRACE_INFO (TEXT("Reading pProcParam in PEB\n")) ;

  NtReadVirtualMemory (hProcess, (LPVOID)((INT_PTR)pbi.PebBaseAddress+nOffset1),
		       &pProcParam, sizeof(pProcParam), NULL) ;

  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("NtReadVirtualMemory failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  //TRACE_INFO (TEXT("Reading usImagePath from pProcParam\n")) ;

  NtReadVirtualMemory (hProcess, (LPVOID)(pProcParam+nOffset2),
		       &usImagePath, sizeof(usImagePath), NULL) ;

  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("NtReadVirtualMemory failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }  

  //TRACE_INFO (TEXT("usImagePath.Length = %d\n"), usImagePath.Length) ;

  pusString->Length = min (pusString->MaximumLength, usImagePath.Length) ;

  NtReadVirtualMemory (hProcess, usImagePath.Buffer,
		       pusString->Buffer, pusString->Length, NULL) ;

  pusString->Buffer[pusString->Length/2] = 0 ;

  //TRACE_INFO (TEXT("Process image = %ls\n"), wszPath) ;

  return STATUS_SUCCESS ;
}
*/

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcInfo_GetImagePath (HANDLE hProcess, PUNICODE_STRING pusPath) 
{
  NTSTATUS	nStatus ;
  int aOffsets[] = { 0x10, 0x38 } ;

  nStatus = _ProcInfo_GenericGetString (hProcess, pusPath, aOffsets, 2) ;

  if( nStatus!=STATUS_SUCCESS )
    TRACE_INFO (TEXT("_ProcInfo_GenericGetString failed (status=0x%08X)\n"), nStatus) ;

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcInfo_GetSystemRoot (HANDLE hProcess, PUNICODE_STRING pusPath) 
{
  int aOffsets[] = { 0x54, 0x04, 0x00 } ;

  return _ProcInfo_GenericGetString (hProcess, pusPath, aOffsets, 3) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcInfo_GetSystem32Root (HANDLE hProcess, PUNICODE_STRING pusPath) 
{
  int aOffsets[] = { 0x54, 0x04, 0x08 } ;

  return _ProcInfo_GenericGetString (hProcess, pusPath, aOffsets, 3) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcInfo_GetCurDirDosPath (HANDLE hProcess, PUNICODE_STRING pusPath) 
{
  int aOffsets[] = { 0x10, 0x40 } ;

  return _ProcInfo_GenericGetString (hProcess, pusPath, aOffsets, 2) ;
}



/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcInfo_GetProcessId (HANDLE hProcess, PULONG pId) 
{
  NTSTATUS		nStatus ;
  PROCESS_BASIC_INFORMATION pbi ;
  ULONG			nReturnLen = 0 ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (pId!=NULL) ;

  *pId = 0 ;

  nStatus = ZwQueryInformationProcess (hProcess,
				       ProcessBasicInformation,
				       &pbi, sizeof(pbi), &nReturnLen) ;

  if( nStatus!=STATUS_SUCCESS || nReturnLen<sizeof(pbi) )
    {
      TRACE_ERROR (TEXT("ZwQueryInformationProcess failed (handle=0x%08X,status=0x%08X)\n"), hProcess,nStatus) ;
      return nStatus ;
    }

  *pId = pbi.UniqueProcessId ;

  return STATUS_SUCCESS ;
}


NTSTATUS ProcInfo_GetAddress (HANDLE hProcess, PROCADDR * pnProcessAddress)
{
  LPVOID	pObject = NULL ;
  NTSTATUS	nStatus ;

  nStatus = ObReferenceObjectByHandle (hProcess, GENERIC_ALL,
				       NULL, KernelMode, &pObject, NULL) ;   

  if( nStatus!=STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
      *pnProcessAddress = 0 ;
      return nStatus ;
    }

  ObDereferenceObject (pObject) ;

  *pnProcessAddress = (PROCADDR)pObject ;

  return STATUS_SUCCESS ;
}

/*
NTSTATUS ProcInfo_KillProcessFromPid (ULONG nProcessId) 
{
  HANDLE	hProcess ;
  OBJECT_ATTRIBUTES oa ;
  CLIENT_ID	clid ;
  NTSTATUS	nStatus ;

  InitializeObjectAttributes (&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL) ;
  
  clid.UniqueProcess = (HANDLE)nProcessId ;
  clid.UniqueThread = 0 ;
  
  nStatus = ntundoc.ZwOpenProcess (&hProcess, PROCESS_TERMINATE, &oa, &clid) ;
  
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("ZwOpenProcess failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  nStatus = ntundoc.NtTerminateProcess (hProcess, 0) ;

  if( nStatus != STATUS_SUCCESS )
    TRACE_ERROR (TEXT("NtTerminateProcess failed (status=0x%08X)\n"), nStatus) ;

  ZwClose (hProcess) ;

  return nStatus ;
}
*/
