/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2005  Benoit Blanchon                      */
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

#define TRACE_LEVEL	2 /* = warnings */


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "EventLog.h"

// standard headers
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>

// project's headers
#include "Assert.h"
#include "LogFile.h"
#include "FiltRule.h"
#include "FilterTools.h"
#include "Strlcpy.h"
#include "Trace.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct EVENTNODE {
  struct EVENTNODE	*pNext ;
  DWORD			nId ;
  DWORD			nRefCount ;
  EVENTSTRUCT		event ;
} EVENTNODE ;

typedef struct {
  EVENTNODE	*pFirst ;
  EVENTNODE	*pLast ;
  DWORD		nBeginId ;
  DWORD		nEndId ;
  DWORD		nMaxNodes ;
  HANDLE	hMutex ;
} EVENTFIFO ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static EVENTFIFO g_fifo ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/
	
VOID	_EventLog_Lock () ;

VOID	_EventLog_Unlock () ; 

DWORD	_EventLog_Push (EVENTNODE * pNewNode) ;

BOOL	_EventLog_Pop (BOOL bForce) ;

EVENTNODE* _EventLog_GetNode (DWORD nId)  ;



/******************************************************************/
/* Exported function : Init                                       */
/******************************************************************/

BOOL	EventLog_Init () 
{
  TRACE ;

  g_fifo.hMutex = CreateMutex (NULL, FALSE, NULL) ;
  if( ! g_fifo.hMutex )
    TRACE_ERROR (TEXT("CreateMutex failed (error=%d)\n"), GetLastError()) ;

  g_fifo.pFirst = NULL ;
  g_fifo.pLast = NULL ;
  g_fifo.nBeginId = 0 ;
  g_fifo.nEndId = 0 ;
    
  EventLog_ReloadConfig () ;

  return TRUE ;
}


/******************************************************************/
/* Exported function : Uninit                                     */
/******************************************************************/

VOID EventLog_Uninit () 
{
  TRACE ;

  while( _EventLog_Pop (TRUE) ) ;

  CloseHandle (g_fifo.hMutex) ;
  g_fifo.hMutex = NULL ;
}


/******************************************************************/
/* Exported function : ReleadConfig                               */
/******************************************************************/

VOID	EventLog_ReloadConfig () 
{
  TRACE ;

  _EventLog_Lock () ;
  g_fifo.nMaxNodes = 128 ;
  _EventLog_Unlock () ;
}


/******************************************************************/
/* Exported function : Add                                        */
/******************************************************************/

DWORD	EventLog_Add (DWORD dwProcessId, LPCTSTR szPath,
		      int nReaction, int nVerbosity, 
		      PCFILTCOND pCondition)
{
  EVENTNODE	*pNewNode ;
  LPCTSTR	szExeName ;

  TRACE ;
  
  szExeName = PathFindFileName(szPath) ;

  TRACE ;

  if( nVerbosity!=RULE_SILENT )
    {
      TCHAR	szBuffer[1024] ;
      UINT	n ;
      n = wsprintf (szBuffer, TEXT("%s (%u) : "), 
		    szExeName, dwProcessId) ;
      n += FiltCond_ToString (pCondition, szBuffer+n, 1024-n) ;
      n += wsprintf (szBuffer+n, TEXT(" -> ")) ;
      if( nReaction==RULE_ACCEPT )
	wsprintf (szBuffer+n, TEXT("accepted")) ;
      else if( nReaction==RULE_FEIGN )
	wsprintf (szBuffer+n, TEXT("feigned")) ;
      else if( nReaction==RULE_REJECT )
	wsprintf (szBuffer+n, TEXT("rejected")) ;
      else if( nReaction==RULE_KILLPROCESS )
	wsprintf (szBuffer+n, TEXT("process killed")) ;	
      else
	wsprintf (szBuffer+n, TEXT("unknown reaction")) ;

      TRACE ;	
      LogFile_Print (szBuffer) ;
    }      

  TRACE ;

  pNewNode = malloc (sizeof(EVENTNODE)) ;
  if( ! pNewNode ) {
    TRACE_ERROR (TEXT("malloc failed\n")) ;
    return 0xFFFFFFFF ;
  }

  pNewNode->nRefCount		= 0 ;
  pNewNode->event.dwProcessId	= dwProcessId ;
  _tcslcpy (pNewNode->event.szPath, szPath, MAX_PATH) ;
  _tcslcpy (pNewNode->event.szExeName, szExeName, MAX_PATH) ;
  pNewNode->event.nReaction	= nReaction ;
  pNewNode->event.nVerbosity	= nVerbosity ;
  GetLocalTime (&pNewNode->event.time) ;

  FiltCond_Dup (&pNewNode->event.condition, pCondition) ;

  return _EventLog_Push (pNewNode) ;
}
     


/******************************************************************/
/* Exported function : MapEvent                                   */
/******************************************************************/

EVENTSTRUCT *	EventLog_MapEvent (DWORD nId) 
{
  EVENTNODE	*pNode ;
  EVENTSTRUCT	*pEvent ;

  TRACE_INFO (TEXT("nId = %u\n"), nId) ;

  _EventLog_Lock () ;

  pNode = _EventLog_GetNode (nId) ;

  if( ! pNode ) {
    _EventLog_Unlock () ;
    return NULL ;
  }
   
  pNode->nRefCount++ ;
  pEvent = &pNode->event ;

  _EventLog_Unlock () ;

  return pEvent ;
}


