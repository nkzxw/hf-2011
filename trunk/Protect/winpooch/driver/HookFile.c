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
#include "HookFile.h"

// standard headers
#include <tchar.h>

#include <winsock2.h>

// ddk headers
#include <ntddk.h>

// project's headers
#include "Assert.h"
#include "FileInfo.h"
#include "HookCommon.h"
#include "Hooks.h"
#include "Malloc.h"
#include "NtUndoc.h"
#include "Strlcpy.h"
#include "Trace.h"
#include "WatchedObjects.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  int			nType ;
  int			nProtocol ;
  SOCKADDR_STORAGE	address ;
} SOCKETDATA ;

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
/* Exported function                                              */
/******************************************************************/

NTSTATUS WINAPI Hook_IoCreateFile (PHANDLE		FileHandle,
				   ACCESS_MASK		DesiredAccess,
				   POBJECT_ATTRIBUTES	ObjectAttributes,
				   PIO_STATUS_BLOCK	IoStatusBlock,
				   PLARGE_INTEGER	AllocationSize,
				   ULONG		FileAttributes,
				   ULONG		ShareAccess,
				   ULONG		Disposition,
				   ULONG		CreateOptions,
				   PVOID		EaBuffer,
				   ULONG		EaLength,
				   CREATE_FILE_TYPE	CreateFileType,
				   PVOID		ExtraCreateParameters,
				   ULONG		Options)
{
  PROC			pfnStub = Hooks_GetStubAddress (HOOKS_IOCREATEFILE) ;
  UNICODE_STRING	usDosPath ;
  BOOL			bIsFile ;
  NTSTATUS		nStatus ;
  UINT			nReaction = RULE_ACCEPT ;
  UINT			nOptions = 0 ;
  LARGE_INTEGER		liFileTime = {{0}} ;

  if( ObjectAttributes )
    TRACE_INFO (TEXT("%ls (0x%X)\n"), ObjectAttributes->ObjectName->Buffer, DesiredAccess) ;
  else
    TRACE_WARNING (TEXT("ObjectAttributes==NULL\n")) ;
 
  usDosPath.Length		= 0 ;
  usDosPath.MaximumLength	= sizeof(WCHAR) * MAX_PATH ;
  usDosPath.Buffer		= MALLOC(usDosPath.MaximumLength) ;
  
  // allocation failed ?
  if( usDosPath.Buffer == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate buffer for file name (%u bytes)\n"), 
		   usDosPath.MaximumLength) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }
  
  if( ObjectAttributes && 
      ObjectAttributes->ObjectName &&
      ObjectAttributes->ObjectName->Buffer )
    {
      nStatus = FileInfo_NtPathToDosPath (NULL, 
					  ObjectAttributes->RootDirectory,
					  ObjectAttributes->ObjectName,
					  &usDosPath) ;
    }
  else
    {
      RtlAppendUnicodeToString (&usDosPath, L"ObjectAttributes=NULL") ;
      nStatus = STATUS_UNSUCCESSFUL ;
    }

  if( CreateOptions & FILE_OPEN_BY_FILE_ID )
    TRACE_WARNING (TEXT("IoCreateFile called with flag FILE_OPEN_BY_FILE_ID (file=%ls)\n"), usDosPath.Buffer) ;
    
  //TRACE_ALWAYS (TEXT("%ls\n"), usDosPath.Buffer) ;

  // TO BE CHANGED  !!!!!!!!!!!!!
  bIsFile = nStatus==STATUS_SUCCESS && usDosPath.Buffer[1]==L':' && usDosPath.Buffer[2]==L'\\' ;  
  
  if( bIsFile ) 
    {
      // log call  
      if( ( DesiredAccess & (GENERIC_WRITE|GENERIC_ALL|0x10000) ) ||
	  ( Disposition==FILE_SUPERSEDE ) ||
	  ( Disposition==FILE_CREATE ) ||
	  ( Disposition==FILE_OVERWRITE ) ||
	  ( Disposition==FILE_OVERWRITE_IF)  ||
	  ( CreateOptions & (FILE_DELETE_ON_CLOSE)) ) 
	HookCommon_CatchCall (&nReaction, NULL,
			      FILTREASON_FILE_WRITE, 
			      TEXT("s"), usDosPath.Buffer) ;
      
      if( nReaction==RULE_ACCEPT && (DesiredAccess&(GENERIC_READ|GENERIC_ALL)) )
	HookCommon_CatchCall (&nReaction, &nOptions,
			      FILTREASON_FILE_READ, 
			      TEXT("s"), usDosPath.Buffer) ;
      
      if( nReaction != RULE_ACCEPT ) 
	{
	  IoStatusBlock->Status = STATUS_ACCESS_DENIED ;
	  IoStatusBlock->Information = 0 ;
	  
	  FREE (usDosPath.Buffer) ;
	  
	  return STATUS_ACCESS_DENIED ;
	}
      
      if( (nOptions & RULE_SCAN) && HookCommon_ShouldScanFile(usDosPath.Buffer) )
	{	  
	  TRACE_INFO (TEXT("Gonna scan %ls\n"), usDosPath.Buffer) ;

	  // open the file to get its time
	  nStatus = (NTSTATUS)pfnStub (FileHandle, DesiredAccess, ObjectAttributes,
				       IoStatusBlock, AllocationSize, FileAttributes,
				       ShareAccess, FILE_OPEN, 0,
				       NULL, 0, CreateFileTypeNone,
				       NULL, Options) ;

	  // if we can't open the file, just return
	  if( nStatus == STATUS_SUCCESS ) 
	    {     
	      // get file time
	      nStatus = FileInfo_GetLastWriteTimeFromHandle (*FileHandle, &liFileTime) ;
	      
	      TRACE_INFO (TEXT("File time = %lu\n"), liFileTime.QuadPart) ;
	      
	      // close the temporary handle, so the antivirus can read the file
	      ZwClose (*FileHandle) ;
	      
	      HookCommon_ScanFile (&nReaction, usDosPath.Buffer, &liFileTime) ;
	      
	      TRACE_INFO (TEXT("Scan result = %d\n"), nReaction) ;
	      
	      if( nReaction!=RULE_ACCEPT ) 
		{
		  TRACE_ALWAYS (TEXT("Denying access to %ls\n"), usDosPath.Buffer) ; 
		  FREE (usDosPath.Buffer) ;
		  *FileHandle = NULL ;
		  return STATUS_ACCESS_DENIED ;
		}
	    }
	  else
	    {
	      TRACE_INFO (TEXT("Failed to open %ls\n"), usDosPath.Buffer) ;
	    }
	}
    }
  
  //DbgPrint ("Calling IoCreateFile... %ls\n", usDosPath.Buffer); 
  
  nStatus = (NTSTATUS)pfnStub (FileHandle, DesiredAccess, ObjectAttributes,
			       IoStatusBlock, AllocationSize, FileAttributes,
			       ShareAccess, Disposition, CreateOptions,
			       EaBuffer, EaLength, CreateFileType,
			       ExtraCreateParameters, Options) ;
  
  //DbgPrint ("Calling IoCreateFile... result = 0x%08X\n", nStatus); 

  if( SUCCEEDED(nStatus) )
    {
      PVOID pObject = NULL ;

      if( bIsFile && liFileTime.QuadPart==0 ) 
	{      
	  nStatus = FileInfo_GetLastWriteTimeFromHandle (*FileHandle, &liFileTime) ;
	  if( nStatus!=STATUS_SUCCESS )
	    TRACE_ERROR (TEXT("FileInfo_GetLastWriteTimeFromHandle (file=\"%ls\", status=0x%08X)\n"), 
			 usDosPath.Buffer, nStatus) ;
	}

      nStatus = ObReferenceObjectByHandle (*FileHandle, GENERIC_ALL, NULL, KernelMode, &pObject, NULL) ;   
      if( nStatus==STATUS_SUCCESS && pObject!=NULL )
	{
	  nStatus = WatchObjs_Lock () ;
	  
	  if( nStatus == STATUS_SUCCESS )
	    {
	      nStatus = WatchObjs_AddWotFile (pObject, &liFileTime, &usDosPath) ;
	      WatchObjs_Unlock () ;
	    }
	  
	  if( FAILED(nStatus) )
	    TRACE_WARNING (TEXT("WatchObjs_AddWotFile failed (status=0x%08X)\n"), nStatus) ;

	  ObDereferenceObject (pObject) ;
	}
      else
	{
	  TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
	}

      nStatus = STATUS_SUCCESS ;
    }

  FREE (usDosPath.Buffer) ;
		   
  return nStatus ;
}


