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
#include "ProcList.h"

// standard headers
#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <psapi.h>

// project's headers
#include "Assert.h"
#include "Strlcpy.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define LOCK_TIMEOUT	10 // seconds


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct PROCNODE {
  struct PROCNODE	*pPrev ;
  struct PROCNODE	*pNext ;
  PROCSTRUCT		proc ;
} PROCNODE ;

typedef struct {
  PROCNODE	*pFirst ;
  PROCNODE	*pLast ;
  HANDLE	hMutex ;
} PROCFIFO ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static PROCFIFO g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

DWORD	_ProcList_AddNode (PROCNODE * pNewNode) ;

VOID _ProcList_Clear () ;

BOOL	_ProcList_IsProcessId (DWORD dwProcessId) ;

// get process path from PID
BOOL	_ProcList_GetProcessFileName (DWORD dwProcessId, LPTSTR szFilename, DWORD nSize) ;


//PROCNODE* _ProcList_GetNode (DWORD nId)  ;


/******************************************************************/
/* Exported function : Init                                       */
/******************************************************************/

BOOL	ProcList_Init () 
{
  TRACE ;

  g_data.hMutex = CreateMutex (NULL, FALSE, NULL) ;
  if( ! g_data.hMutex )
    TRACE_ERROR (TEXT("CreateMutex failed (error=%d)\n"), GetLastError()) ;

  g_data.pFirst = NULL ;
  g_data.pLast = NULL ;

  return TRUE ;
}


/******************************************************************/
/* Exported function : Uninit                                     */
/******************************************************************/

VOID ProcList_Uninit () 
{
  TRACE ;

  ProcList_Lock () ;
  ProcList_Clear () ;
  ProcList_Unlock () ;

  CloseHandle (g_data.hMutex) ;
  g_data.hMutex = NULL ;
}


/******************************************************************/
/* Exported function : Clear                                      */
/******************************************************************/

VOID ProcList_Clear ()
{
  PROCNODE	*pNode, *pNext ;
  
  TRACE ;
  
  for( pNode=g_data.pFirst ; pNode ; pNode=pNext )
    {
      pNext = pNode->pNext ;
      free (pNode) ;
    }
}


/******************************************************************/
/* Exported function : acquire exclusive access                   */
/******************************************************************/

VOID	ProcList_Lock () 
{
  DWORD dwResult ;

  TRACE ;

  dwResult = WaitForSingleObject (g_data.hMutex, LOCK_TIMEOUT*1000) ;

  if( dwResult==WAIT_TIMEOUT )
    TRACE_ERROR (TEXT("Time-out while locking process list (%d seconds)\n"), LOCK_TIMEOUT) ;
  else if( dwResult!=WAIT_OBJECT_0 )
    TRACE_BREAK (TEXT("WaitForSingleObject failed (res=%u, error=%u)\n"), dwResult, GetLastError()) ;		       
}


/******************************************************************/
/* Exported function : release exclusive access                   */
/******************************************************************/

VOID	ProcList_Unlock () 
{
  TRACE ;

  if( ! ReleaseMutex (g_data.hMutex) )
    TRACE_ERROR (TEXT("ReleaseMutex failed (error=%u)\n"), GetLastError()) ; 
}


/******************************************************************/
/* Exported function : Add                                        */
/******************************************************************/

BOOL	ProcList_Add (const PROCSTRUCT * pProc) 
{
  PROCNODE * pNewNode ;

  TRACE_INFO ("%s (%d)\n", pProc->szName, pProc->nProcessId) ;

  pNewNode = malloc (sizeof(PROCNODE)) ;
  if( ! pNewNode ) {
    TRACE_ERROR (TEXT("malloc failed\n")) ;
    return FALSE ;
  }

  memcpy (&pNewNode->proc, pProc, sizeof(PROCSTRUCT)) ;
  
  return _ProcList_AddNode (pNewNode) ;
}


/******************************************************************/
/* Exported function : Enum                                       */
/******************************************************************/

VOID	ProcList_Enum (ENUMPROCCALLBACK pfnCallback, void * pContext) 
{
  PROCNODE * pNode ;

  TRACE ;
  
  for( pNode=g_data.pFirst ; pNode ; pNode=pNode->pNext )
    if( ! pfnCallback(pContext,&pNode->proc) )
      break ;
}


/******************************************************************/
/* Internal functio                                               */
/******************************************************************/

