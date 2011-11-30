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
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "StringList.h"

// project's headers
#include "Assert.h"

// standard headers
#include <ctype.h>
#include <stdio.h>
#include <tchar.h>


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define LINE_BUFFER_LEN		1024
#define INITIAL_CLASS_COUNT	32
#define INITIAL_STRING_COUNT	32
#define MAX_STRING_COUNT        64
#define MAX_CLASS_COUNT		64

LPCTSTR szInvalidString = TEXT("--") ;


/******************************************************************/
/* Internal data structures                                       */
/******************************************************************/

typedef struct {
  DWORD		nStrings ;
  LPCTSTR	*pStrings ;
} STRINGCLASS ;

typedef struct {
  DWORD		nClasses ;
  STRINGCLASS	*pClasses ;
} STRINGLIST ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

void _StringList_GetAbsolutePath (LPTSTR szPath, LPCTSTR szFile) ;
int _StringList_ExpandString (LPTSTR szDst, DWORD nDstSize, LPCTSTR szSrc) ;
BOOL _StringList_ParseLine (LPTSTR szLine, DWORD * pdwKey, LPTSTR * pszString) ;



/******************************************************************/
/* Internal function : GetAbsolutePath                            */
/******************************************************************/

void _StringList_GetAbsolutePath (LPTSTR szPath, LPCTSTR szFile)
{
  TCHAR *p ;

  GetModuleFileName (NULL, szPath, MAX_PATH) ;
  
  p = _tcsrchr (szPath, TEXT('\\')) ;  
  ASSERT (p!=NULL) ;
  _tcscpy (p+1, szFile) ;
}


/******************************************************************/
/* Exported function : StringList_Load                            */
/******************************************************************/

HSTRINGLIST StringList_Load (LPCTSTR szFile)
{
  STRINGLIST	*pStringList ;
  TCHAR		szPath[MAX_PATH] ;
  TCHAR		szLine[LINE_BUFFER_LEN] ;
  DWORD		nLineNumber ;
  DWORD		dwCurKey ;
  LPTSTR	szCurString ;
  FILE		*fp ;
  int		i ;
  
  ASSERT (szFile) ;

  fp = _tfopen (szFile, TEXT("rb")) ;
  if( ! fp ) {
    _StringList_GetAbsolutePath (szPath, szFile) ;
    fp = _tfopen (szPath, TEXT("rb")) ; 
    if( ! fp )
      return NULL ;
  }

  pStringList = malloc (sizeof(STRINGLIST)) ;
  pStringList->nClasses = INITIAL_CLASS_COUNT ;
  pStringList->pClasses = calloc (pStringList->nClasses, 
				  sizeof(STRINGCLASS)) ;

  // initialize strings
  for( i=0 ; i<INITIAL_CLASS_COUNT ; i++ )
    {
      pStringList->pClasses[i].pStrings = calloc(INITIAL_STRING_COUNT,
						 sizeof(LPCTSTR)) ;
      pStringList->pClasses[i].nStrings = INITIAL_STRING_COUNT ;
    }

  // skip the first char 
  // it's a dummy char to tell is the file is little or big endian
  // TODO : verify that value
  _fgettc (fp) ; 

  while( _fgetts (szLine, LINE_BUFFER_LEN, fp) )
    {
      nLineNumber++ ;
      
      if( _StringList_ParseLine(szLine,&dwCurKey,&szCurString) ) 
	StringList_AddString (pStringList, dwCurKey, szCurString) ;
    }

  fclose (fp) ;

  return pStringList ;
}


/******************************************************************/
/* Exported function : GetFromFile                                */
/******************************************************************/

