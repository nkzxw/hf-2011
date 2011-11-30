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

#define	TRACE_LEVEL		2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Config.h"

// project's headers
#include "Assert.h"
#include "Trace.h"
#include "ProjectInfo.h"

// standard headers
#include <tchar.h>
#include <shlobj.h>


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  LPCTSTR	szName ;
  LPCTSTR	szDefaultValue ;
  LPTSTR	szValue ;
} CONFIG_STRING ;

typedef struct {
  LPCTSTR	szName ;
  int		iDefaultValue ;
  int		iValue ;
} CONFIG_INTEGER ;

typedef struct {
  LPCTSTR	szName ;
  UINT		nLength ;
  UINT		nBufferSize ;
  LPTSTR*	pszArray ;
  BYTE*		pBuffer ;
} CONFIG_STRING_ARRAY ;


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define CONFIG_STRING_MAX	256

static LPCTSTR szRegKey = TEXT("Software\\" APPLICATION_NAME) ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static CONFIG_STRING aStrings[_CFGSTR_COUNT] = {
  { TEXT("Language"),				TEXT("English") },
  { TEXT("Antivirus"),				NULL },
  { TEXT("Custom alert sound"),			TEXT("") },
  { TEXT("Custom ask sound"),			TEXT("") },
  { TEXT("Custom virus sound"),			TEXT("") },
} ;

static CONFIG_INTEGER aIntegers[_CFGINT_COUNT] = {
  { TEXT("ProcessGuard detected"),		0 },
  { TEXT("RegDefend detected"),                 0 },
  { TEXT("Splash screen"),			1 },
  { TEXT("Max log file size"),			1024*1024 },
  { TEXT("Sound"),				1 },
  { TEXT("Check for updates"),			1 },
  { TEXT("Scan cache length"),			1024 },
  { TEXT("Skip first close warning"),		0 },
  { TEXT("Tray icon animation"),                1 },
} ;

static CONFIG_STRING_ARRAY aStringArrays[_CFGSAR_COUNT] = {
  { TEXT("Scan include pattern")		},
  { TEXT("Background scan folders")		},
} ;

static HKEY g_hKey = NULL ;


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define arraysize(a) (sizeof(a)/sizeof((a)[0]))


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

VOID	_Config_SetDefaultValues () ;


/******************************************************************/
/* Exported function : Init                                       */
/******************************************************************/

BOOL Config_Init () 
{  
  LONG lResult ;

  // try to create config on LOCAL_MACHINE root key
  lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szRegKey, 0, 
			    TEXT(""), REG_OPTION_NON_VOLATILE, 
			    KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &g_hKey, NULL) ;

  // else try in CURRENT_USER
  if( lResult!=ERROR_SUCCESS )
    lResult = RegCreateKeyEx (HKEY_CURRENT_USER, szRegKey, 0, 
			      TEXT(""), REG_OPTION_NON_VOLATILE, 
			      KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &g_hKey, NULL) ;

  // if both failed the surrender
  if( lResult!=ERROR_SUCCESS )
    return FALSE ;

  _Config_SetDefaultValues () ;
  
  return Config_Load () ;
}


/******************************************************************/
/* Exported function : Uninit                                     */
/******************************************************************/

BOOL Config_Uninit ()
{
  int i;

  TRACE ;

  Config_Save () ;

  RegCloseKey (g_hKey) ;

  // free strings
  for( i=0 ; i<_CFGSTR_COUNT ; i++ )
    {
      free (aStrings[i].szValue) ;
      aStrings[i].szValue = NULL ;
    } 

  // free array strings
  for( i=0 ; i<_CFGSTR_COUNT ; i++ )
    {
      aStringArrays[i].nLength = 0 ;
      aStringArrays[i].nBufferSize = 0 ;

      free (aStringArrays[i].pszArray) ;
      aStringArrays[i].pszArray = NULL ;

      free (aStringArrays[i].pBuffer) ;
      aStringArrays[i].pBuffer = NULL ;
    } 

  return TRUE ;
}


/******************************************************************/
/* Exported function : Load                                       */
/******************************************************************/

