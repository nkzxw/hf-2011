// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

#define DO_NOT_INCLUDE_XCOMBOLIST 

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0500

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls

#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxmt.h>
//#include <afxsock.h>

#include "iphlpapi.h"
#include "Iprtrmib.h"
#include "Fltdefs.h"
#include "FilterDefs.h"
#include "Psapi.h"
#include "NetdefenderComnInc.h"