BOOL StringList_GetFromFile (LPCTSTR szFile, DWORD dwKey, LPTSTR szString, int nMaxChars)
{
  TCHAR		szPath[MAX_PATH] ;
  TCHAR		szLine[LINE_BUFFER_LEN] ;
  FILE		*fp ;
  BOOL		bFound = FALSE ;

  DWORD		dwCurKey ;
  LPTSTR	szCurString ;
  
  ASSERT (szFile) ;

  fp = _tfopen (szFile, TEXT("rb")) ;
  if( ! fp ) {
    _StringList_GetAbsolutePath (szPath, szFile) ;
    fp = _tfopen (szPath, TEXT("rb")) ; 
    if( ! fp )
      return FALSE ;
  }

  // skip the first char 
  // it's a dummy char to tell is the file is little or big endian
  // TODO : verify that value
  _fgettc (fp) ; 

  while( _fgetts (szLine, LINE_BUFFER_LEN, fp) )
    {      
      if( _StringList_ParseLine(szLine,&dwCurKey,&szCurString) )
	{
	  if( dwCurKey == dwKey )
	    {
	      _tcsncpy (szString, szCurString, nMaxChars) ;
	      bFound = TRUE ;
	      break ;
	    }	
	}   
    }

  fclose (fp) ;

  return bFound ;
}


/******************************************************************/
/* Internal function : ParseLine                                  */
/******************************************************************/

BOOL _StringList_ParseLine (LPTSTR szLine, DWORD * pdwKey, LPTSTR * pszString) 
{
  DWORD		iKeyStart, iKeyEnd ;
  DWORD		iStringStart, iStringEnd ;
  int		i ;

  // go  to the beginning of the line
  i = 0 ;
  
  // skip spaces 
  while( szLine[i] && _istspace(szLine[i]) ) i++ ;
  
  // comment or empty line => goto next line
  if( szLine[i]==TEXT('\n') || szLine[i]==TEXT('\r') || szLine[i]==0 || szLine[i]==TEXT('#') ) 
    return FALSE ;
  
  // seems to be a key ? else goto next line
  if( ! _istxdigit(szLine[i]) ) 
    return FALSE ;
  
  // find key start and end 
  iKeyStart = i ;
  while( _istxdigit(szLine[i]) ) i++ ;
  iKeyEnd = i ;
  
  // skip spaces 
  while( szLine[i]!=0 && szLine[i]!=TEXT('\n') && szLine[i]!=TEXT('\r') && _istspace(szLine[i]) ) 
    i++ ;
  
  // find string start and end
  iStringStart = i ;
  while( szLine[i] && szLine[i]!=TEXT('\n') && szLine[i]!=TEXT('\r') ) i++ ;
  iStringEnd = i ;
  
  // convert key string to integer
  szLine[iKeyEnd] = 0 ;
  *pdwKey = _tcstoul (szLine+iKeyStart, NULL, 16) ;
  
  // copy value string
  szLine[iStringEnd] = 0 ;

  *pszString = szLine+iStringStart ;

  return TRUE ;
}


/******************************************************************/
/* Exported function : AddString                                  */
/******************************************************************/

BOOL StringList_AddString (HSTRINGLIST h, DWORD dwKey, LPCTSTR szString)
{
  STRINGLIST	*p = (STRINGLIST*)h ;
  WORD		wClass ;
  WORD		wOffset ;
  STRINGCLASS	*pClass ;
  DWORD		nMaxLen ;

  ASSERT (p!=NULL) ;

  // MessageBox (NULL, szString, TEXT("Addstring"), 0) ;

  wClass = dwKey >> 8 ;
  wOffset = dwKey & 0xFF ;

  if( wClass >= MAX_CLASS_COUNT )
    return FALSE ;

  if( wClass >= p->nClasses ) 
    {
      DWORD nNewClasses = wClass + 4 ;

      p->pClasses = realloc (p->pClasses, nNewClasses*sizeof(STRINGCLASS)) ;

      while( p->nClasses < nNewClasses ) {
	p->pClasses[p->nClasses].pStrings = calloc(INITIAL_STRING_COUNT,
						   sizeof(LPCTSTR)) ;
	p->pClasses[p->nClasses].nStrings = INITIAL_STRING_COUNT ;
	p->nClasses++ ;
      }
    }

  if( wOffset >= MAX_STRING_COUNT )
    return FALSE ;

  pClass = &p->pClasses[wClass] ;

  if( wOffset >= pClass->nStrings )
    {
      DWORD	nNewStrings = wOffset + 16 ;
      LPCTSTR	*pNewStrings ;

      pNewStrings = realloc (pClass->pStrings, nNewStrings*sizeof(LPCTSTR)) ;
      if( ! pNewStrings ) return FALSE ;

      pClass->pStrings = pNewStrings ;

      while( pClass->nStrings < nNewStrings ) 
	pClass->pStrings[pClass->nStrings++] = NULL ;
    }

  // free existing string
  free ((void*)pClass->pStrings[wOffset]) ;

  // alloc string
  nMaxLen = _tcslen(szString) + 1 ;
  pClass->pStrings[wOffset] = malloc (nMaxLen*sizeof(TCHAR)) ;

  // copy string
  _StringList_ExpandString ((LPTSTR)pClass->pStrings[wOffset], nMaxLen, szString) ;
  
  return TRUE ;
}