BOOL Config_Load () 
{
  DWORD		dwSize ;
  DWORD		dwType ;
  DWORD		dwValue ;
  LONG		lResult ;
  int		i ;

  //
  // 1. Strings
  //
  
  // for each config string (begin)
  for( i=0 ; i<_CFGSTR_COUNT ; i++ )
    {
      TCHAR		szBuffer[CONFIG_STRING_MAX] ;

      // get value
      dwSize = CONFIG_STRING_MAX ;
      dwType = REG_SZ ;      
      lResult = RegQueryValueEx (g_hKey, aStrings[i].szName, NULL, 			
				 &dwType, (BYTE*)szBuffer, &dwSize) ;
      // if success then store value
      if( lResult==ERROR_SUCCESS && szBuffer[0]!=0 )
	aStrings[i].szValue = _tcsdup (szBuffer) ;    
    }
  // for each config string (end)

  //
  // 2. Integers
  //

  // for each config integer (begin)
  for( i=0 ; i<_CFGINT_COUNT ; i++ )
    {
      // get value
      dwSize = sizeof(DWORD) ;
      dwType = REG_DWORD ;
      lResult = RegQueryValueEx (g_hKey, aIntegers[i].szName, NULL, 
				 &dwType, (BYTE*)&dwValue, &dwSize) ;
      // if success then store value
      if( lResult==ERROR_SUCCESS )
	aIntegers[i].iValue = dwValue ;
    }
  // for each config integer (end)

  //
  // 3. String arrays
  //

  // for each config string array (begin)
  for( i=0 ; i<_CFGSAR_COUNT ; i++ )
    {   
      TRACE_INFO (TEXT("Getting size for array \"%s\"...\n"), aStringArrays[i].szName) ;

      // get required size
      dwSize = 0 ;
      
      lResult = RegQueryValueEx (g_hKey, aStringArrays[i].szName, NULL, 			
				 &dwType, NULL, &dwSize) ;

      if( lResult!=ERROR_SUCCESS || dwType!=REG_MULTI_SZ ) {
	TRACE_INFO (TEXT("Error while getting size (result=%u, type=%u)\n"), lResult, dwType) ;
	continue ;
      }

      TRACE_INFO (TEXT("Required size is %u bytes\n"), dwSize) ;

      if( aStringArrays[i].pBuffer!=NULL ) free (aStringArrays[i].pBuffer) ;
      if( aStringArrays[i].pszArray!=NULL ) free (aStringArrays[i].pszArray) ;

      if( dwSize == 0 )
	{
	  aStringArrays[i].nBufferSize = 0 ;
	  aStringArrays[i].pBuffer = NULL ;
	  aStringArrays[i].nLength = 0 ;
	  aStringArrays[i].pszArray = NULL ;
	}
      else
	{
	  aStringArrays[i].nBufferSize = dwSize ;
	  aStringArrays[i].pBuffer = malloc (dwSize) ; ;
	  
	  TRACE_INFO (TEXT("Reading array \"%s\"...\n"), aStringArrays[i].szName) ;
	  
	  lResult = RegQueryValueEx (g_hKey, aStringArrays[i].szName, NULL, &dwType, 
				     aStringArrays[i].pBuffer, &dwSize) ;
	  
	  if( lResult!=ERROR_SUCCESS )
	    {
	      TRACE_INFO (TEXT("Error while reading\n")) ;
	      free (aStringArrays[i].pBuffer) ;
	      aStringArrays[i].pBuffer = NULL ;
	      aStringArrays[i].nBufferSize = 0 ;
	      continue ;
	    }
	      
	  aStringArrays[i].nBufferSize = dwSize ;
         
	  // count strings and get pointers
	  {
	    LPTSTR szBuffer ;
	    int j, n ;
	    
	    TRACE_INFO (TEXT("Counting string in array...\n")) ;
	    
	    szBuffer = (LPTSTR)aStringArrays[i].pBuffer ;
	    n = 0 ;
	    
	    for( j=1 ; j<dwSize/sizeof(TCHAR) ; j++ )
	      {
		if( szBuffer[j]==0 && szBuffer[j-1]!=0 )  n++ ;		
	      }
	    
	    TRACE_INFO (TEXT("Found %d strings.\n"), n) ;
	    
	    aStringArrays[i].nLength = n ;
	    aStringArrays[i].pszArray = malloc (aStringArrays[i].nLength*sizeof(LPTSTR)) ;
	    
	    n = 0 ;
	    aStringArrays[i].pszArray[n] = szBuffer ;
	    TRACE_INFO (TEXT("String %d is \"%s\"\n"), n, aStringArrays[i].pszArray[n]) ;
	    n++ ;
	    
	    for( j=1 ; j<dwSize/sizeof(TCHAR) ; j++ )
	      {
		if( szBuffer[j]!=0 && szBuffer[j-1]==0 )
		  {
		    aStringArrays[i].pszArray[n] = &szBuffer[j] ;
		    TRACE_INFO (TEXT("String %d is \"%s\"\n"), n, aStringArrays[i].pszArray[n]) ;
		    n++ ;
		  }
	      }
	    
	    TRACE_INFO (TEXT("Finished getting %d strings\n"), n) ;
	  }
      }
    }
  // for each config string array (end)
 
  return TRUE ;
}


