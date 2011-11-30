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
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Assert.h"

// standart headers
#include <stdio.h>

// project's headers
#include "ProjectInfo.h"
#include "Trace.h"

#define MAX_LENGTH	256


static PROC	g_pfnAfterAssert = NULL ;


void SetOnAssertFailed (PROC pfn)
{
  g_pfnAfterAssert = pfn ;
}

void _Assert_Function (LPCSTR szExpr, LPCSTR szFile, int iLine)
{
  TCHAR szBuffer[MAX_LENGTH] ;

  wsprintf (szBuffer, TEXT("Assertion failed : %hs\nFile : %hs\nLine %d\n"),
	    szExpr, szFile, iLine) ;

  DbgPrint (TEXT("%s"), szBuffer) ;

  MessageBox (NULL, szBuffer, TEXT(APPLICATION_NAME), 
	      MB_ICONERROR|MB_SYSTEMMODAL|MB_SETFOREGROUND) ;

  //if( g_pfnAfterAssert ) g_pfnAfterAssert () ;

  DebugBreak();
  FatalExit (1) ;  
}