/******************************************************************/
/* Internal Function : _StringList_Expand                         */
/******************************************************************/

int _StringList_ExpandString (LPTSTR szDst, DWORD nDstSize, LPCTSTR szSrc)
{
  DWORD		iSrc, iDst ;
  TCHAR		c ;
  
  iSrc = 0 ;
  iDst = 0 ;
  
  while( (c=szSrc[iSrc++])!=0 && iDst<nDstSize-1 )
    {
      if( c==TEXT('\\') )
	{
	  switch( szSrc[iSrc++] )
	    {
	    case 'n':
	      szDst[iDst++] = TEXT('\n') ;
	      break ;
	    case '\\':
	      szDst[iDst++] = TEXT('\\') ;
	      break ;
	    default:
	      szDst[iDst++] = TEXT('\\') ;
	      szDst[iDst++] = szSrc[iSrc] ;
	    }
	}
      else szDst[iDst++] = c ;
    }

  szDst[iDst++] = 0 ;

  return iDst ;
}




/******************************************************************/
/* Exported function : StringList_Free                            */
/******************************************************************/

VOID StringList_Free (HSTRINGLIST h) 
{
  STRINGLIST	*p = (STRINGLIST*)h ;
  STRINGCLASS	*pClass ;
  int		iClass, iOffset ;
  
  if( p )
    {
      if( p->pClasses )
	{
      
	  for( iClass=0 ; iClass<p->nClasses ; iClass++ )
	    {
	      pClass = &p->pClasses[iClass] ;
      
	      for( iOffset=0 ; iOffset<pClass->nStrings ; iOffset++ ) 		
		free ((void*)pClass->pStrings[iOffset]) ;
	      	      
	      free (pClass->pStrings) ;
	    }

	  free (p->pClasses) ;
	}
      
      free (p) ;
    }
}


/******************************************************************/
/* Exported function : StringList_Get                             */
/******************************************************************/

LPCTSTR StringList_Get (HSTRINGLIST h, DWORD dwKey) 
{
  STRINGLIST	*p = (STRINGLIST*)h ;
  WORD		wClass, wOffset ;

  ASSERT (p!=NULL) ;

  wClass = dwKey >> 8 ;
  wOffset = dwKey & 0xFF ;

  if( wClass >= p->nClasses )
    return szInvalidString ;

  if( wOffset >= p->pClasses[wClass].nStrings )
    return szInvalidString ;
  
  if( ( wClass >= p->nClasses ) ||
      ( wOffset >= p->pClasses[wClass].nStrings ) ||
      ( ! p->pClasses[wClass].pStrings[wOffset] ) )
    {
      TCHAR szBuffer[32] ;
      
       _stprintf (szBuffer, TEXT("{0x%04X}"), dwKey) ;
       
       if( ! StringList_AddString (h, dwKey, szBuffer) )
	 return szInvalidString ;
    }
  
  return p->pClasses[wClass].pStrings[wOffset] ;
}