/******************************************************************/
/* Exported function : UnmapEvent                                 */
/******************************************************************/

VOID EventLog_UnmapEvent (DWORD nId) 
{
  EVENTNODE	*pNode ;
  
  TRACE_INFO (TEXT("nId = %u\n"), nId) ;

  _EventLog_Lock () ;

  pNode = _EventLog_GetNode (nId) ;

  if( pNode ) 
    pNode->nRefCount-- ;

  _EventLog_Unlock () ;
}


/******************************************************************/
/* Exported function : GetFirstId                                 */
/******************************************************************/

DWORD	EventLog_GetBeginId () 
{
  TRACE_INFO (TEXT("nResult = %u\n"), g_fifo.nBeginId) ;

  return g_fifo.nBeginId ;
}


/******************************************************************/
/* Exported function : GetLastId                                  */
/******************************************************************/

DWORD	EventLog_GetEndId ()
{
  TRACE_INFO (TEXT("nResult = %u\n"), g_fifo.nEndId) ;

  return g_fifo.nEndId ;
}


/******************************************************************/
/* Internal function : Lock                                       */
/******************************************************************/

VOID _EventLog_Lock ()
{
  DWORD dwResult ;

  TRACE ;

  dwResult = WaitForSingleObject (g_fifo.hMutex, INFINITE) ;

  if( dwResult!=WAIT_OBJECT_0 )
    TRACE_ERROR (TEXT("WaitForSingleObject failed (res=%u, error=%u)\n"),
		  dwResult, GetLastError()) ;		       
}


/******************************************************************/
/* Internal function : Unlock                                     */
/******************************************************************/

VOID _EventLog_Unlock ()
{
  TRACE ;

  if( ! ReleaseMutex (g_fifo.hMutex) )
    TRACE_ERROR (TEXT("ReleaseMutex failed (error=%u)\n"), GetLastError()) ;  
}


/******************************************************************/
/* Internal function : Push                                       */
/******************************************************************/

DWORD _EventLog_Push (EVENTNODE * pNewNode)
{
  TRACE ;
 
  _EventLog_Lock () ;

  pNewNode->pNext = NULL ;
  pNewNode->nId = g_fifo.nEndId++ ;

  if( ! g_fifo.pFirst ) 
    g_fifo.pFirst = pNewNode ;

  if( g_fifo.pLast )
    g_fifo.pLast->pNext = pNewNode ;
  
  g_fifo.pLast = pNewNode ;

  while( g_fifo.nEndId-g_fifo.nBeginId > g_fifo.nMaxNodes )
    if( ! _EventLog_Pop(FALSE) ) break ;

  _EventLog_Unlock () ;

  return pNewNode->nId ;
}


/******************************************************************/
/* Internal function : Pop                                        */
/******************************************************************/

BOOL _EventLog_Pop (BOOL bForce)
{
  EVENTNODE * pNode ;

  TRACE ;

  _EventLog_Lock () ;

  pNode = g_fifo.pFirst ;

  if( ! pNode ) {
    TRACE_INFO (TEXT("List is empty\n")) ;
    _EventLog_Unlock () ;
    return FALSE ;
  }

  if( !bForce && pNode->nRefCount>0 ) {
    TRACE_WARNING (TEXT("Node can't be popped (nRefCount=%d)\n"), pNode->nRefCount) ;
    _EventLog_Unlock () ;
    return FALSE ;
  }
    
  g_fifo.pFirst = pNode->pNext ;
  g_fifo.nBeginId++ ;
   
  if( !pNode->pNext ) {
    TRACE_INFO (TEXT("Remove the last node\n")) ;
    g_fifo.pLast = NULL ;
    ASSERT (g_fifo.pFirst==NULL) ;
    ASSERT (g_fifo.nBeginId==g_fifo.nEndId) ;
  }

  _EventLog_Unlock () ;

  FiltCond_Clear (&pNode->event.condition) ;

  free (pNode) ;

  return TRUE ;
}


/******************************************************************/
/* Internal function : GetNode                                    */
/******************************************************************/

EVENTNODE *_EventLog_GetNode (DWORD nId) 
{
  EVENTNODE	*pNode ;

  TRACE_INFO (TEXT("nId = %u\n"), nId) ;
  
  if( nId<g_fifo.nBeginId || nId>=g_fifo.nEndId )
    {
      TRACE_ERROR (TEXT("Event ID out of range\n")) ;
      return NULL ;
    }

  pNode = g_fifo.pFirst ;
  
  while( pNode->nId!=nId ) 
    {
      pNode = pNode->pNext ;
      ASSERT (pNode!=NULL) ;
      ASSERT (pNode->nId<=nId) ;
    }

  return pNode ;
}


/******************************************************************/
/* Exported function : Clear                                      */
/******************************************************************/

BOOL	EventLog_Clear () 
{
  TRACE ;

  while( _EventLog_Pop (FALSE) ) ;

  return TRUE ;
}
