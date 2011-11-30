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

#define TRACE_LEVEL 2	// warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "FilterTools.h"

// standard headers
#include <tchar.h>
#include <stdio.h>

// project's header
#include "Assert.h"
#include "Language.h"
#include "Strlcpy.h"
#include "Trace.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  LPCTSTR	szName ;
  UINT		nParams ;
  LPCTSTR	aParams[MAX_PARAMS] ;
} REASON_DESC ;


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define arraysize(A) (sizeof(A)/sizeof((A)[0]))


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

static REASON_DESC aReasons[_FILTREASON_COUNT] = {
  { TEXT("Undefined"),		0 }, 
  { TEXT("File::Read"),		1, { TEXT("File name") } },
  { TEXT("File::Write"),	1, { TEXT("File name") } },
  { TEXT("Net::Connect"),	3, { TEXT("Address"), TEXT("Port"), TEXT("Protocol") } },
  { TEXT("Net::Listen"),	3, { TEXT("Address"), TEXT("Port"), TEXT("Protocol") } },
  { TEXT("Net::Send"),		3, { TEXT("Address"), TEXT("Port"), TEXT("Protocol") } },
  { TEXT("Reg::SetValue"),	2, { TEXT("Key path"), TEXT("Value name") } },
  { TEXT("Reg::QueryValue"),	2, { TEXT("Key path"), TEXT("Value name") } },
  { TEXT("Sys::Execute"),	1, { TEXT("Command line") } },
  { TEXT("Sys::KillProcess"),	1, { TEXT("Executable path") } }
} ;


/******************************************************************/
/* Exported function : GetName                                    */
/******************************************************************/

LPCTSTR FiltReason_GetName (UINT nReason) 
{
  if( nReason>=_FILTREASON_COUNT )
    return TEXT("[Invalid reason]") ;

  return aReasons[nReason].szName ;
}


/******************************************************************/
/* Exported function : GetId                                      */
/******************************************************************/

UINT	FiltReason_GetId (LPCTSTR szReason) 
{
  UINT	nId ;

  for( nId=_FILTREASON_COUNT-1 ; nId>0 ; nId-- )
    if( ! _tcsicmp(aReasons[nId].szName, szReason) )
      break ;

  return nId ;
}


/******************************************************************/
/* Exported function : GetParamCount                              */
/******************************************************************/

UINT FiltReason_GetParamCount (UINT nReason) 
{
  // verify params
  ASSERT (nReason<_FILTREASON_COUNT) ;

  // verify result 
  ASSERT (aReasons[nReason].nParams<MAX_PARAMS) ;

  return aReasons[nReason].nParams ;
}


/******************************************************************/
/* Exported function : GetParamName                               */
/******************************************************************/

LPCTSTR FiltReason_GetParamName (UINT nReason, UINT nParam) 
{
  // verify params
  ASSERT (nReason<_FILTREASON_COUNT) ;
  ASSERT (nParam<aReasons[nReason].nParams) ;

  return aReasons[nReason].aParams[nParam] ;
}


/******************************************************************/
/* Exported function : GetOptionMask                              */
/******************************************************************/

UINT FiltReason_GetOptionMask (UINT nReason) 
{
  // verify params
  ASSERT (nReason<_FILTREASON_COUNT) ;
  
  if( nReason==FILTREASON_SYS_EXECUTE || nReason==FILTREASON_FILE_READ )
    return RULE_ASK | RULE_SCAN ;
  
  return RULE_ASK ;
}




/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

UINT FiltCond_GetParamCount (PCFILTCOND pCond) 
{
  // verify params
  ASSERT (pCond!=NULL) ;
  
  return pCond->nParams ;
}


/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

LPCTSTR FiltCond_GetParamString (PCFILTCOND pCond, UINT iParam) 
{
  PCFILTPARAM pParam ;

  // verify params
  ASSERT (pCond!=NULL) ;
  ASSERT (iParam>=0) ;
  ASSERT (iParam<pCond->nParams) ;
    
  pParam = &pCond->aParams[iParam] ;

  ASSERT (pParam->nType==FILTPARAM_STRING || pParam->nType==FILTPARAM_WILDCARDS || pParam->nType==FILTPARAM_PATH) ;

  return pParam->szValue ;
}


/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

UINT FiltCond_GetParamType (PCFILTCOND pCond, UINT iParam) 
{
  PCFILTPARAM pParam ;

  // verify params
  ASSERT (pCond!=NULL) ;
  ASSERT (iParam>=0) ;
  ASSERT (iParam<pCond->nParams) ;
    
  pParam = &pCond->aParams[iParam] ;

  return pParam->nType ;
}


/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

UINT FiltCond_GetParamUint (PCFILTCOND pCond, UINT iParam) 
{
  PCFILTPARAM pParam ;

  // verify params
  ASSERT (pCond!=NULL) ;
  ASSERT (iParam>=0) ;
  ASSERT (iParam<pCond->nParams) ;
    
  pParam = &pCond->aParams[iParam] ;

  ASSERT (pParam->nType==FILTPARAM_UINT) ;

  return pParam->nValue ;
}


/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