DWORD _ProcList_AddNode (PROCNODE * pNode)
{
  TRACE ;

  // verify linked list 
  ASSERT (!g_data.pFirst==!g_data.pLast) ;

  pNode->pPrev	= g_data.pLast ;
  pNode->pNext	= NULL ;

  if( g_data.pLast )
    g_data.pLast->pNext = pNode ;
  g_data.pLast = pNode ;

  if( ! g_data.pFirst )
    g_data.pFirst = pNode ;

  return TRUE ;
}


/******************************************************************/
/* Internal function : IsProcessId                                */
/******************************************************************/

BOOL	_ProcList_IsProcessId (DWORD dwProcessId)
{
  HANDLE	hProcess ;

  hProcess = OpenProcess (0, FALSE, dwProcessId) ;

  if( ! hProcess )
    return ERROR_ACCESS_DENIED==GetLastError() ;
  
  CloseHandle (hProcess) ;
  
  return TRUE ;
}


/******************************************************************/
/* Internal function : GetProcessFileName                         */
/******************************************************************/

BOOL _ProcList_GetProcessFileName (DWORD dwProcessId, LPTSTR szFilename, DWORD nSize) 
{
  HANDLE	hProcess ;
  BOOL		bSuccess ;
  DWORD		dwAccess = PROCESS_QUERY_INFORMATION|PROCESS_VM_READ ;

  ASSERT (szFilename!=NULL) ;
  ASSERT (nSize>0) ;

  if( dwProcessId==0 )
    return _tcslcpy (szFilename, TEXT("System"), nSize) ;
    
  hProcess = OpenProcess (dwAccess,FALSE,dwProcessId) ;

  if( ! hProcess ) {
    TRACE_ERROR (TEXT("OpenProcess failed (error=%d)\n"), GetLastError()) ;
    return FALSE ;
  }

  szFilename[0] = 0 ;
  bSuccess = GetModuleFileNameEx (hProcess, NULL, szFilename, nSize) ;

  if( ! bSuccess )
    TRACE_ERROR (TEXT("GetModuleFileNameEx failed (pid=%u, error=%d)\n"), 
		 dwProcessId, GetLastError()) ;

  CloseHandle (hProcess) ;

  return bSuccess ;
}


/******************************************************************/
/* Exported function : Remove                                     */
/******************************************************************/

VOID ProcList_Remove (PROCADDR nProcessAddress)
{
  PROCNODE	*pCur, *pPrev, *pNext ;

  TRACE ;
  
  for( pCur=g_data.pFirst ; pCur ; pCur=pNext )
    {
      pPrev = pCur->pPrev ;
      pNext = pCur->pNext ;

      if( pCur->proc.nProcessAddress==nProcessAddress )
	{
	  free (pCur) ;

	  if( pPrev!=NULL )
	    pPrev->pNext = pNext ;
	  else
	    g_data.pFirst = pNext ;

	  if( pNext!=NULL )
	    pNext->pPrev = pPrev ;
	  else
	    g_data.pLast = pPrev ;
	  
	  pCur = pPrev ;
	}

      pPrev = pCur ;
    }
}


/******************************************************************/
/* Exported function : Get                                        */
/******************************************************************/

PROCSTRUCT * ProcList_Get (PROCADDR nProcessAddress)
{
  PROCNODE		*pCur ;

  TRACE ;
  
  for( pCur=g_data.pFirst ; pCur ; pCur=pCur->pNext )
    if( pCur->proc.nProcessAddress == nProcessAddress )
      break ;
    
  if( ! pCur ) 
    {
      TRACE_ERROR (TEXT("Failed to find process 0x%08X\n"), nProcessAddress) ;
      return NULL ;
    }

  return &pCur->proc ;
}


/******************************************************************/
/* Exported function : GetState                                   */
/******************************************************************/

DWORD ProcList_GetState (PROCADDR nProcessAddress)
{
  PROCSTRUCT	*pProc ;
  DWORD		nState ;

  TRACE ;

  pProc = ProcList_Get (nProcessAddress) ;  

  if( ! pProc ) nState = PS_UNKNOWN_STATE ; 
  else nState = pProc->nState ;

  return nState ;
}


/******************************************************************/
/* Exported function : SetState                                   */
/******************************************************************/

VOID ProcList_SetState (PROCADDR nProcessAddress, DWORD nState)
{
  PROCSTRUCT	*pProc ;

  TRACE ;

  pProc = ProcList_Get (nProcessAddress) ;  

  if( pProc ) pProc->nState = nState ;
}