/******************************************************************/
/* Exported function : Save                                       */
/******************************************************************/

BOOL Config_Save ()
{
  int		i ;

  TRACE ;

  // 1. Strings
  //
  for( i=0 ; i<_CFGSTR_COUNT ; i++ )
    Config_SaveString (i) ;

  //
  // 2. Integers
  //
  for( i=0 ; i<_CFGINT_COUNT ; i++ )
    Config_SaveInteger (i) ;

  //
  // 3. String arrays
  //
  for( i=0 ; i<_CFGSAR_COUNT ; i++ )
    Config_SaveStringArray (i) ;
 
  return TRUE ;
}

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

void Config_SaveString (CONFIG_STRING_ID i) 
{
  LPCTSTR szValue ;
  
  TRACE ;

  szValue = aStrings[i].szValue ?: aStrings[i].szDefaultValue ;
  
  if( ! szValue ) return ;
  
  RegSetValueEx (g_hKey, aStrings[i].szName, 0, REG_SZ, 
		 (BYTE*)szValue, 
		 (_tcslen(szValue)+1)*sizeof(TCHAR)) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

void Config_SaveStringArray (CONFIG_STRING_ARRAY_ID i) 
{
  TRACE ;

  RegSetValueEx (g_hKey, aStringArrays[i].szName, 0, REG_MULTI_SZ, 
		 (BYTE*)aStringArrays[i].pBuffer, 
		 aStringArrays[i].nBufferSize) ; 
}

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

void Config_SaveInteger (CONFIG_INTEGER_ID i) 
{
  DWORD		dwValue ;

  TRACE ;

  dwValue = aIntegers[i].iValue ;

  RegSetValueEx (g_hKey, aIntegers[i].szName, 0, REG_DWORD, 
		 (BYTE*)&dwValue, sizeof(dwValue)) ; 
}


/******************************************************************/
/* Exported function : GetInteger                                 */
/******************************************************************/

int Config_GetInteger (CONFIG_INTEGER_ID id)
{
  ASSERT (id>=0) ;
  ASSERT (id<_CFGINT_COUNT) ;

  return aIntegers[id].iValue ;
}


/******************************************************************/
/* Exported function : SetInteger                                 */
/******************************************************************/

void Config_SetInteger (CONFIG_INTEGER_ID id, int iValue)
{
  ASSERT (id>=0) ;
  ASSERT (id<_CFGINT_COUNT) ;

  aIntegers[id].iValue = iValue ;
}


/******************************************************************/
/* Exported function : GetString                                  */
/******************************************************************/

LPCTSTR Config_GetString (CONFIG_STRING_ID id)
{
  ASSERT (id>=0) ;
  ASSERT (id<_CFGSTR_COUNT) ;

  return aStrings[id].szValue ;
}


/******************************************************************/
/* Exported function : SetString                                  */
/******************************************************************/

void Config_SetString (CONFIG_STRING_ID id, LPCTSTR szValue)
{
  ASSERT (id>=0) ;
  ASSERT (id<_CFGSTR_COUNT) ;

  free (aStrings[id].szValue) ;
  aStrings[id].szValue = _tcsdup (szValue) ;
}




/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID Config_SetStringArray (CONFIG_STRING_ARRAY_ID id, LPCTSTR* pszArray, UINT nLength) 
{
  ASSERT (id>=0) ;
  ASSERT (id<_CFGSAR_COUNT) ;

  if( aStringArrays[id].pBuffer!=NULL ) free (aStringArrays[id].pBuffer) ;
  if( aStringArrays[id].pszArray!=NULL ) free (aStringArrays[id].pszArray) ;
  
  if( nLength > 0 )
    {
      UINT i, n ;

      TRACE_INFO (TEXT("Calculating buffer size...\n")) ;
      
      n = 0 ;
      for( i=0 ; i<nLength ; i++ )
	n += (_tcslen(pszArray[i])+1)*sizeof(TCHAR) ;

      TRACE_INFO (TEXT("Buffer size is %u\n"), n) ;
      
      aStringArrays[id].nBufferSize = n ;
      aStringArrays[id].nLength = nLength ;
      aStringArrays[id].pBuffer = malloc (n) ;
      aStringArrays[id].pszArray = malloc (nLength*sizeof(LPCTSTR)) ;
      
      n = 0 ;
      for( i=0 ; i<nLength ; i++ )
	{	  
	  aStringArrays[id].pszArray[i] = (LPTSTR)&aStringArrays[id].pBuffer[n] ;
	  _tcscpy (aStringArrays[id].pszArray[i], pszArray[i]) ;
	  n += (_tcslen(pszArray[i])+1)*sizeof(TCHAR) ;
	  TRACE_INFO (TEXT("String %u = %s\n"), i, aStringArrays[id].pszArray[i]) ;
	}     
    }
  else
    {
      aStringArrays[id].pBuffer = NULL ;
      aStringArrays[id].pszArray = NULL ;
      aStringArrays[id].nBufferSize = 0 ;
      aStringArrays[id].nLength = 0 ;
    }
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

LPCTSTR* Config_GetStringArray (CONFIG_STRING_ARRAY_ID id, UINT * pnLength) 
{
  ASSERT (id>=0) ;
  ASSERT (id<_CFGSAR_COUNT) ;
  ASSERT (pnLength!=0) ;
  
  *pnLength = aStringArrays[id].nLength ;

  return (LPCTSTR*)aStringArrays[id].pszArray ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _Config_SetDefaultValues () 
{
  int i ;

  //
  // set deault values for each integer
  //
  for( i=0 ; i<_CFGINT_COUNT ; i++ )
    aIntegers[i].iValue = aIntegers[i].iDefaultValue ;

  //
  // Set default values for "Background scan folders"
  //
  {
    TCHAR szWindowsFolder[MAX_PATH] ;
    LPCTSTR szStringArray[1] ;
    
    SHGetSpecialFolderPath (NULL, szWindowsFolder, CSIDL_WINDOWS, FALSE) ;
    
    szStringArray[0] = szWindowsFolder ;

    Config_SetStringArray (CFGSAR_SCAN_FOLDERS, szStringArray, arraysize(szStringArray)) ;
  }

  //
  // Set default values for "Scan include filter"
  //
  {
    LPCTSTR szStringArray[] = 
      { TEXT("*.exe"), TEXT("*.com"), TEXT("*.dll"), TEXT("*.ocx"), 
	TEXT("*.scr"), TEXT("*.bin"), TEXT("*.dat"), TEXT("*.386"),
	TEXT("*.vxd"), TEXT("*.sys"), TEXT("*.wdm"), TEXT("*.cla"), 
	TEXT("*.class"), TEXT("*.ovl"), TEXT("*.ole"), TEXT("*.hlp"), 
	TEXT("*.doc"), TEXT("*.dot"), TEXT("*.xls"), TEXT("*.ppt"), 
	TEXT("*.wbk"), TEXT("*.wiz"), TEXT("*.pot"), TEXT("*.ppa"), 
	TEXT("*.xla"), TEXT("*.xlt"), TEXT("*.vbs"), TEXT("*.vbe"), 
	TEXT("*.mdb"), TEXT("*.rtf"), TEXT("*.htm"), TEXT("*.hta"), 
	TEXT("*.html"), TEXT("*.xml"), TEXT("*.xtp"), TEXT("*.php"),
	TEXT("*.asp"), TEXT("*.js"), TEXT("*.shs"), TEXT("*.chm"),
	TEXT("*.lnk"), TEXT("*.pif"), TEXT("*.prc"), TEXT("*.url"), 
	TEXT("*.smm"), TEXT("*.pfd"), TEXT("*.msi"), TEXT("*.ini"), 
	TEXT("*.csc"), TEXT("*.cmd"), TEXT("*.bas"), TEXT("*.nws") } ;

    Config_SetStringArray (CFGSAR_SCAN_PATTERNS, szStringArray, arraysize(szStringArray)) ;
  }
}
