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

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif

// module's interface
#include "Wildcards.h"

// project's headers
#include "Assert.h"

#include <ctype.h>
#include <tchar.h>

#ifdef __NTDDK__
#define towupper RtlUpcaseUnicodeChar
#endif

BOOL Wildcards_GenericCmp (LPCTSTR szPattern, LPCTSTR szString, BOOL bPartial, BOOL bStopAtSlash) 
{
  int i, j ;

  // verify params
  ASSERT (szPattern!=NULL) ;
  ASSERT (szString!=NULL) ;
  
  ///_tprintf (TEXT("\n- %s\n+ %s\n"), szPattern, szString) ;

  for( i=0 ; szPattern[i] ; i++ )
    {
      switch( szPattern[i] )
	{
	case TEXT('?'):

	  // end of the string ?
	  if( szString[i]==0 ) 
	    return FALSE ;

	  // new directory
	  if( bStopAtSlash && szString[i]==TEXT('\\') ) 
	    return FALSE ;	  
	  
	  break ;

	case TEXT('*'):
	  	  
	  // star at the end ?
	  if( szPattern[i+1]==0 )
	    return TRUE ;
  
	  for( j=0 ; szString[i+j] ; j++ )
	    {
	      if( bStopAtSlash && szString[i+j]==TEXT('\\') && szPattern[i+1]!=TEXT('\\') ) 
		return FALSE ;	  
	      
	      if( Wildcards_GenericCmp(szPattern+i+1,szString+i+j,bPartial,bStopAtSlash) )
		return TRUE ;
	    }
	  
	  return FALSE ;
	  
	default:

	  // end of the string ?
	  if( szString[i]==0 ) 
	    return bPartial ;

	  // compare chars
	  if( _totupper(szPattern[i]) != _totupper(szString[i]) )
	    return FALSE ;
	}
    }
	  
  return szString[i]==0 ? TRUE : bPartial ;
}

BOOL Wildcards_CompleteStringCmp (LPCTSTR szPattern, LPCTSTR szString) 
{
  return Wildcards_GenericCmp (szPattern, szString, FALSE, FALSE) ;
}

BOOL Wildcards_PartialStringCmp (LPCTSTR szPattern, LPCTSTR szString) 
{
  return Wildcards_GenericCmp (szPattern, szString, TRUE, FALSE) ;
}

BOOL Wildcards_CompletePathCmp (LPCTSTR szPattern, LPCTSTR szString) 
{
  return Wildcards_GenericCmp (szPattern, szString, FALSE, TRUE) ;
}

BOOL Wildcards_PartialPathCmp (LPCTSTR szPattern, LPCTSTR szString) 
{
  return Wildcards_GenericCmp (szPattern, szString, TRUE, TRUE) ;
}