/******************************************************************/
/* Exported function :                                            */
/******************************************************************/

NTSTATUS WINAPI Hook_NtDeleteFile (POBJECT_ATTRIBUTES	ObjectAttributes) 
{
  PROC			pfnStub = Hooks_GetStubAddress (HOOKS_NTDELETEFILE) ;
  NTSTATUS		nStatus ;
  WCHAR			szFilePath[MAX_PATH] ;
  UNICODE_STRING	usDosPath ;
  BOOL			bIsFile ;
 
  usDosPath.Length		= 0 ;
  usDosPath.MaximumLength	= sizeof(WCHAR) * MAX_PATH ;
  usDosPath.Buffer		= szFilePath ;

  nStatus = STATUS_UNSUCCESSFUL ;

  if( ObjectAttributes &&
      ObjectAttributes->ObjectName && 
      ObjectAttributes->ObjectName->Buffer &&
      ObjectAttributes->ObjectName->Buffer[0] )
    {
      nStatus = FileInfo_NtPathToDosPath (NULL, 
					  ObjectAttributes->RootDirectory,
					  ObjectAttributes->ObjectName,
					  &usDosPath) ;
    }
  else if( ObjectAttributes &&
	   ObjectAttributes->RootDirectory )
    {
      WOTFILE		*pWotFileData ;
      ULONG		nWotDataSize ;
      PVOID		pObject = NULL ;

      nStatus = ObReferenceObjectByHandle (ObjectAttributes->RootDirectory, GENERIC_ALL,
					   NULL, KernelMode, &pObject, NULL) ;    
      
      if( nStatus==STATUS_SUCCESS && pObject!=NULL )
	{
	  nStatus = WatchObjs_Lock () ;   
	  
	  if( nStatus == STATUS_SUCCESS )
	    {   
	      nStatus = WatchObjs_GetFromPointer (pObject, WOT_FILE, (void**)&pWotFileData, &nWotDataSize) ;

	      if( nStatus==STATUS_SUCCESS )
		wcslcpy (szFilePath, pWotFileData->wszFilePath, MAX_PATH) ;
	      
	      WatchObjs_Unlock () ;
	    }

	  ObDereferenceObject (pObject) ;
	}   
      else 
	{
	  TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
	  return nStatus ;
	}         
    } 

  if( nStatus!=STATUS_SUCCESS )
    {
      wcslcpy (szFilePath, L"!! Unknown file !!", MAX_PATH) ;
    }
  
  TRACE_INFO (TEXT("Delete file : %ls\n"), szFilePath) ;
  
  bIsFile = nStatus==STATUS_SUCCESS && szFilePath[1]==L':' ;
    
  if( bIsFile )
    {
      UINT nReaction ;
	    
      // log call  
      HookCommon_CatchCall (&nReaction, NULL,
			    FILTREASON_FILE_WRITE, 
			    TEXT("s"), szFilePath) ;
	
      if( nReaction == RULE_FEIGN ) 
	return STATUS_SUCCESS ;
	
      if( nReaction != RULE_ACCEPT ) 
	return STATUS_ACCESS_DENIED ;
    }
  
  nStatus = (NTSTATUS) pfnStub (ObjectAttributes) ;

  TRACE_INFO (TEXT("NtDeleteFile result = 0x08X\n"), nStatus) ;

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WINAPI Hook_NtSetInformationFile (HANDLE		FileHandle,
					   PIO_STATUS_BLOCK	IoStatusBlock,
					   PVOID		FileInformation,
					   ULONG		Length,
					   ULONG		FileInformationClass)
{
  {
    UINT nReaction = RULE_ACCEPT ;

    TRACE_INFO ("FileInformationClass = %d\n", FileInformationClass) ;

    if( FileInformationClass==FileRenameInformation ||
	FileInformationClass==FileLinkInformation )
      {
	//
	// Original file name
	//
	if( nReaction == RULE_ACCEPT ) 	
	  {
	    NTSTATUS	nStatus ;
	    WCHAR	szFilePath[MAX_PATH] ;
	    PVOID	pObject = NULL ;

	    nStatus = ObReferenceObjectByHandle (FileHandle, GENERIC_ALL, NULL, KernelMode, &pObject, NULL) ;    
	    
	    if( nStatus==STATUS_SUCCESS && pObject!=NULL )
	      {
		WOTFILE	*pWotFileData ;
		ULONG	nWotDataSize ;

		nStatus = WatchObjs_Lock () ;
		
		if( nStatus == STATUS_SUCCESS )
		  {
		    nStatus = WatchObjs_GetFromPointer (pObject, 
							WOT_FILE, 
							(void**)&pWotFileData, 
							&nWotDataSize) ;    
		    if( nStatus==STATUS_SUCCESS )      
		      wcslcpy (szFilePath, pWotFileData->wszFilePath, MAX_PATH) ;
		    
		    WatchObjs_Unlock () ;
		  }
		else
		  wcslcpy (szFilePath, g_usUnknownFile.Buffer, MAX_PATH) ;

		ObDereferenceObject (pObject) ;
		
		HookCommon_CatchCall (&nReaction, NULL,
				      FILTREASON_FILE_WRITE, 
				      TEXT("s"), szFilePath) ;
	      }
	    else
	      {
		TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
		return nStatus ;
	      }
	  }
	
	if( nReaction == RULE_ACCEPT ) 
	  {
	    WCHAR		szFilePath[MAX_PATH] ;
	    FILE_LINK_RENAME_INFORMATION	*pInfo = FileInformation ;   
	    UNICODE_STRING	usDosPath, usNtPath ;
	    NTSTATUS nStatus ;
	    BOOL	bIsFile ;	
	    
	    usDosPath.Length = 0 ;
	    usDosPath.MaximumLength = sizeof(WCHAR) * MAX_PATH ;
	    usDosPath.Buffer = szFilePath ;
	    
	    usNtPath.Length = pInfo->FileNameLength ;
	    usNtPath.MaximumLength = pInfo->FileNameLength ;
	    usNtPath.Buffer = pInfo->FileName ;
	    
	    nStatus = FileInfo_NtPathToDosPath (NULL, 
						pInfo->RootDirectory,
						&usNtPath,
						&usDosPath) ;
	    
	    bIsFile = nStatus==STATUS_SUCCESS && szFilePath[1]==L':' ;   
	    
	    TRACE_INFO (TEXT("File path = %s\n"), szFilePath) ;
	    
	    if( bIsFile )
	      {
		HookCommon_CatchCall (&nReaction, NULL,
				      FILTREASON_FILE_WRITE, 
				      TEXT("s"), szFilePath) ;
	      }
	  }
      }

    if( nReaction == RULE_FEIGN ) 
      return STATUS_SUCCESS ;
    
    if( nReaction != RULE_ACCEPT ) 
      return STATUS_ACCESS_DENIED ;
  }

  JUMP_TO_STUB (HOOKS_NTSETINFORMATIONFILE) ;
  ASSERT (0) ;
  return 0 ;
}


void _HookFile_IPv4toString (LPWSTR szAddr, struct in_addr * pAddr)
{
  ntundoc.swprintf (szAddr, L"%d.%d.%d.%d", 
		    pAddr->S_un.S_un_b.s_b1,
		    pAddr->S_un.S_un_b.s_b2,
		    pAddr->S_un.S_un_b.s_b3,
		    pAddr->S_un.S_un_b.s_b4) ;
}

int _HookFile_GetProtocol (HANDLE h)
{
  SOCKETDATA	*pData ;
  ULONG		nSize ;
  NTSTATUS	nStatus ;
  int		nProtocol = 1 ;
  PVOID		pObject = NULL ;
  
  nStatus = ObReferenceObjectByHandle (h, GENERIC_ALL, NULL, KernelMode, &pObject, NULL) ;    
      
  if( nStatus==STATUS_SUCCESS && pObject!=NULL )
    {
      nStatus = WatchObjs_Lock () ;

      if( nStatus == STATUS_SUCCESS )
	{  
	  nStatus = WatchObjs_GetFromPointer (pObject, WOT_SOCKET, (void**)&pData, &nSize) ;
	  
	  if( nStatus==STATUS_SUCCESS )
	    nProtocol = pData->nProtocol ;
	  
	  WatchObjs_Unlock () ;
	}
      
      ObDereferenceObject (pObject) ;
    }   
  else 
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
    }    
  
  return nProtocol ;
}


