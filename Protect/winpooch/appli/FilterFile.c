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

#define TRACE_LEVEL	2	// warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "FilterFile.h"

// standard headers
#include <ctype.h>
#include <stdio.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "FilterTools.h"
#include "ProjectInfo.h"
#include "Strlcpy.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define FF_CURRENT_VERSION	4

#define MAX_VALUE	512
#define MAX_LINE	512
#define MAX_ERROR	128


typedef enum {
  LT_ERROR,
  LT_COMMENT,
  LT_OPENBRACE,
  LT_CLOSEBRACE,
  LT_VERSION,
  LT_APPLICATION,
  LT_PROGRAM,
  LT_PATH,
  LT_HOOK,
  LT_RULE,
  LT_REASON,
  LT_PARAM, 
  LT_REACTION,
  LT_VERBOSITY,
  LT_OPTION,
  _LT_COUNT //< has to stay last
} LINETYPE ;

LPCTSTR aKeyWords[_LT_COUNT] = {
  NULL,
  TEXT("#"),
  TEXT("{"),
  TEXT("}"),
  TEXT("Version"),
  TEXT("Application"),
  TEXT("Program"),
  TEXT("Path"),
  TEXT("Hook"),
  TEXT("Rule"),
  TEXT("Reason"),
  TEXT("Param"),
  TEXT("Reaction"),
  TEXT("Verbosity"),
  TEXT("Option")
} ;

typedef enum {
  ST_ROOT,
  ST_WAITPROGRAM,
  ST_WAITRULE,
  ST_PROGRAM,
  ST_RULE,
} STATE ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

TCHAR	g_szError[MAX_ERROR] ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LINETYPE _FilterFile_ReadLine (LPCTSTR szLine, LPTSTR szValue, int nMaxValue) ;

BOOL _FilterFile_EnumFiltersCallback (LPVOID pContext, HFILTER hFilter) ;

VOID _FilterFile_EnumRulesCallback (LPVOID pContext, PFILTRULE pRule) ;


/******************************************************************/
/* Exported function : GetLastError                               */
/******************************************************************/

LPCTSTR FilterFile_GetErrorString () 
{
  return g_szError ;
}


/******************************************************************/
/* Exported function : Read                                       */
/******************************************************************/

