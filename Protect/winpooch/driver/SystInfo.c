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
#include "SystInfo.h"

// ddk's header
#include <ddk/ntapi.h>

// project's headers
#include "Malloc.h"
#include "Trace.h"


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

WORD SystInfo_GetWindowsVersion () 
{  
  BOOL	bCheckedVersion ;  
  ULONG	nMajorVersion ;
  ULONG nMinorVersion ;
  ULONG	nBuildNumber ;
 
  WCHAR			wszCSDVersion[64] = L"" ;
  UNICODE_STRING	usCSDVersion  ;

  usCSDVersion.Buffer = wszCSDVersion ;
  usCSDVersion.Length = 0 ;
  usCSDVersion.MaximumLength = 128 ;     

  bCheckedVersion = PsGetVersion (&nMajorVersion,
				  &nMinorVersion,
				  &nBuildNumber,
				  &usCSDVersion) ;

  TRACE_INFO ("Checked version = %d\n", bCheckedVersion) ;
  TRACE_INFO ("Major version = %d\n", nMajorVersion) ;
  TRACE_INFO ("Minor version = %d\n", nMinorVersion) ;
  TRACE_INFO ("Build number = %d\n", nBuildNumber) ;
  TRACE_INFO ("CSD version = %ls\n", wszCSDVersion) ;

  if( bCheckedVersion ) return UNKNOWN_WINDOWS_VERSION ;

  return (nMajorVersion << 8) | nMinorVersion ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS SystInfo_GetSystemRoot (PUNICODE_STRING pusPath) 
{
  pusPath->Length = 0 ;
  
  RtlAppendUnicodeToString (pusPath, (LPCWSTR)0x7FFE0030) ;

  return STATUS_SUCCESS ;
}


/*
NTSTATUS SystInfo_GetSystemRoot (PUNICODE_STRING pusPath) 
{
  NTSTATUS	nStatus ;

  RTL_QUERY_REGISTRY_TABLE aQueryTable[] =
    { 
      { 
	NULL,
	RTL_QUERY_REGISTRY_REQUIRED|RTL_QUERY_REGISTRY_DIRECT,
	L"SystemRoot",
	pusPath
      },
      {
	NULL
      }
    } ;

  nStatus = RtlQueryRegistryValues (RTL_REGISTRY_WINDOWS_NT, L"", 
				    aQueryTable, NULL, NULL) ;

  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("RtlQueryRegistryValues failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  TRACE_INFO (TEXT("\\SystemRoot = %ls\n"), pusPath->Buffer) ;
    
  return STATUS_SUCCESS ;
}
*/


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS SystInfo_GetModuleBase (LPCSTR szModule, PVOID * ppBase, ULONG* pnSize) 
{
  NTSTATUS			nStatus ;
  SYSTEM_MODULE_INFORMATION * pInfo ;
  ULONG		n, nSize ;
 
  TRACE ;

  nStatus = ZwQuerySystemInformation (SystemModuleInformation,
				      &nSize, 0, &nSize) ;

  if( nStatus!=STATUS_INFO_LENGTH_MISMATCH )
    {
      TRACE_ERROR (TEXT("ZwQuerySystemInformation failed (status=0x%08X)\n"), 
		   nStatus) ;
      return nStatus ;
    }

  TRACE_INFO (TEXT("Required size = %u\n"), nSize) ;
 
  pInfo = MALLOC (nSize) ;

  if( pInfo == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate structure SYSTEM_MODULE_INFORMATION (%u bytes)\n"), nSize) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }
  
  nStatus = ZwQuerySystemInformation (SystemModuleInformation,
				      pInfo, nSize, NULL) ;

  if( nStatus!=STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("ZwQuerySystemInformation failed (status=0x%08X)\n"), 
		   nStatus) ;
      return nStatus ;
    }

  nStatus = STATUS_NOT_FOUND ;

  if( szModule != NULL )
    {
      *ppBase = NULL ;
  
      for( n=0 ; n<pInfo->Count ; n++ )
	{
	  LPCSTR szCurModule = pInfo->Module[n].ImageName + pInfo->Module[n].PathLength ;
	  
	  TRACE_INFO (TEXT("Module %hs\n"), szCurModule) ;
	  
	  if( ! _stricmp(szCurModule,szModule) )
	    {
	      TRACE_INFO (TEXT("Found !\n")) ;
	      nStatus = STATUS_SUCCESS ;
	      *ppBase = pInfo->Module[n].Base ;
	      *pnSize = pInfo->Module[n].Size ;
	      break ;
	    }
	}
    }
  else
    {
      *ppBase = pInfo->Module[0].Base ;
      *pnSize = pInfo->Module[0].Size ;
    }
  
  FREE (pInfo) ;

  return nStatus ;
}
