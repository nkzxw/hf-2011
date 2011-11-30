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

#define	TRACE_LEVEL	1 // error level
#define APPEND_TERMINAL_NULL	0

/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "FileInfo.h"

// ddk's header
#include <ddk/ntapi.h>

// project's headers
#include "Hooks.h"
#include "Malloc.h"
#include "NtUndoc.h"
#include "ProcInfo.h"
#include "SystInfo.h"
#include "Trace.h"
#include "WatchedObjects.h"


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

BOOL _FileInfo_CheckPrefix (UNICODE_STRING * pusPath, LPCWSTR wszPrefix) ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS FileInfo_NtPathToDosPath (HANDLE		hProcess,
				   HANDLE		hDirectory,
				   PUNICODE_STRING	pusNtPath,
				   PUNICODE_STRING	pusDosPath) 
{
  ASSERT (pusDosPath!=NULL) ;
  ASSERT (pusDosPath->Buffer!=NULL) ;

  if( pusNtPath==NULL || pusNtPath->Buffer==NULL )
    {
      TRACE_WARNING (TEXT("NT Path is NULL\n")) ;
      pusDosPath->Buffer[0] = 0 ;
      pusDosPath->Length = 0 ;
      return STATUS_OBJECT_PATH_INVALID ;
    }

  TRACE_INFO (TEXT("pusNtPath->Length = %d\n"), pusNtPath->Length) ;
  TRACE_INFO (TEXT("pusNtPath->MaximumLength = %d\n"), pusNtPath->MaximumLength) ;
  TRACE_INFO (TEXT("pusNtPath->Buffer = %ls\n"), pusNtPath->Buffer) ;
  
  if( _FileInfo_CheckPrefix(pusNtPath,L"\\??\\") ) 
    {
      if( pusNtPath->Buffer[5]==L':' ) 
	{
	  UNICODE_STRING	usTemp ;
	  
	  usTemp.Length		= pusNtPath->Length-4*sizeof(WCHAR) ;
	  usTemp.MaximumLength	= pusNtPath->MaximumLength-4*sizeof(WCHAR) ;
	  usTemp.Buffer		= pusNtPath->Buffer+4 ;
	  
	  //TRACE_INFO (TEXT("Removing prefix \\??\\ on absolute path\n")) ;
	  
	  RtlCopyUnicodeString (pusDosPath, &usTemp) ;
#if APPEND_TERMINAL_NULL
	  pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif
	}
      else
	{
	  RtlCopyUnicodeString (pusDosPath, pusNtPath) ;
#if APPEND_TERMINAL_NULL
	  pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif
	}

      return STATUS_SUCCESS ;
    }


  if( _FileInfo_CheckPrefix(pusNtPath,L"\\SystemRoot\\") ) 
    {
      UNICODE_STRING	usTemp ;
     
      //TRACE_INFO (TEXT("Changing prefix \\SystemRoot\n")) ;
      
      if( hProcess && STATUS_SUCCESS==ProcInfo_GetSystemRoot(hProcess, pusDosPath) )
	{

	}
      else if( STATUS_SUCCESS==SystInfo_GetSystemRoot(pusDosPath) )
	{

	}
      else 
	{
	  TRACE_WARNING (TEXT("SystInfo_GetSystemRoot failed\n")) ;
	  RtlInitUnicodeString (&usTemp, L"%systemroot%") ;
	  RtlCopyUnicodeString (pusDosPath, &usTemp) ;
	} 

      usTemp.Length		= pusNtPath->Length-11*sizeof(WCHAR) ;
      usTemp.MaximumLength	= pusNtPath->MaximumLength-11*sizeof(WCHAR) ;
      usTemp.Buffer		= pusNtPath->Buffer+11 ;

      RtlAppendUnicodeStringToString (pusDosPath, &usTemp) ;
#if APPEND_TERMINAL_NULL
      pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif
      return STATUS_SUCCESS ;   
    }

  if( _FileInfo_CheckPrefix(pusNtPath,L"\\Device\\") ) 
   {
     RtlCopyUnicodeString (pusDosPath, pusNtPath) ;
#if APPEND_TERMINAL_NULL
     pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif
     return STATUS_SUCCESS ;
   }

  if( pusNtPath->Buffer[1]==L':' )
    {
      RtlCopyUnicodeString (pusDosPath, pusNtPath) ;
#if APPEND_TERMINAL_NULL  
      pusDosPath->Buffer[pusDosPath->Length/2] = 0 ; 
#endif
      return STATUS_SUCCESS ;  
    }


  TRACE_WARNING (TEXT("Difficulty : %ls (length=%u)\n"), pusNtPath->Buffer, pusNtPath->Length) ;
  
  if( hDirectory!=NULL )
    { 
      UNICODE_STRING	usDirectory ;
      NTSTATUS		nStatus ;
      WCHAR		wszDirectory[MAX_PATH] ;

      TRACE_WARNING (TEXT("Trying to complete with directory handle\n")) ;
	      
      usDirectory.Length = 0 ;
      usDirectory.MaximumLength = MAX_PATH * sizeof(WCHAR) ;
      usDirectory.Buffer = wszDirectory ;
      
      nStatus = FileInfo_GetPath (hDirectory, &usDirectory) ;	 
      
      if( nStatus==STATUS_SUCCESS )
	{ 
	  TRACE_WARNING (TEXT("Directory = %ls\n"), usDirectory.Buffer) ;
	  RtlAppendUnicodeToString(&usDirectory, L"\\") ;
	  RtlAppendUnicodeStringToString (&usDirectory, pusNtPath) ;
	  RtlCopyUnicodeString (pusDosPath, &usDirectory) ;
#if APPEND_TERMINAL_NULL
	  pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif
	}
      else
	{
	  TRACE_WARNING (TEXT("FileInfo_GetPath failed (status=0x%08X)\n"), nStatus) ;
	  RtlCopyUnicodeString (pusDosPath, pusNtPath) ;
#if APPEND_TERMINAL_NULL
	  pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif
	}
    }
  else if( hProcess!=NULL )
    {
      NTSTATUS nStatus ;
      
      TRACE_WARNING (TEXT("Trying to complete with current directory\n")) ;
      
      nStatus = ProcInfo_GetCurDirDosPath (hProcess, pusDosPath) ;
      
      if( nStatus==STATUS_SUCCESS )
	{
	  RtlAppendUnicodeStringToString (pusDosPath, pusNtPath) ;
#if APPEND_TERMINAL_NULL
	  pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif  
	}
      else
	{
	  TRACE_ERROR (TEXT("Failed to get current directory\n")) ;
	  RtlCopyUnicodeString (pusDosPath, pusNtPath) ;
#if APPEND_TERMINAL_NULL
	  pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;
#endif
	}
    }
  else
    {			       
      TRACE_WARNING (TEXT("No directory\n")) ;
      RtlCopyUnicodeString (pusDosPath, pusNtPath) ;
#if APPEND_TERMINAL_NULL
      pusDosPath->Buffer[pusDosPath->Length/2] = 0 ;    
#endif
    }
  
  TRACE_WARNING (TEXT("Result : %ls\n"), pusDosPath->Buffer) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _FileInfo_CheckPrefix (UNICODE_STRING* pusPath, LPCWSTR wszPrefix)
{
  int i ;

  ASSERT (pusPath!=NULL) ;
  ASSERT (pusPath->Buffer!=NULL) ;
  ASSERT (wszPrefix!=NULL) ;

  for( i=0 ; wszPrefix[i] ; i++ )
    {
      if( i>=pusPath->Length ) return FALSE ;
      if( pusPath->Buffer[i]==0 ) return FALSE ;

      if( RtlUpcaseUnicodeChar(pusPath->Buffer[i])  !=
	  RtlUpcaseUnicodeChar(wszPrefix[i]) ) 
	return FALSE ;
    }
      
  return TRUE;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/
/*
HANDLE FileInfo_GetRootDirectory (HANDLE hFile)
{
  PVOID		pObject=NULL ;
  OBJECT_HEADER *pHeader ;
  OBJECT_CREATE_INFO	*pCreateInfo=NULL ;
  NTSTATUS	nStatus ;
  HANDLE	hRootDir = NULL ;

  nStatus = ObReferenceObjectByHandle (hFile, GENERIC_ALL,
				       NULL, KernelMode, &pObject,
				       NULL) ;    

  if( nStatus!=STATUS_SUCCESS || pObject==NULL )
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), 
		   nStatus) ;
      return NULL ;
    }
    
  pHeader = (OBJECT_HEADER*)((BYTE*)pObject-0x18) ;

  if( pHeader->ObjectFlags & OB_FLAG_CREATE_INFO )
    {
      pCreateInfo = pHeader->ObjectCreateInfo ;

      if( pCreateInfo != NULL )
	{
	  hRootDir = pCreateInfo->RootDirectory ;
	}
      else TRACE_WARNING (TEXT("ObjectCreateInfo is NULL\n")) ;
    }
  else TRACE_WARNING (TEXT("OB_FLAG_CREATE_INFO not set\n")) ;
  
  ObDereferenceObject (pObject) ;

  return hRootDir ;
}
*/

/******************************************************************/
/* Exported function                                              */
/******************************************************************/
/*
NTSTATUS FileInfo_GetNtPath (HANDLE hFile, PUNICODE_STRING pusPath)
{
  NTSTATUS nStatus ;
  IO_STATUS_BLOCK	iosb ;
  FILE_NAME_INFORMATION *pFni ;
  ULONG	nSize = sizeof(FILE_NAME_INFORMATION)+MAX_PATH*sizeof(WCHAR) ;
  UNICODE_STRING	usTemp ;

  pFni = MALLOC (nSize) ;
  
  nStatus = ZwQueryInformationFile (hFile, &iosb,
				    pFni, nSize,
				    FileNameInformation) ;

  if( nStatus!=STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("ZwQueryInformationFile failed (status=0x%08X)\n"),
		   nStatus) ;
      return nStatus ;
    }

  RtlInitUnicodeString (&usTemp, pFni->FileName) ;
 
  RtlCopyUnicodeString (pusPath, &usTemp) ;

  FREE (pFni) ;

  //pusPath->Buffer[pusPath->Length/2+1] = 0 ;

  return STATUS_SUCCESS ;
}
*/


/******************************************************************/
/* Exported function                                              */
/******************************************************************/
/*
NTSTATUS FileInfo_GetDosPath (HANDLE hFile, PUNICODE_STRING pusDosPath) 
{
  NTSTATUS		nStatus ;
  UNICODE_STRING	usNtPath ;
  WCHAR			wszBuffer[MAX_PATH] ;

 

  usNtPath.Length = 0 ;
  usNtPath.MaximumLength = sizeof(WCHAR) * MAX_PATH ;
  usNtPath.Buffer = wszBuffer ;

  nStatus = FileInfo_GetNtPath (hFile, &usNtPath) ;

  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("FileInfo_GetNtPath failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }    

  nStatus = FileInfo_NtPathToDosPath (NULL, 
				      NULL, 
				      &usNtPath, 
				      pusDosPath) ;

  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("FileInfo_GetNtPath failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  //pusDosPath->Buffer[pusDosPath->Length/2+1] = 0 ;

  return STATUS_SUCCESS ;
}

*/


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS FileInfo_GetPath (IN HANDLE hFile, 
			   OUT PUNICODE_STRING pusFilePath) 
{
  UNICODE_STRING	usFilePath ;
  ULONG			nSize ;
  NTSTATUS		nStatus ;
  FILE_NAME_INFORMATION *pFni ;
  IO_STATUS_BLOCK	iosb ;
  LARGE_INTEGER		liFileTime ;
  WOTFILE		*pWotFileData ;
  PVOID			pObject=NULL ;

  // verify params
  ASSERT (hFile!=NULL) ;
  ASSERT (pusFilePath!=NULL) ;
  ASSERT (pusFilePath->Buffer!=NULL) ;
  ASSERT (pusFilePath->MaximumLength>0) ;

  //
  // 1. Get object pointer
  //

  nStatus = ObReferenceObjectByHandle (hFile, GENERIC_ALL, NULL, KernelMode, &pObject, NULL) ;    
  
  if( nStatus!=STATUS_SUCCESS || pObject==NULL )
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }
  
  //
  // 2. Try to look in watched object list
  //

  nStatus = WatchObjs_Lock () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;  

  nStatus = WatchObjs_GetFromPointer (pObject, WOT_FILE, (void**)&pWotFileData, &nSize) ;
  
  if( nStatus==STATUS_SUCCESS )
    {    
      usFilePath.Buffer		= pWotFileData->wszFilePath ;
      usFilePath.Length		= nSize - sizeof(WOTFILE) ;
      usFilePath.MaximumLength	= nSize - sizeof(WOTFILE) ;

      RtlCopyUnicodeString (pusFilePath, &usFilePath) ;

      WatchObjs_Unlock () ;
      ObDereferenceObject (pObject) ;

      return STATUS_SUCCESS ;
    }

  WatchObjs_Unlock () ;

  //
  // 3. If not found, try to get file path thru Windows native API
  //

  nSize = sizeof(FILE_NAME_INFORMATION)+MAX_PATH*sizeof(WCHAR) ;
      
  pFni = MALLOC (nSize) ;

  if( pFni == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate strcuture FILE_NAME_INFORMATION (%u bytes)\n"), nSize) ;
      ObDereferenceObject (pObject) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }
  
  nStatus = ZwQueryInformationFile (hFile, &iosb,
				    pFni, nSize,
				    FileNameInformation) ;
  
  if( nStatus!=STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("ZwQueryInformationFile failed (status=0x%08X)\n"),
		   nStatus) ;
      FREE (pFni) ;
      ObDereferenceObject (pObject) ;
      return nStatus ;
    }
  
  usFilePath.Buffer		= pFni->FileName ;   
  usFilePath.Length		= pFni->FileNameLength ;
  usFilePath.MaximumLength	= pFni->FileNameLength ; 
       
  //
  // 4. Convert NT file path to DOS file path
  //

  nStatus = FileInfo_NtPathToDosPath (NULL, NULL, &usFilePath, pusFilePath) ;

  FREE (pFni) ; 
      
  if( nStatus != STATUS_SUCCESS )
    TRACE_WARNING (TEXT("FileInfo_NtPathToDosPath failed (status=0x%08X)\n"), nStatus) ;

  //
  // 5. Set file name in watched object list
  //
  
  nStatus = FileInfo_GetLastWriteTimeFromHandle (hFile, &liFileTime) ;
  if( nStatus!=STATUS_SUCCESS )
    TRACE_ERROR (TEXT("FileInfo_GetLastWriteTimeFromHandle failed (status=0x%08X)\n"), nStatus) ;

  nStatus = WatchObjs_Lock () ;
  
  if( nStatus == STATUS_SUCCESS )
    {
      nStatus = WatchObjs_AddWotFile (pObject, &liFileTime, pusFilePath) ;
      WatchObjs_Unlock () ;
    }

  if( nStatus!=STATUS_SUCCESS )
    TRACE_WARNING (TEXT("FileInfo_AssocFileName failed (status=0x%08X)\n"), nStatus) ;

  //
  // 5. Release object pointer (WatchObjs must be unlocked before this)
  //
  
  ObDereferenceObject (pObject) ;

  return STATUS_SUCCESS ;
}



