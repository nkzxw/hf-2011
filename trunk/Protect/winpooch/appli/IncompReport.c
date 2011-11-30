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

// standard headers
#include <windows.h>
#include <io.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>

// project's headers
#include "BuildCount.h"
#include "ImgInfo.h"
#include "ProjectInfo.h"
#include "Strlcpy.h"


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define arraysize(a) (sizeof(a)/sizeof((a)[0]))


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct
{
  UINT		nSize ;
  char		szVersion[64] ;
  char		szExeFile[64] ;
  char		szDbgFile[64] ;
  char		szSignature[64] ;
  char		szTranslation[64] ;
} KERNEL_INFO ;

/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

static LPCTSTR g_aKernels[] = 
{ 
  TEXT("ntkrnlpa.exe"), 
  TEXT("ntoskrnl.exe") 
} ;

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _IncompReport_GetKernelInfo (KERNEL_INFO * p, LPCTSTR szFile) 
{
  VOID	*pBuffer ;
  IMGINFO	info ;
  FILE	*fpInput ;
  DWORD	dwFoo, nVersionInfoSize ;   
      
  // open file for reading
  fpInput = _tfopen (szFile, TEXT("rb")) ;
  if( ! fpInput ) return FALSE ;
  
  // get file size
  p->nSize = filelength(fileno(fpInput)) ;
  
  // alloc buffer for the whole file
  pBuffer = malloc (p->nSize) ;
      
  // read the whole file
  fread (pBuffer, p->nSize, 1, fpInput) ;
  
  fclose (fpInput) ;
  
  // get image info and print into output text file
  if( ImgInfo_GetInfo (&info, pBuffer, p->nSize, TRUE) )
    {
      strlcpy (p->szDbgFile, info.szSymFilename, 64) ;
      strlcpy (p->szSignature, info.szSymSignature, 64) ;	 
    }
  
  free (pBuffer) ;
  
  nVersionInfoSize = GetFileVersionInfoSize(szFile,&dwFoo) ;
  pBuffer = malloc (nVersionInfoSize) ;

  if( GetFileVersionInfo(szFile,0,nVersionInfoSize,pBuffer) ) 
    {
      UINT	uLen ;
      LPSTR	szVersion ;
      LPSTR	szInternalName ;
      WORD	*pwTranslation ;
      
      if( VerQueryValueA (pBuffer,"\\VarFileInfo\\Translation", 
			 (LPVOID *)&pwTranslation, &uLen) )
	{
	  char szSubBlock[64] ;

	  VerLanguageNameA (*(DWORD*)pwTranslation, p->szTranslation, 64) ;
	  
	  sprintf (szSubBlock,
		   "\\StringFileInfo\\%04x%04x\\FileVersion",
		   pwTranslation[0], pwTranslation[1]) ;
	  
	  if( VerQueryValueA(pBuffer,szSubBlock, (void**)&szVersion,&uLen) ) 
	    strlcpy (p->szVersion, szVersion, 64) ;

	  sprintf (szSubBlock,
		   "\\StringFileInfo\\%04x%04x\\InternalName",
		   pwTranslation[0], pwTranslation[1]) ;
	  
	  if( VerQueryValueA(pBuffer,szSubBlock, (void**)&szInternalName,&uLen) ) 
	    strlcpy (p->szExeFile, szInternalName, 64) ;
	}
    }
  
  free (pBuffer) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL IncompReport_Generate (LPCTSTR szFilename)
{
  FILE * fpOutput = _tfopen (szFilename, TEXT("wt"));

  int i ;

  _ftprintf (fpOutput,
	     TEXT("This file is an \"incompatibility report\" for %s version %s\n\n"),
	     TEXT(APPLICATION_NAME), TEXT(APPLICATION_VERSION_STRING)) ;

  _ftprintf (fpOutput,
	     TEXT("It contains information that allow Winpooch development team\n"
		  "to make Winpooch compatible with your system.\n"
		  "Please post in on the \"bug report system\" at the address :\n"
		  "http://sourceforge.net/tracker/?group_id=122629&atid=694093\n\n")) ;    
	   
  for( i=0 ; i<arraysize(g_aKernels) ; i++ )
    {
      KERNEL_INFO ki ;
      TCHAR	szInputPath[MAX_PATH] ;
      
      // Get C:\Windows\System32 location
      SHGetSpecialFolderPath (NULL, szInputPath, CSIDL_SYSTEM, FALSE) ;
      
      // append file name
      PathAppend (szInputPath, g_aKernels[i]) ;

      if( _IncompReport_GetKernelInfo (&ki, szInputPath) )
	{
	  char szVersionFull[256] ;

	  _ftprintf (fpOutput,
		     TEXT("File %s contains the following kernel :\n"),
		     g_aKernels[i]) ;

	  strcpy (szVersionFull, ki.szExeFile) ;
	  strrchr (szVersionFull, '.')[0] = 0 ;
	  strcat (szVersionFull, " ") ;
	  strcat (szVersionFull, ki.szVersion) ;

	  /*if( !strncmp(ki.szVersion,"5.0",3) )
	    {
	      strcat (szVersionFull, " (") ;
	      strcat (szVersionFull, ki.szTranslation) ;
	      strcat (szVersionFull, ")") ;
	      }*/

	  _ftprintf (fpOutput, TEXT("- Label......... %hs\n"), szVersionFull) ;
	  _ftprintf (fpOutput, TEXT("- Dbg file...... %hs\n"), ki.szDbgFile) ;
	  _ftprintf (fpOutput, TEXT("- Signature..... %hs\n"), ki.szSignature) ;
	  _ftprintf (fpOutput, TEXT("- Translation... %hs\n"), ki.szTranslation) ;
	  _ftprintf (fpOutput, TEXT("- Size.......... %d kB\n"), ki.nSize/1024) ;
	  _ftprintf (fpOutput, TEXT("\n")) ;
	}
    }

  fclose (fpOutput) ;

  return TRUE ;
}
