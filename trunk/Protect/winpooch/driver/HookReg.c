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
#include "HookReg.h"

// standard headers
#include <ctype.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "Hooks.h"
#include "HookCommon.h"
#include "Trace.h"
#include "NtUndoc.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define MAX_KEY_PATH		1024


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

LPWSTR HookReg_ChangePrefix (LPWSTR szPath, LPCWSTR szPrefix, LPCWSTR szNewPrefix)
{
  int nPrefixLen = wcslen(szPrefix) ;

  // this loop replaces _wcsnicmp
  while( --nPrefixLen > 0 )
    {
      int nDiff = szPath[nPrefixLen] - szPrefix[nPrefixLen] ;
      if( nDiff!=0 && nDiff!=32 && nDiff!=-32 )
	return NULL ;
    }

  szPath += wcslen(szPrefix) - wcslen(szNewPrefix) ;
  
  wcsncpy (szPath, szNewPrefix, wcslen(szNewPrefix)) ;
  
  return szPath ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

LPWSTR HookReg_ShortenKeyPath (LPWSTR szKeyPath)
{
  LPWSTR szNewKeyPath ;

  szNewKeyPath = HookReg_ChangePrefix (szKeyPath, 
				       L"\\Registry\\Machine\\",
				       L"HKLM\\") ;
  if( szNewKeyPath ) return szNewKeyPath ;
  
  szNewKeyPath = HookReg_ChangePrefix (szKeyPath, 
				       L"\\Registry\\User\\",
				       L"HKU\\") ;
  if( szNewKeyPath ) return szNewKeyPath ;

  return szKeyPath ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

LONG WINAPI Hook_NtSetValueKey (HANDLE	KeyHandle,
				PUNICODE_STRING	ValueName,
				ULONG	TitleIndex,
				ULONG	Type,
				PVOID	Data,
				ULONG	DataSize) 
{
  {
    BYTE	pBuffer[1024] ;
    ULONG	nSize ;
    NTSTATUS	nResult ;
    
    LPWSTR	szKeyPath ;
    UINT	nReaction ;
    
    TRACE ;

    nResult = ZwQueryKey (KeyHandle, KeyNameInformation, 
			  pBuffer, sizeof(pBuffer)-2, &nSize) ;

    if( nResult==STATUS_SUCCESS )
      {
	szKeyPath = ((KEY_NAME_INFORMATION*)pBuffer)->Name ;
	szKeyPath[((KEY_NAME_INFORMATION*)pBuffer)->NameLength/2] = 0 ;

	szKeyPath = HookReg_ShortenKeyPath (szKeyPath) ;

	TRACE_INFO (TEXT("Key = %ls\n"), szKeyPath) ;
      }
    else szKeyPath = NULL ;
      
    if( szKeyPath )
      {
	if( ValueName!=NULL && ValueName->Buffer!=NULL && ValueName->Length!=0 )
	  HookCommon_CatchCall (&nReaction, NULL,
				FILTREASON_REG_SETVALUE, 
				TEXT("ss"), szKeyPath, ValueName->Buffer) ;
	else
	  HookCommon_CatchCall (&nReaction, NULL,
				FILTREASON_REG_SETVALUE,
				TEXT("ss"), szKeyPath, L"") ;
      }
    else 
      {
	nReaction = RULE_ACCEPT ;    
      }
    
    if( nReaction == RULE_REJECT ) return STATUS_ACCESS_DENIED ; 
    if( nReaction == RULE_FEIGN ) return STATUS_SUCCESS ;     
  }

  JUMP_TO_STUB (HOOKS_NTSETVALUEKEY) ;
  ASSERT (0) ;
  return 0 ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WINAPI Hook_NtQueryValueKey (IN HANDLE  KeyHandle,
				  IN PUNICODE_STRING  ValueName,
				  IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
				  OUT PVOID  KeyValueInformation,
				  IN ULONG  Length,
				  OUT PULONG  ResultLength)
{
  {
    BYTE	pBuffer[1024] ;
    ULONG	nSize ;
    NTSTATUS	nResult ;
    
    LPWSTR	szKeyPath ;
    UINT	nReaction ;
    
    TRACE ;

    nResult = ZwQueryKey (KeyHandle, KeyNameInformation, 
			  pBuffer, sizeof(pBuffer)-2, &nSize) ;

    if( nResult==STATUS_SUCCESS )
      {
	szKeyPath = ((KEY_NAME_INFORMATION*)pBuffer)->Name ;
	szKeyPath[((KEY_NAME_INFORMATION*)pBuffer)->NameLength/2] = 0 ;

	szKeyPath = HookReg_ShortenKeyPath (szKeyPath) ;

	TRACE_INFO (TEXT("Key = %ls\n"), szKeyPath) ;
      }
    else szKeyPath = NULL ;
      
    if( szKeyPath )
      {
	if( ValueName!=NULL && ValueName->Buffer!=NULL && ValueName->Length!=0 )
	  HookCommon_CatchCall (&nReaction, NULL,
				FILTREASON_REG_QUERYVALUE, 
				TEXT("ss"), szKeyPath, ValueName->Buffer) ;
	else
	  HookCommon_CatchCall (&nReaction, NULL,
				FILTREASON_REG_QUERYVALUE,
				TEXT("ss"), szKeyPath, L"") ;
      }
    else 
      {
	nReaction = RULE_ACCEPT ;    
      }
    
    if( nReaction == RULE_REJECT ) return STATUS_ACCESS_DENIED ; 
    if( nReaction == RULE_FEIGN ) return STATUS_SUCCESS ;     
  }

  JUMP_TO_STUB (HOOKS_NTQUERYVALUEKEY) ;
  ASSERT (0) ;
  return 0 ;
}
