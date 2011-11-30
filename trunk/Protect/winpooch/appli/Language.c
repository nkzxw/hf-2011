/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2007  Benoit Blanchon                      */
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
#include "Assert.h"
#include "Config.h"
#include "Language.h"
#include "ProjectInfo.h"
#include "StringList.h"
#include "Trace.h"

// standards headers
#include <stdio.h>
#include <tchar.h>


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define MAX_LANGUAGE_COUNT	32


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct 
{
  LPTSTR	szFile ;
  LPTSTR	szName ; 
} LANGUAGE ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static HSTRINGLIST	hslStrings ;
static int		nLanguages ;
static HWND		hwndNotify ;
static LANGUAGE		pLanguages[MAX_LANGUAGE_COUNT] ;


/******************************************************************/
/* Internal function : GetAbsolutePath                            */
/******************************************************************/

void _Language_GetAbsolutePath (LPTSTR szPath, LPCTSTR szFile)
{
  TCHAR *p ;

  GetModuleFileName (NULL, szPath, MAX_PATH) ;
  
  p = _tcsrchr (szPath, TEXT('\\')) ;  
  ASSERT (p!=NULL) ;
  _tcscpy (p+1, szFile) ;
}


/******************************************************************/
/* Exported function : Init                                       */
/******************************************************************/

BOOL Language_Init ()
{
  TCHAR			szPath[MAX_PATH] ;
  TCHAR			szName[MAX_LANGUAGE_NAME] ;
  HANDLE		hSearch ;
  WIN32_FIND_DATA	wfd ;

  // verify that it's the first initialisation
  ASSERT (nLanguages==0) ;
 
  hSearch = FindFirstFile (TEXT("languages\\*.txt"), &wfd) ;

  if( hSearch==INVALID_HANDLE_VALUE ) {
    _Language_GetAbsolutePath (szPath, TEXT("languages\\*.txt")) ;
    hSearch = FindFirstFile (szPath, &wfd) ;
  }
     
  if( hSearch==INVALID_HANDLE_VALUE ) 
    return FALSE ;
  
  do {

    _stprintf (szPath, TEXT("languages\\%s"), wfd.cFileName) ;

    if( StringList_GetFromFile (szPath, _LANGUAGE_NAME_IN_ENGLISH,
				szName, MAX_LANGUAGE_NAME) )
      {
	TRACE_INFO (TEXT("%s -> %s\n"), szPath, szName) ;

        pLanguages[nLanguages].szFile = _tcsdup (szPath) ;
	pLanguages[nLanguages].szName = _tcsdup (szName) ;

	if( nLanguages++ >= MAX_LANGUAGE_COUNT )
	  {
	    TRACE_ERROR (TEXT("Maximum language count exceded\n")) ;
	    break ;
	  }
      }
    else
      {
	TRACE_ERROR (TEXT("Failed to read language name in %s\n"), szPath) ;
      }
           
  } while( FindNextFile(hSearch,&wfd) ) ;
  
  FindClose (hSearch) ;

  if( nLanguages == 0 )
    return FALSE ;
  
  if( Config_GetString(CFGSTR_LANGUAGE) && Language_LoadByName (Config_GetString(CFGSTR_LANGUAGE)) )
      return TRUE ;
    
  return FALSE ;
}


/******************************************************************/
/* Exported function : Uninit                                     */
/******************************************************************/

VOID Language_Uninit ()
{
  // free each language descriptor
  while( nLanguages>0 ) pLanguages[--nLanguages] ;

  StringList_Free (hslStrings) ;
}


/******************************************************************/
/* Exported function : SetHwnd                                    */
/******************************************************************/

VOID Language_SetHwnd (HWND hwnd)
{
  hwndNotify = hwnd ;
}


/******************************************************************/
/* Exported function : LoadByName                                 */
/******************************************************************/

BOOL Language_LoadByName (LPCTSTR szName)
{
  int i ;
  int iSelected = -1 ;

  ASSERT (szName!=NULL) ;

  for( i=0 ; i<nLanguages ; i++ )
    {
      if( ! _tcscmp(szName,pLanguages[i].szName) )
	iSelected = i ;
    }

  if( iSelected<0 ) return FALSE ;

  return Language_LoadByIndex (iSelected) ;
}


/******************************************************************/
/* Exported function : LoadByIndex                                */
/******************************************************************/

BOOL Language_LoadByIndex (int i)
{
  ASSERT (i>=0) ;
  ASSERT (i<nLanguages) ;
  
  return Language_LoadByFile (pLanguages[i].szFile) ;
}


/******************************************************************/
/* Exported function : LoadByFile                                 */
/******************************************************************/

BOOL Language_LoadByFile (LPCTSTR szPath)
{
  HSTRINGLIST	hslNew ;

  hslNew = StringList_Load (szPath) ;

  if( hslNew )
    {
      StringList_Free (hslStrings) ;

      hslStrings = hslNew ;

      Config_SetString (CFGSTR_LANGUAGE, STR(_LANGUAGE_NAME_IN_ENGLISH)) ;

      PostMessage (hwndNotify, WM_LANGUAGECHANGED, 0, 0) ;
    }
  
  return hslNew!=NULL ;
}


/******************************************************************/
/* Exported function : GetString                                  */
/******************************************************************/

LPCTSTR Language_GetString (STRID id)
{
  return StringList_Get(hslStrings, id) ;
}


/******************************************************************/
/* Exported function : GetLanguage                                */
/******************************************************************/

LPCTSTR Language_GetLanguage (int i)
{
  ASSERT (i<nLanguages) ;

  return pLanguages[i].szName ;
}


/******************************************************************/
/* Exported function : GetLanguageCount                           */
/******************************************************************/

int Language_GetLanguageCount ()
{
  return nLanguages ;
}


/******************************************************************/
/* Exported function : IsLoaded                                   */
/******************************************************************/

BOOL Language_IsLoaded ()
{
  return hslStrings!=NULL ;
}