UINT FiltCond_GetReasonAsString (PCFILTCOND pCond, LPTSTR szBuffer, UINT nSize) 
{
  // verify params
  ASSERT (pCond!=NULL) ;
  ASSERT (szBuffer!=NULL) ;

  _tcslcpy (szBuffer, FiltReason_GetName(pCond->nReason), nSize) ;

  return _tcslen(szBuffer) ;
}



/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

UINT FiltCond_GetParamAsString (PCFILTCOND pCond, UINT iParam, LPTSTR szBuffer, UINT nSize) 
{
  PCFILTPARAM pParam ;

  // verify params
  ASSERT (pCond!=NULL) ;

  if( iParam >= pCond->nParams )
    {
      szBuffer[0] = 0 ;
      return 0 ;
    }
    
  pParam = &pCond->aParams[iParam] ;

  switch( pParam->nType )
    {
    case FILTPARAM_ANY:
      _tcslcpy (szBuffer, TEXT("*"), nSize) ;
      break ;

    case FILTPARAM_UINT:
      wsprintf (szBuffer, TEXT("%u"), pParam->nValue) ;
      break ;

    case FILTPARAM_STRING:
    case FILTPARAM_WILDCARDS:
    case FILTPARAM_PATH:
      _tcslcpy (szBuffer, pParam->szValue, nSize) ;
      break ;

    default:
      _tcslcpy (szBuffer, TEXT("[Invalid param type]"), nSize) ;
    }

  TRACE_INFO (TEXT("Param %d : %s\n"), iParam, szBuffer) ;

  return _tcslen(szBuffer) ;
}


/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

UINT FiltCond_ToString (PCFILTCOND pCond, LPTSTR szBuffer, UINT nSize) 
{
  UINT i, n ;

  n = FiltCond_GetReasonAsString (pCond, szBuffer, nSize) ;

  _tcsncat (szBuffer+n, TEXT(" ("), nSize-n) ;
  n += 2 ;
  
  for( i=0 ; i<pCond->nParams ; i++ )
    {
      n += FiltCond_GetParamAsString (pCond, i, szBuffer+n, nSize-n) ;

      if( i+1<pCond->nParams ) {
	_tcsncat (szBuffer+n, TEXT(", "), nSize-n) ;
	n += 2 ;
      }
    }

  _tcsncat (szBuffer+n, TEXT(")"), nSize-n) ;
  n += 1 ;

  return n ;
}
  


UINT FiltRule_GetReactionString (PCFILTRULE pRule, LPTSTR szOutput, UINT nOutputMax) 
{
  LPCTSTR szResult = NULL ;

  if( pRule->nOptions & RULE_ASK )
    {
      switch( pRule->nReaction )
	{
	case RULE_ACCEPT:
	  szResult = STR_DEF (_ASK_DEFAULT_ACCEPT, TEXT("Ask (default Accept)")) ;
	  break ;
	case RULE_FEIGN:
	  szResult = STR_DEF (_ASK_DEFAULT_FEIGN, TEXT("Ask (default Feign)")) ;
	  break ;
	case RULE_REJECT:
	  szResult = STR_DEF (_ASK_DEFAULT_REJECT, TEXT("Ask (default Reject)")) ;
	  break ;
	case RULE_KILLPROCESS:
	  szResult = STR_DEF (_ASK_DEFAULT_KILL_PROCESS, TEXT("Ask (default Kill process)")) ;
	  break ;
	default:
	  ASSERT(0) ;
	}
    }
  else
    {
      switch( pRule->nReaction )
	{
	case RULE_ACCEPT:
	  szResult = STR_DEF (_ACCEPT, TEXT("Accept")) ;
	  break ;
	case RULE_FEIGN:
	  szResult = STR_DEF (_FEIGN, TEXT("Feign")) ;
	  break ;
	case RULE_REJECT:
	  szResult = STR_DEF (_REJECT, TEXT("Reject")) ;
	  break ;
	case RULE_KILLPROCESS:
	  szResult = STR_DEF (_KILL_PROCESS, TEXT("Kill process")) ;
	  break ;
	default:
	  ASSERT(0) ;
	}      
    }

  ASSERT (szResult!=NULL) ;
  
  return _tcslcpy (szOutput, szResult, nOutputMax) ;
}


UINT FiltRule_GetVerbosityString (PCFILTRULE pRule, LPTSTR szOutput, UINT nOutputMax) 
{
  LPCTSTR szResult = NULL ;

  switch( pRule->nVerbosity )
    {
    case RULE_SILENT:
      szResult = STR_DEF (_SILENT, TEXT("Silent")) ;
      break ;      
    case RULE_LOG:
      szResult = STR_DEF (_LOG, TEXT("Log")) ;
      break ;
    case RULE_ALERT:
      szResult = STR_DEF (_ALERT, TEXT("Alert")) ;
      break ;
    default:
      ASSERT (0) ;
    }

  ASSERT (szResult!=NULL) ;

  return _tcslcpy (szOutput, szResult, nOutputMax) ;
}


UINT FiltRule_GetOptionsString (PCFILTRULE pRule, LPTSTR szOutput, UINT nOutputMax) 
{
  szOutput[0] = 0 ;

  if( pRule->nOptions & RULE_SCAN )
    _tcslcpy (szOutput, 
	      STR_DEF (_VIRUS_SCAN, TEXT("Scan")), 
	      nOutputMax) ;
   
  return _tcslen (szOutput) ;
}