VOID DDKAPI _HookFile_Recv (PVOID ApcContext,
			    PIO_STATUS_BLOCK IoStatusBlock,
			    ULONG Reserved)
{
  DbgPrint ("+++++++++++++++ COUCOU +++++++++++++++++\n") ;
}
  
/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WINAPI Hook_NtDeviceIoControlFile (HANDLE FileHandle,
					    HANDLE Event,
					    PIO_APC_ROUTINE ApcRoutine,
					    PVOID ApcContext,
					    PIO_STATUS_BLOCK IoStatusBlock,
					    ULONG IoControlCode,
					    PVOID InputBuffer,
					    ULONG InputBufferLength,
					    PVOID OutputBuffer,
					    ULONG OutputBufferLength) 
{
/*
  {
    NTSTATUS	nStatus ;
    ULONG	n, nRemainBytes ;

    WOTFILE	*pWotFileData ;
    ULONG	nWotDataSize ;

    WatchObjs_Lock () ;      
    nStatus = WatchObjs_GetFromHandle (FileHandle, 
				       WOT_FILE, 
				       (void**)&pWotFileData, 
				       &nWotDataSize) ;    
    if( nStatus!=STATUS_SUCCESS )
      pWotFileData = NULL ;

    if( (IoControlCode & 0x00FF000) == 0x12000 )
       //if( IoControlCode == 0x12003 )
      {
	DbgPrint ("  I/O Control 0x%08X on %ls\n", IoControlCode, pWotFileData ? pWotFileData->wszFilePath : L"???") ; 

	nRemainBytes = InputBufferLength ;
	
	if( nRemainBytes>32 ) nRemainBytes = 32 ;
	
	for( n=0 ; n<nRemainBytes ; n+=4 )
	  DbgPrint ("  +%04X - %08X - %02X %02X %02X %02X\n", n,
		    *(ULONG*)((BYTE*)InputBuffer+n),
		    ((BYTE*)InputBuffer+n)[0],
		    ((BYTE*)InputBuffer+n)[1], 
		    ((BYTE*)InputBuffer+n)[2],
		    ((BYTE*)InputBuffer+n)[3]) ; 	  
      }

    WatchObjs_Unlock () ;

    // if( IoControlCode == 0x1201F )
    //  DbgBreakPoint () ;
  }
*/
  {
    switch( IoControlCode )
      {
      case 0x12003:
	{
	  struct INPUT 
	  {
	    DWORD     	nReserved0 ;
	    DWORD	nReserved4 ;
	    WORD	nAddrSize ;
	    SOCKADDR_IN	addr ;	    
	  } PACKED ;

	  SOCKADDR_IN *pAddr = &((struct INPUT*)InputBuffer)->addr ;
	  // SOCKADDR_IN *pAddr = (void*)( (BYTE*)InputBuffer + 0x0A ) ;

	  if( ((struct INPUT*)InputBuffer)->nReserved0==0 && pAddr->sin_family == AF_INET )
	    {
	      UINT		nReaction ;
	      WCHAR		szAddr[16] ;
	      USHORT		nPort, nProtocol ;	      

	      _HookFile_IPv4toString (szAddr, &pAddr->sin_addr) ;
	    
	      ((BYTE*)&nPort)[0] = ((BYTE*)&pAddr->sin_port)[1] ;
	      ((BYTE*)&nPort)[1] = ((BYTE*)&pAddr->sin_port)[0] ;

	      nProtocol = _HookFile_GetProtocol(FileHandle) ;
	     
	      TRACE_INFO (TEXT("Bind (addr=%ls, port=%d, proto=%d)\n"), szAddr, nPort, nProtocol) ;	    
	    
	      HookCommon_CatchCall (&nReaction, NULL,
				    FILTREASON_NET_LISTEN, 
				    TEXT("snn"), szAddr, nPort, nProtocol) ;

	      if( nReaction!=RULE_ACCEPT )
		return STATUS_ACCESS_DENIED ;
	    }
	}
	break ;

      case 0x12007:
	{
	  struct sockaddr_in * pAddr ;

	  pAddr = (void*)( (BYTE*)InputBuffer + 0x12 ) ;

	  if( pAddr->sin_family == AF_INET )
	    {
	      NTSTATUS		nStatus ;
	      PVOID		pObject ;
	      UINT		nReaction ;
	      SOCKETDATA	*pData ;
	      ULONG		nSize ;
	      
	      USHORT		nPort, nProtocol ;	      
	      WCHAR		szAddr[16] ;

	      nStatus = ObReferenceObjectByHandle (FileHandle, GENERIC_ALL, NULL, KernelMode, &pObject, NULL) ;    
	      if( nStatus!=STATUS_SUCCESS || pObject==NULL )
		{
		  TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
		  return nStatus ;
		}
		 		  
	      nStatus = WatchObjs_Lock () ;
	      if( nStatus != STATUS_SUCCESS ) 
		{
		  ObDereferenceObject (pObject) ;
		  return nStatus ;
		}
	      
	      nStatus = WatchObjs_GetFromPointer (pObject, WOT_SOCKET, (void**)&pData, &nSize) ;
	      
	      if( nStatus!=STATUS_SUCCESS )
		{
		  TRACE_WARNING (TEXT("Socket 0x%08X was not in object list\n"), FileHandle) ;
		  nSize = sizeof(SOCKETDATA) ;
		  pData = MALLOC (nSize) ;
		  
		  if( pData == NULL )
		    {
		      TRACE_ERROR (TEXT("Failed to allocate strcuture SOCKETDATA (%u bytes)\n"),
				   nSize) ;
		      WatchObjs_Unlock () ;
		      ObDereferenceObject (pObject) ;
		      return STATUS_INSUFFICIENT_RESOURCES ;
		    }
		  
		  memset (pData, 0, nSize) ;
		  pData->nProtocol = -1 ;
		  WatchObjs_AddFromPointer (pObject, WOT_SOCKET, pData, nSize) ;
		}
	      
	      memcpy (&pData->address, pAddr, sizeof(SOCKADDR_IN)) ;
	      nProtocol = pData->nProtocol ;
	      
	      WatchObjs_Unlock () ;	  
	      ObDereferenceObject (pObject) ;

	      _HookFile_IPv4toString (szAddr, &pAddr->sin_addr) ;
	      
	      ((BYTE*)&nPort)[0] = ((BYTE*)&pAddr->sin_port)[1] ;
	      ((BYTE*)&nPort)[1] = ((BYTE*)&pAddr->sin_port)[0] ;
	      
	      TRACE_INFO ("Connect (addr=%ls, port=%d, proto=%d)\n", szAddr, nPort, nProtocol) ;
	      
	      HookCommon_CatchCall (&nReaction, NULL,
				    FILTREASON_NET_CONNECT, 
				    TEXT("snn"), szAddr, nPort, nProtocol) ;

	      if( nReaction!=RULE_ACCEPT )
		return STATUS_ACCESS_DENIED ;
	    }
	}
	break ;	

	//
	// recvfrom () ;
	//
      case 0x1201B:
	{
	  TRACE_INFO (TEXT("Recvfrom not supported\n")) ;
	}
	break ;

	//
	// send () ;
	//
      case 0x1201F:
	{
	  NTSTATUS	nStatus ;
	  UINT		nReaction ;
	  SOCKETDATA	*pData ;
	  ULONG		nSize ;
	  struct sockaddr_in addr ;
	  WCHAR		szAddr[16] ;
	  USHORT	nPort, nProtocol ;
	  PVOID		pObject ;

	  nStatus = ObReferenceObjectByHandle (FileHandle, GENERIC_ALL, NULL, KernelMode, &pObject, NULL) ;    
	  if( nStatus!=STATUS_SUCCESS || pObject==NULL )
	    {
	      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
	      return nStatus ;
	    }

	  nStatus = WatchObjs_Lock () ;
	  if( nStatus != STATUS_SUCCESS )
	    {
	      ObDereferenceObject (pObject) ;
	      return nStatus ;
	    }
	  
	  nStatus = WatchObjs_GetFromPointer (pObject, WOT_SOCKET, (void**)&pData, &nSize) ;
	  
	  if( nStatus==STATUS_SUCCESS )
	    {
	      memcpy (&addr, &pData->address, sizeof(SOCKADDR_IN)) ;
	      nProtocol = pData->nProtocol ;
	    }
	  
	  WatchObjs_Unlock () ;
	  ObDereferenceObject (pObject) ;

	  if( nStatus==STATUS_SUCCESS && addr.sin_family==AF_INET )
	    {
	      _HookFile_IPv4toString (szAddr, &addr.sin_addr) ;

	      ((BYTE*)&nPort)[0] = ((BYTE*)&addr.sin_port)[1] ;
	      ((BYTE*)&nPort)[1] = ((BYTE*)&addr.sin_port)[0] ;

	      HookCommon_CatchCall (&nReaction, NULL,
				    FILTREASON_NET_SEND, 
				    TEXT("snn"), szAddr, nPort, nProtocol) ;
	      
	      if( nReaction!=RULE_ACCEPT )
		return STATUS_ACCESS_DENIED ;
	    }
	}
	break ;

	//
	// sendto () ;
	//
      case 0x12023:
	{
	  BYTE ** p ;
	  struct sockaddr_in * pAddr ;

	  p = (BYTE**)((BYTE*)InputBuffer + 0x34) ;
	  pAddr = (void*)( *p+6 ) ;

	  if( pAddr->sin_family == AF_INET )
	    {
	      UINT	nReaction ;
	      USHORT	nPort, nProtocol ;
	      WCHAR	szAddr[16] ;

	      _HookFile_IPv4toString (szAddr, &pAddr->sin_addr) ;
	    
	      ((BYTE*)&nPort)[0] = ((BYTE*)&pAddr->sin_port)[1] ;
	      ((BYTE*)&nPort)[1] = ((BYTE*)&pAddr->sin_port)[0] ;

	      nProtocol = _HookFile_GetProtocol(FileHandle) ;
	    
	      TRACE_INFO (TEXT("Sendto (addr=%ls, port=%d, proto=%d)\n"), szAddr, nPort, nProtocol) ;

	      HookCommon_CatchCall (&nReaction, NULL,
				    FILTREASON_NET_SEND, 
				    TEXT("snn"), szAddr, nPort, nProtocol) ;

	      if( nReaction!=RULE_ACCEPT )
		return STATUS_ACCESS_DENIED ;
	    }
	}
	break ;	

      case 0x12047:
	{	  
	  struct INPUT
	  {
	    ULONG	nReserved0 ;
	    ULONG	nAddressFamily ;
	    ULONG	nSocketType ;
	    ULONG	nProtocol ;
	  } ;

	  SOCKETDATA	*pData ;
	  ULONG		nSize ;
	  NTSTATUS	nStatus ;
	  PVOID		pObject ;

	  nStatus = ObReferenceObjectByHandle (FileHandle, GENERIC_ALL, NULL, KernelMode, &pObject, NULL) ;    
	  if( nStatus!=STATUS_SUCCESS || pObject==NULL )
	    {
	      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), nStatus) ;
	      return nStatus ;
	    }

	  nStatus = WatchObjs_Lock () ;
	  if( nStatus != STATUS_SUCCESS ) 
	    {
	      ObDereferenceObject (pObject) ;
	      return nStatus ;
	    }
	  
	  nStatus = WatchObjs_GetFromPointer (pObject, WOT_SOCKET, (void**)&pData, &nSize) ;

	  if( nStatus != STATUS_SUCCESS )
	    {
	      nSize = sizeof(SOCKETDATA) ;
	      pData = MALLOC (nSize) ;
	      
	      if( pData == NULL )
		{
		  TRACE_ERROR (TEXT("Failed to allocate strcuture SOCKETDATA (%u bytes)\n"), nSize) ;
		  WatchObjs_Unlock () ;
		  ObDereferenceObject (pObject) ;
		  return STATUS_INSUFFICIENT_RESOURCES ;
		}	      

	      pData->address.ss_family = AF_UNSPEC ;
	      WatchObjs_AddFromPointer (pObject, WOT_SOCKET, pData, nSize) ;
	    }
	  
	  pData->nType = ((struct INPUT*)InputBuffer)->nSocketType ;
	  pData->nProtocol = ((struct INPUT*)InputBuffer)->nProtocol ;

	  TRACE_INFO (TEXT("Socket 0x%08X (type=%d, protocol=%d)\n"), 
			FileHandle, pData->nType, pData->nProtocol) ;  
	  
	  WatchObjs_Unlock () ;
	  ObDereferenceObject (pObject) ;
	}
	break ;
      }
  }
 
  JUMP_TO_STUB (HOOKS_NTDEVICEIOCONTROLFILE) ;
  ASSERT (0) ;
  return 0 ;
}

/*
NTSTATUS WINAPI Hook_NtWriteFile (IN HANDLE FileHandle,
				  IN HANDLE  Event  OPTIONAL,
				  IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
				  IN PVOID  ApcContext  OPTIONAL,
				  OUT PIO_STATUS_BLOCK  IoStatusBlock,
				  IN PVOID  Buffer,
				  IN ULONG  Length,
				  IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
				  IN PULONG  Key  OPTIONAL) 
{
  {
    NTSTATUS	nStatus ;
    LPVOID pUserData ;
    ULONG nUserDataSize ;

    nStatus = WatchObjs_Lock () ;
    if( nStatus != STATUS_SUCCESS ) return nStatus ;

    nStatus = WatchObjs_GetFromHandle (FileHandle, WOT_FILE,
				       &pUserData, &nUserDataSize) ;

    if( nStatus!=STATUS_SUCCESS )
      pUserData = L"?????" ;

    DbgPrint ("  Write %lu bytes on %ls\n", Length, pUserData) ; 

    WatchObjs_Unlock () ;
  }

  JUMP_TO_STUB (HOOKS_NTWRITEFILE) ;
  ASSERT (0) ;
  return 0 ; 
}
*/