NTSTATUS FileInfo_GetLastWriteTime (IN LPCWSTR		wszFilePath,
				    OUT LARGE_INTEGER	* pliTime) 
{
  HANDLE			hFile ;
  PROC				pfnIoCreateFile ;
  UNICODE_STRING		usFilePath ;
  OBJECT_ATTRIBUTES		oa ;
  IO_STATUS_BLOCK		iostatus ;
  NTSTATUS			nStatus ;
  FILE_BASIC_INFORMATION	fbi ;

  pliTime->QuadPart = 0 ;
 
  pfnIoCreateFile = Hooks_GetStubAddress (HOOKS_IOCREATEFILE) ;

  if( pfnIoCreateFile==NULL )
    pfnIoCreateFile = (PROC)IoCreateFile ;

  RtlInitUnicodeString (&usFilePath, wszFilePath) ;
  InitializeObjectAttributes (&oa, &usFilePath, OBJ_KERNEL_HANDLE, NULL, NULL) ;

  nStatus = (NTSTATUS)pfnIoCreateFile (&hFile, GENERIC_READ, &oa,
				       &iostatus, 0, 0,
				       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
				       FILE_OPEN, 0, NULL, 0,
				       CreateFileTypeNone, NULL, 0) ;
  
  if( FAILED(nStatus) ) 
    {
      TRACE_ERROR(TEXT("IoCreateFile failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  nStatus = ZwQueryInformationFile (hFile, &iostatus,
				    &fbi, sizeof(fbi),
				    FileBasicInformation) ;

  if( FAILED(nStatus) ) 
    TRACE_ERROR(TEXT("ZwQueryInformationFile failed (status=0x%08X)\n"), nStatus) ;

  if( SUCCEEDED(nStatus) )
    *pliTime = fbi.LastWriteTime ;
  
  ZwClose (hFile) ;
  
  return nStatus ;
}


NTSTATUS FileInfo_GetLastWriteTimeFromHandle (IN HANDLE		hFile,
					      OUT LARGE_INTEGER	* pliTime) 
{
  IO_STATUS_BLOCK		iostatus ;
  NTSTATUS			nStatus ;
  FILE_BASIC_INFORMATION	fbi ;

  pliTime->QuadPart = 0 ;

  nStatus = ZwQueryInformationFile (hFile, &iostatus,
				    &fbi, sizeof(fbi),
				    FileBasicInformation) ;

  if( FAILED(nStatus) ) 
    TRACE_ERROR(TEXT("ZwQueryInformationFile failed (status=0x%08X)\n"), nStatus) ;

  if( SUCCEEDED(nStatus) )
    *pliTime = fbi.LastWriteTime ;
    
  return nStatus ;
}
