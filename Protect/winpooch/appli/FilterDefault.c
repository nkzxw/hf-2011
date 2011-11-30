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

#define TRACE_LEVEL	2	// warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "FilterDefault.h"

// standard headers
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "Trace.h"
#include "Wildcards.h"


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define arraysize(a) (sizeof(a)/sizeof((a)[0]))


/******************************************************************/
/* Exported function :                                            */
/******************************************************************/

BOOL FilterSet_InitDefaultFilter (HFILTERSET hFilterSet)
{
  HFILTER	hCurFilter ;
  TCHAR		szDir[MAX_PATH] ;
  TCHAR		szBuffer[MAX_PATH] ;

  TRACE ; 

  ASSERT (hFilterSet!=NULL) ;
  
  hCurFilter = FilterSet_GetDefaultFilter (hFilterSet) ;
  Filter_Clear (hCurFilter) ;

  // scan files on read
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_SILENT, RULE_SCAN,
		     FILTREASON_FILE_READ, TEXT("*")) ; 

  // create a process
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_LOG, 0,
		     FILTREASON_SYS_EXECUTE, TEXT("*")) ;

  // kill a process
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK,
		     FILTREASON_SYS_KILLPROCESS, TEXT("*")) ;

  // listen local interface
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_SILENT, 0,
  		     FILTREASON_NET_LISTEN, TEXT("s**"), TEXT("127.0.0.1")) ; 

  // listen all interface
  Filter_AddNewRule (hCurFilter,
		     RULE_ACCEPT, RULE_ALERT, 0,
  		     FILTREASON_NET_LISTEN, TEXT("***")) ;

  // connect to local interface
  Filter_AddNewRule (hCurFilter,
		     RULE_ACCEPT, RULE_SILENT, 0,
  		     FILTREASON_NET_CONNECT, TEXT("s**"), TEXT("127.0.0.1")) ;

  // connect to HTTPS
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_SILENT, 0, 
  		     FILTREASON_NET_CONNECT, TEXT("*nn"), 443, 6) ;

  // connect to news
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_SILENT, 0,
  		     FILTREASON_NET_CONNECT, TEXT("*nn"), 119, 6) ;

  // connect to POP
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_SILENT, 0,
  		     FILTREASON_NET_CONNECT, TEXT("*nn"), 110, 6) ;

  // connect to HTTP
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_SILENT, 0,
  		     FILTREASON_NET_CONNECT, TEXT("*nn"), 80, 6) ;

  // connect to SMTP 
  Filter_AddNewRule (hCurFilter,
		     RULE_ACCEPT, RULE_SILENT, 0,
  		     FILTREASON_NET_CONNECT, TEXT("*nn"), 25, 6) ;

  // connect to FTP 
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_SILENT, 0,
  		     FILTREASON_NET_CONNECT, TEXT("*nn"), 21, 6) ;

  // connect to any port
  Filter_AddNewRule (hCurFilter, 
		     RULE_ACCEPT, RULE_LOG, 0, 
  		     FILTREASON_NET_CONNECT, TEXT("***")) ;

  // UDP send to any addresses
  Filter_AddNewRule (hCurFilter,
		     RULE_ACCEPT, RULE_ALERT, 0,
  		     FILTREASON_NET_SEND, TEXT("**n"), 11) ;

  // Startup dirs

  if( SHGetSpecialFolderPath (NULL, szDir, CSIDL_COMMON_STARTUP, FALSE) ) {
    wsprintf (szBuffer, TEXT("%s\\*"), szDir) ;
    Filter_AddNewRule (hCurFilter, 
		       RULE_REJECT, RULE_LOG, RULE_ASK, 
		       FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;
  }

  if( SHGetSpecialFolderPath (NULL, szDir, CSIDL_COMMON_ALTSTARTUP, FALSE) ) {
    wsprintf (szBuffer, TEXT("%s\\*"), szDir) ;
    Filter_AddNewRule (hCurFilter, 
		       RULE_REJECT, RULE_LOG, RULE_ASK,
		       FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;
  }

  if( SHGetSpecialFolderPath (NULL, szDir, CSIDL_STARTUP, FALSE) ) {
    wsprintf (szBuffer, TEXT("%s\\*"), szDir) ;
    Filter_AddNewRule (hCurFilter,
		       RULE_REJECT, RULE_LOG, RULE_ASK, 
		       FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;  
  }

  if( SHGetSpecialFolderPath (NULL, szDir, CSIDL_ALTSTARTUP, FALSE) ) {
    wsprintf (szBuffer, TEXT("%s\\*"), szDir) ;
    Filter_AddNewRule (hCurFilter, 
		       RULE_REJECT, RULE_LOG, RULE_ASK, 
		       FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;  
  }

  // Windows directory
  SHGetSpecialFolderPath (NULL, szDir, CSIDL_WINDOWS, FALSE) ;

  wsprintf (szBuffer, TEXT("%s\\*.dll"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.exe"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.bat"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK,
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.ocx"), szDir) ;
  Filter_AddNewRule (hCurFilter, RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.pif"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.scr"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\system.ini"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("s"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\win.ini"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("s"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\wininit.ini"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK,
		     FILTREASON_FILE_WRITE, TEXT("s"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\Tasks\\*"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  // System32 directory
  SHGetSpecialFolderPath (NULL, szDir, CSIDL_SYSTEM, FALSE) ;

  wsprintf (szBuffer, TEXT("%s\\*.dll"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.exe"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.sys"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("w"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.bat"), szDir) ;
  Filter_AddNewRule (hCurFilter,
		     RULE_REJECT, RULE_LOG, RULE_ASK,
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.ocx"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.pif"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\*.scr"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), szBuffer) ;

  wsprintf (szBuffer, TEXT("%s\\drivers\\etc\\hosts"), szDir) ;
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("s"), szBuffer) ;

  // Drive root
  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), TEXT("?:\\explorer.exe")) ;

  Filter_AddNewRule (hCurFilter,
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), TEXT("?:\\autoexec.bat")) ;

  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), TEXT("?:\\boot.ini")) ;

  Filter_AddNewRule (hCurFilter,
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), TEXT("?:\\config.sys")) ;

  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), TEXT("?:\\ntdetect.com")) ;

  Filter_AddNewRule (hCurFilter, 
		     RULE_REJECT, RULE_LOG, RULE_ASK, 
		     FILTREASON_FILE_WRITE, TEXT("p"), TEXT("?:\\ntldr")) ;

  // REGISTRY
  {
    typedef struct {
      LPCTSTR	szFormat ;
      LPCTSTR	szKey ;
      LPCTSTR	szValue ;
    } QUICKRULE ;

    QUICKRULE	aAskReject[] = {
      { TEXT("w*"), TEXT("*\\Software\\Microsoft\\Windows\\CurrentVersion\\Run*")				},
      { TEXT("w*"), TEXT("*\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")			},
      { TEXT("w*"), TEXT("*\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run")		},
      { TEXT("ss"), TEXT("HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\boot"),	TEXT("shell")	},
      { TEXT("s*"), TEXT("HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping")		},
      { TEXT("ws"), TEXT("*\\Software\\Microsoft\\Windows\\CurrentVersion\\explorer\\shell folders"),TEXT("Common Startup") },
      { TEXT("ws"), TEXT("*\\Software\\Microsoft\\Windows\\CurrentVersion\\explorer\\user shell folders"),TEXT("Common Startup")},
      { TEXT("s*"), TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved") },
      { TEXT("s*"), TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad") },
      { TEXT("ss"), TEXT("HKLM\\Software\\Microsoft\\Active Setup\\Installed Components\\*"),TEXT("StubPath") },
      { TEXT("w*"), TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects") },
      { TEXT("ws"), TEXT("*\\Software\\Microsoft\\Command Processor"), TEXT("AutoRun") },
      { TEXT("ws"), TEXT("*\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), TEXT("load") },
      { TEXT("ws"), TEXT("*\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), TEXT("run") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Control\\Session Manager\\Environment"), TEXT("Path") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Control\\Session Manager\\Environment"), TEXT("ComSpec") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Control\\Session Manager"), TEXT("BootExecute") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\?\\shellex\\contextmenuhandlers") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\exefile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\exefile\\shell\\runas\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\Folder\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\Folder\\shell\\explore\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\batfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\comfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\cmdfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\regfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\scrfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\scrfile\\shell\\config\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\vbsfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\vbsfile\\shell\\open2\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\jsfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\jarfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\piffile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\htafile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\jarfile\\shell\\open\\command") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.exe") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.Folder") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.bat") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.com") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.cmd") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.reg") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.scr") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.vbs") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.js") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.jar") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.pif") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\.hta") },
      { TEXT("w*"), TEXT("*\\Software\\Classes\\mailto\\shell\\open\\command") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Control\\WOW"), TEXT("cmdline") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Control\\WOW"), TEXT("wowcmdline") },
      { TEXT("ws"), TEXT("HKU\\*\\Environment"), TEXT("Path") },
      { TEXT("ws"), TEXT("HKU\\*\\Control Panel\\Desktop"), TEXT("SCRNSAVE.EXE") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Control\\Session Manager"), TEXT("PendingFileRenameOperations") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Services\\Tcpip\\Parameters"), TEXT("DataBasePath") },
      { TEXT("ws"), TEXT("HKLM\\System\\ControlSet???\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\*"), TEXT("DataBasePath") }      
    } ;

    int i ; 
  
    for( i=0 ; i<arraysize(aAskReject) ; i++ )
      Filter_AddNewRule (hCurFilter,
			 RULE_REJECT, RULE_LOG, RULE_ASK, 
			 FILTREASON_REG_SETVALUE, 
			 aAskReject[i].szFormat,
			 aAskReject[i].szKey,
			 aAskReject[i].szValue) ;

  }	


  //
  // CSRSS.EXE
  //
  {
    TCHAR szSystem32[MAX_PATH] ;
    TCHAR srCsrssExe[MAX_PATH] ;

    // System32 directory
    SHGetSpecialFolderPath (NULL, szSystem32, CSIDL_SYSTEM, FALSE) ;
    
    wsprintf (srCsrssExe, TEXT("%s\\csrss.exe"), szSystem32) ;

    hCurFilter = FilterSet_GetFilterStrict (hFilterSet, srCsrssExe) ;

    if( hCurFilter == NULL )
      {
	hCurFilter = Filter_Create (srCsrssExe) ;
	FilterSet_AddFilter (hFilterSet, hCurFilter) ;
      }

    Filter_AddNewRule (hCurFilter,
		       RULE_ACCEPT, RULE_LOG, 0, 
		       FILTREASON_SYS_KILLPROCESS, 
		       TEXT("*")) ;
  }

  //
  // SERVICES.EXE
  //
  {
    TCHAR szSystem32[MAX_PATH] ;
    TCHAR srServicesExe[MAX_PATH] ;

    // System32 directory
    SHGetSpecialFolderPath (NULL, szSystem32, CSIDL_SYSTEM, FALSE) ;
    
    wsprintf (srServicesExe, TEXT("%s\\services.exe"), szSystem32) ;

    hCurFilter = FilterSet_GetFilterStrict (hFilterSet, srServicesExe) ;

    if( hCurFilter == NULL )
      {
	hCurFilter = Filter_Create (srServicesExe) ;
	FilterSet_AddFilter (hFilterSet, hCurFilter) ;
      }

    Filter_AddNewRule (hCurFilter,
		       RULE_ACCEPT, RULE_SILENT, 0, 
		       FILTREASON_REG_SETVALUE, 
		       TEXT("ws"),
		       TEXT("HKU\\*\\Software\\Microsoft\\Windows"),
		       TEXT("ParseAutoexec")) ;
  }

  //
  // EXPLORER.EXE
  //
  {
    TCHAR szWindows[MAX_PATH] ;
    TCHAR szExplorerExe[MAX_PATH] ;

    // System32 directory
    SHGetSpecialFolderPath (NULL, szWindows, CSIDL_WINDOWS, FALSE) ;
    
    wsprintf (szExplorerExe, TEXT("%s\\explorer.exe"), szWindows) ;

    hCurFilter = FilterSet_GetFilterStrict (hFilterSet, szExplorerExe) ;

    if( hCurFilter == NULL )
      {
	hCurFilter = Filter_Create (szExplorerExe) ;
	FilterSet_AddFilter (hFilterSet, hCurFilter) ;
      }

    Filter_AddNewRule (hCurFilter,
		       RULE_ACCEPT, RULE_SILENT, 0, 
		       FILTREASON_REG_SETVALUE, 
		       TEXT("ss"),
		       TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
		       TEXT("Common Startup")) ;
  }
    
  return TRUE ;
}