HFILTERSET FilterFile_Read (LPCTSTR szFilename) 
{
  FILE		*fp ;
  TCHAR		szValue[MAX_VALUE] ;
  TCHAR		szLine[MAX_LINE] ;
  int		nLine ;
  LINETYPE	nLineType ;
  STATE		nState = ST_ROOT ;
  UINT		nVersion ;
  HFILTERSET	hFilterSet ;
  HFILTER	hCurFilter ;
  FILTRULE	*pCurRule ;
  FILTCOND	*pCurCond ;
  FILTPARAM	*pCurParam ;
  
  // open file
  fp = _tfopen (szFilename, TEXT("rt")) ;

  // open failed ?
  if( ! fp ) {
    wsprintf (g_szError, TEXT("Can't open file %s"), szFilename) ;
    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
    return NULL ;
  }

  // create filter set
  hFilterSet = FilterSet_Create (128) ;

  // for each line (begin)
  for( nLine=1 ; _fgetts(szLine,MAX_LINE,fp) ; nLine++ )
    {
      // read line in file
      nLineType = _FilterFile_ReadLine (szLine, szValue, MAX_VALUE) ;
      
      // error ?
      if( nLineType==LT_ERROR )
	{
	  wsprintf (g_szError, TEXT("On line %d :\nInvalid line format."), nLine) ;
	  TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	  goto failed ;
	}
      
      // comment ?
      if( nLineType==LT_COMMENT ) continue ;
      
      // waiting for a { ?
      if( (nState==ST_WAITPROGRAM || nState==ST_WAITRULE )
	  && nLineType!=LT_OPENBRACE )
	{
	  wsprintf (g_szError, TEXT("On line %d :\nMissing a '{'."), nLine) ;
	  TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	  goto failed ;	
	}

      switch( nLineType )
	{
	case LT_OPENBRACE:

	  switch( nState )
	    {
	    case ST_WAITPROGRAM:
	      TRACE_INFO (TEXT("Enter in program section\n")) ;
	      nState = ST_PROGRAM ;
	      hCurFilter = Filter_Create (NULL) ;
	      break ;
	      
	    case ST_WAITRULE:
	      
	      TRACE_INFO (TEXT("Enter in rule section\n"))  ;

	      nState = ST_RULE ;

	      pCurRule = (FILTRULE*) calloc (1, sizeof(FILTRULE)) ;
	      
	      // set pointer to the current condition
	      pCurCond = &pCurRule->condition ;

	      break ;

	    default:
	      wsprintf (g_szError, TEXT("On line %d :\nUnexpected '{'."), nLine) ;
	      TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	      goto failed ;	
	    }
	    
	  break ; // case LT_OPENBRACE:

	case LT_CLOSEBRACE:
	  
	  switch( nState )
	    {
	    case ST_PROGRAM:
	      TRACE_INFO (TEXT("Leave program section\n")) ;
	      nState = ST_ROOT ;	      
	      FilterSet_AddFilter (hFilterSet, hCurFilter) ;
	      hCurFilter = NULL ;
	      break ;
	      
	    case ST_RULE:

	      TRACE_INFO (TEXT("Leave rule section\n")) ;

	      if( pCurCond->nReason==FILTREASON_UNDEFINED )	{
		wsprintf (g_szError, TEXT("On line %d :\nNo reason specified."), nLine) ;
		TRACE_ERROR (TEXT("%s\n"), g_szError) ;
		goto failed ;	
	      }
	      
	      nState = ST_PROGRAM ;

	      Filter_AddRule (hCurFilter, pCurRule) ;
	      pCurRule = NULL ;

	      break ;

	    default:
	      wsprintf (g_szError, TEXT("On line %d :\nUnexpected '}'."), nLine) ;
	      TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	      goto failed ;	
	    } 
	  
	  break ; // case LT_CLOSEBRACE:

	case LT_VERSION:

	  nVersion = _ttoi (szValue) ;

	  if( nVersion>FF_CURRENT_VERSION ) {
	    wsprintf (g_szError, TEXT("This file has been created for a newer version.\n"
				      "It can't be read by this program, please update.")) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;		      
	  }
	  
	  TRACE_INFO (TEXT("File version is OK\n")) ;
	  
	  break ; // case LT_VERSION:
	  
	case LT_APPLICATION:
	  
	  break ;

	case LT_PROGRAM:

	  if( nState!=ST_ROOT ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  TRACE_INFO (TEXT("Wait for a program section\n")) ;

	  nState = ST_WAITPROGRAM ;
	  
	  break ; // case LT_PROGRAM:

	case LT_PATH:

	  if( nState!=ST_PROGRAM ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  if( Filter_GetProgram(hCurFilter)[0] ) {
	    wsprintf (g_szError, TEXT("On line %d :\nPath already set to :\n%s."), 
		      nLine, Filter_GetProgram(hCurFilter)) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;   
	  }

	  TRACE_INFO (TEXT("Set path to %s\n"), szValue) ;

	  Filter_SetProgram (hCurFilter, szValue) ;

	  break ; // case LT_PATH:

	case LT_HOOK:

	  if( nState!=ST_PROGRAM ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  if( !_tcsicmp(TEXT("Enabled"),szValue) ) 
	    Filter_EnableHook (hCurFilter, TRUE) ;
	  else if( !_tcsicmp(TEXT("Disabled"),szValue) ) 
	    Filter_EnableHook (hCurFilter, FALSE) ;
	  else {
	    wsprintf (g_szError, TEXT("On line %d :\nUnknown value '%s'."), 
		      nLine, szValue) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  TRACE_INFO (TEXT("Hook set to %s\n"), szValue) ;

	  break ; // case LT_HOOK:

	case LT_RULE:

	  if( nState!=ST_PROGRAM ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  TRACE_INFO (TEXT("Wait for a rule section\n")) ;
	  nState = ST_WAITRULE ;

	  break ; // case LT_RULE:

	case LT_REASON:

	  if( nState!=ST_RULE ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  if( pCurCond->nReason != FILTREASON_UNDEFINED ) {
	    wsprintf (g_szError, TEXT("On line %d :\nReasion already set to : %s."), 
		      nLine, FiltReason_GetName(pCurCond->nReason)) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;   	    
	  }

	  pCurCond->nReason = FiltReason_GetId (szValue) ;

	  if( pCurCond->nReason==FILTREASON_UNDEFINED ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnknown reason : '%s'."), 
		      nLine, szValue) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;   
	  }

	  TRACE_INFO (TEXT("Reason set to %s\n"), szValue) ;

	  break ; // case LT_REASON:	 

	case LT_PARAM:

	  if( nState!=ST_RULE ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  if( pCurCond->nParams>=MAX_PARAMS ) {
	    wsprintf (g_szError, TEXT("On line %d :\nMaximum param count already reached."), 
		      nLine) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;	    
	  }

	  pCurParam = &pCurCond->aParams[pCurCond->nParams++] ;

	  if( !_tcsicmp(TEXT("Any"), szValue) )
	    {	            
	      pCurParam->nType = FILTPARAM_ANY ;	    
	      TRACE_INFO (TEXT("Param set to ANY\n")) ;
	    }
	  else if( !_tcsnicmp(TEXT("Integer:"), szValue, 8) ) 
	    {
	      pCurParam->nType = FILTPARAM_UINT ;
	      pCurParam->nValue = _ttoi(szValue+8) ;
	      TRACE_INFO (TEXT("Param set to UINT : %d\n"), pCurParam->nValue) ;
	    }
	  else if( !_tcsnicmp(TEXT("String:"), szValue, 7) ) 
	    {
	      pCurParam->nType = FILTPARAM_STRING ;
	      pCurParam->szValue = malloc (_tcslen(szValue)*sizeof(TCHAR)) ;
	      _tcscpy (pCurParam->szValue, szValue+7) ;
	      TRACE_INFO (TEXT("Param type set to STRING : %s\n"), pCurParam->szValue) ;
	    }
	  else if( !_tcsnicmp(TEXT("Wildcards:"), szValue, 10) ) 
	    {
	      pCurParam->nType = FILTPARAM_WILDCARDS ;
	      pCurParam->szValue = malloc (_tcslen(szValue)*sizeof(TCHAR)) ;
	      _tcscpy (pCurParam->szValue, szValue+10) ;
	      TRACE_INFO (TEXT("Param type set to WILDCARDS : %s\n"), pCurParam->szValue) ;
	    }
	  else if( !_tcsnicmp(TEXT("Path:"), szValue, 5) ) 
	    {
	      pCurParam->nType = FILTPARAM_PATH ;
	      pCurParam->szValue = malloc (_tcslen(szValue)*sizeof(TCHAR)) ;
	      _tcscpy (pCurParam->szValue, szValue+5) ;
	      TRACE_INFO (TEXT("Param type set to PATH : %s\n"), pCurParam->szValue) ;
	    }
	  else
	    {
	      wsprintf (g_szError, TEXT("On line %d :\nUnknown type for param : '%s'."), 
			nLine, szValue) ;
	      TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	      goto failed ;   	      
	    }

	  pCurParam = NULL ;

	  break ; // case LT_PARAM:

	case LT_REACTION:

	  if( nState!=ST_RULE ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  if( ! _tcsicmp(TEXT("Accept"), szValue) )
	    pCurRule->nReaction = RULE_ACCEPT ;
	  else if( ! _tcsicmp(TEXT("Ask"), szValue) )
	    pCurRule->nOptions |= RULE_ASK ;
	  else if( ! _tcsicmp(TEXT("Feign"), szValue) )
	    pCurRule->nReaction = RULE_FEIGN ;
	  else if( ! _tcsicmp(TEXT("Reject"), szValue) )
	    pCurRule->nReaction = RULE_REJECT ;
	  else if( ! _tcsicmp(TEXT("KillProcess"), szValue) )
	    pCurRule->nReaction = RULE_KILLPROCESS ;
	  else {
	    wsprintf (g_szError, TEXT("On line %d :\nUnknown reaction '%s'."), 
		      nLine, szValue) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;	    
	  } 

	  break ; // case LT_REACTION: 

	case LT_VERBOSITY:
	  
	  if( nState!=ST_RULE ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  } 

	  if( ! _tcsicmp(TEXT("Silent"), szValue) )
	    pCurRule->nVerbosity = RULE_SILENT ;
	  else if( ! _tcsicmp(TEXT("Log"), szValue) )
	    pCurRule->nVerbosity = RULE_LOG ;
	  else if( ! _tcsicmp(TEXT("Alert"), szValue) )
	    pCurRule->nVerbosity = RULE_ALERT ;
	  else {
	    wsprintf (g_szError, TEXT("On line %d :\nUnknown verbosity '%s'."), 
		      nLine, szValue) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  break ; // case LT_VERBOSITY:

	case LT_OPTION:
	  
	  if( nState!=ST_RULE ) {
	    wsprintf (g_szError, TEXT("On line %d :\nUnexpected keyword '%s'."), 
		      nLine, aKeyWords[nLineType]) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  } 

	  if( ! _tcsicmp(TEXT("Ask"), szValue) )
	    pCurRule->nOptions |= RULE_ASK ;
	  else if( ! _tcsicmp(TEXT("Scan"), szValue) )
	    pCurRule->nOptions |= RULE_SCAN ;
	  else {
	    wsprintf (g_szError, TEXT("On line %d :\nUnknown option '%s'."), 
		      nLine, szValue) ;
	    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
	    goto failed ;
	  }

	  break ; // case LT_OPTION:

	default:

	  ASSERT (0) ;
	}
      // switch( nLineType )           
    }
  // for each line (end)

  if( nState!=ST_ROOT ) {
    wsprintf (g_szError, TEXT("On line %d :\nUnexpected end-of-file."), nLine) ;
    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
    goto failed ;     
  }

  free (pCurCond) ;
  
  return hFilterSet ;
  
 failed:
  
  free (pCurRule) ;
  FilterSet_Destroy (hFilterSet) ;

  return NULL ;
}


/******************************************************************/
/* Exported function :                                            */
/******************************************************************/

BOOL		FilterFile_GetFileVersion (LPCTSTR szFilename, 
					   DWORD   *pdwFormatVersion,
					   DWORD   *pdwAppVersion)
{
  FILE		*fp ;
  TCHAR		szValue[MAX_VALUE] ;
  TCHAR		szLine[MAX_LINE] ;
  int		nLine ;
  LINETYPE	nLineType ;
  BOOL		bGoOn ;
  
  if( pdwFormatVersion )
    *pdwFormatVersion = 0 ;
  if( pdwAppVersion )
    *pdwAppVersion = 0 ;

  // open file
  fp = _tfopen (szFilename, TEXT("rt")) ;

  // open failed ?
  if( ! fp ) return FALSE ;
  
  // for each line (begin)
  bGoOn = TRUE ;
  for( nLine=1 ; _fgetts(szLine,MAX_LINE,fp) && bGoOn ; nLine++ )
    {
      nLineType = _FilterFile_ReadLine (szLine, szValue, MAX_VALUE) ;
      
      switch( nLineType )
	{
	case LT_COMMENT:
	  break ;

	case LT_VERSION:

	  if( pdwFormatVersion )
	    *pdwFormatVersion = _ttoi (szValue) ;
	  
	  break ; // case LT_VERSION:
	  
	case LT_APPLICATION:

	  if( pdwAppVersion )
	  {
	    int nHigh, nMed, nLow ;
	    _stscanf (szValue, TEXT("%d.%d.%d"), 
		     &nHigh, &nMed, &nLow) ;
	    *pdwAppVersion = (nHigh<<16) | (nMed<<8) | nLow ;	
	  }

	  break ;

	default:
	  bGoOn = FALSE ;
	}
    }

  fclose (fp) ;
  
  return TRUE ;
}



/******************************************************************/
/* Internal function : ReadLine                                   */
/******************************************************************/

LINETYPE _FilterFile_ReadLine (LPCTSTR szLine, LPTSTR szValue, int nMaxValue)  
{
  int i = 0 ;
  int iStart ;
  int iEnd ;
  int iType ;

  TRACE_INFO ("%s", szLine) ;
  
  // skip spaces
  while( _istspace(szLine[i]) ) i++ ;
  
  // is the line empty ?
  if( ! szLine[i] ) return LT_COMMENT ;
  
  // is it a comment ?
  if( szLine[i]=='#' ) return LT_COMMENT ;
  
  // is it a brace 
  if( szLine[i]=='{' || szLine[i]=='}' ) 
    {
      // openning or closing brace ?
      iType = szLine[i]=='{' ? LT_OPENBRACE : LT_CLOSEBRACE ;
      
      // skip spaces
      i++ ;
      while( _istspace(szLine[i]) ) i++ ;
      
      // it should be the end of line
      return szLine[i] ? LT_ERROR : iType ;
    }	
  
  // save start of first word
  iStart = i ;
  
  // eat each non-space characters
  while( szLine[i] && !_istspace(szLine[i]) ) i++ ;
  
  // save end of the first word
  iEnd = i ;
  
  // try to recognize the first word
  for( iType=_LT_COUNT-1 ; iType>0 ; iType-- )
    if( ! _tcsnicmp(aKeyWords[iType], szLine+iStart, iEnd-iStart) )
      break ;
  
  // Keyword recognized ?
  if( !iType )
    return LT_ERROR ;
  
  // skip spaces
  while( _istspace(szLine[i]) ) i++ ;

  // save start of value
  iStart = i ;
  
  // eat each char which is not a \n
  while( szLine[i] && szLine[i]!='\r' && szLine[i]!='\n' ) i++ ;
  
  // save end of value
  iEnd = i ;
  
  // copy value into buffer
  _tcslcpy (szValue, szLine+iStart, nMaxValue) ;
  szValue[iEnd-iStart] = 0 ;

  TRACE_INFO (TEXT("%d : %s\n"), iType, szValue) ;
  
  return iType ;
}


/******************************************************************/
/* Exported function : Write                                      */
/******************************************************************/

BOOL FilterFile_Write (LPCTSTR szFilename, HFILTERSET hFilterSet) 
{
  FILE		*fp ;
 
  // open file
  fp = _tfopen (szFilename, TEXT("wt")) ;
  
  // open failed ?
  if( ! fp ) {
    wsprintf (g_szError, TEXT("Can't open file %s"), szFilename) ;
    TRACE_ERROR (TEXT("%s\n"), g_szError) ;
    return FALSE ;
  }

  // print comment header
  _ftprintf (fp, 
	     TEXT("#\n")
	     TEXT("# Winpooch filtering rules\n")
	     TEXT("#\n")
	     TEXT("# This file has been automatically generated.\n")
	     TEXT("# You may not modify it.\n")
	     TEXT("# This file format may change in next versions.\n")
	     TEXT("#\n")
	     TEXT("\n\n")) ;
	     
  // print file format version
  _ftprintf (fp, 
	     TEXT("# File format version\n")
	     TEXT("Version\t%d\n")
	     TEXT("\n"), 
	     FF_CURRENT_VERSION) ;

  // print application version
  _ftprintf (fp, 
	     TEXT("# Application version\n")
	     TEXT("Application\t%s\n")
	     TEXT("\n"), 
	     TEXT(APPLICATION_VERSION_STRING)) ;

  // write program sections
  FilterSet_EnumFilters (hFilterSet, _FilterFile_EnumFiltersCallback, fp) ;

  fclose (fp) ;

  return TRUE ;
}


/******************************************************************/
/* Internal function : EnumFiltersCallback                        */
/******************************************************************/

BOOL _FilterFile_EnumFiltersCallback (LPVOID pContext, HFILTER hFilter)
{
  FILE * fp = pContext ;

  // verify params
  ASSERT (pContext!=NULL) ;
  ASSERT (hFilter!=NULL) ;

  // print program section head
  _ftprintf (fp, TEXT("Program\n{\n")) ;
  
  // print path
  _ftprintf (fp, TEXT("\tPath\t%s\n\n"),
	     Filter_GetProgram(hFilter)) ;
  
  // print hook enable/disabled
  _ftprintf (fp, TEXT("\tHook\t%s\n\n"),
	     Filter_IsHookEnabled(hFilter) ?
	     TEXT("Enabled") : TEXT("Disabled")) ;

  Filter_EnumRules (hFilter, _FilterFile_EnumRulesCallback, fp) ;
  
  // print program section foot
  _ftprintf (fp, TEXT("}\n\n")) ;

  return TRUE ;
}	 


VOID _FilterFile_EnumRulesCallback (LPVOID pContext, FILTRULE *pRule)
{
  FILE		*fp = pContext ;
  FILTCOND	*pCond ;
  FILTPARAM	*pParam ;
  int		iParam ;

  // verify params
  ASSERT (pContext!=NULL) ;
  ASSERT (pRule!=NULL) ;

  // print rule section head
  _ftprintf (fp, TEXT("\tRule\n\t{\n")) ;
  
  pCond = &pRule->condition ;

  // print reason
  _ftprintf (fp, TEXT("\t\tReason\t\t%s\n\n"),
	     FiltReason_GetName(pCond->nReason)) ;

  // print reaction
  switch( pRule->nReaction )
    {
    case RULE_ACCEPT:
      _ftprintf (fp, TEXT("\t\tReaction\tAccept\n\n")) ;
      break ;
    case RULE_FEIGN:
      _ftprintf (fp, TEXT("\t\tReaction\tFeign\n\n")) ;
      break ;   
    case RULE_REJECT:
      _ftprintf (fp, TEXT("\t\tReaction\tReject\n\n")) ;
      break ;
    case RULE_KILLPROCESS:
      _ftprintf (fp, TEXT("\t\tReaction\tKillProcess\n\n")) ;
      break ; 
    default:
      ASSERT (0) ;
    }

  // print verbosity
  switch( pRule->nVerbosity ) 
    {
    case RULE_SILENT:
      _ftprintf (fp, TEXT("\t\tVerbosity\tSilent\n\n")) ;
      break ;
    case RULE_LOG:
      _ftprintf (fp, TEXT("\t\tVerbosity\tLog\n\n")) ;
      break ;
    case RULE_ALERT:
      _ftprintf (fp, TEXT("\t\tVerbosity\tAlert\n\n")) ;
      break ;
      ASSERT (0) ;
    }

  // print options
  if( pRule->nOptions & RULE_ASK )
    _ftprintf (fp, TEXT("\t\tOption\tAsk\n\n")) ;
  if( pRule->nOptions & RULE_SCAN )
    _ftprintf (fp, TEXT("\t\tOption\tScan\n\n")) ; 

  // add params
  for( iParam=0 ; iParam<pCond->nParams ; iParam++ )
    {
      pParam = &pCond->aParams[iParam] ;
      
      _ftprintf (fp, TEXT("\t\t# Param %d/%d : %s\n"),
		 iParam+1, 
		 FiltReason_GetParamCount(pCond->nReason),
		 FiltReason_GetParamName(pCond->nReason,iParam)) ;

      _ftprintf (fp, TEXT("\t\tParam\t\t")) ;

      switch( pParam->nType )
	{
	case FILTPARAM_ANY:
	  _ftprintf (fp, TEXT("Any")) ;
	  break ;

	case FILTPARAM_UINT:
	  _ftprintf (fp, TEXT("Integer:%u"), pParam->nValue) ;
	  break ;

	case FILTPARAM_STRING:
	  _ftprintf (fp, TEXT("String:%s"), pParam->szValue) ;
	  break ;

	case FILTPARAM_WILDCARDS:
	  _ftprintf (fp, TEXT("Wildcards:%s"), pParam->szValue) ;
	  break ;

	case FILTPARAM_PATH:
	  _ftprintf (fp, TEXT("Path:%s"), pParam->szValue) ;
	  break ;

	default:
	  ASSERT (0) ;
	}

      _ftprintf (fp, TEXT("\n\n")) ;
    }

  // print program section foot
  _ftprintf (fp, TEXT("\t}\n\n")) ;
}
