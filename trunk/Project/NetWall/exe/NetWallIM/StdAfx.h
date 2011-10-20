// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__BBABF03F_2D5B_41AB_8DC2_342328098147__INCLUDED_)
#define AFX_STDAFX_H__BBABF03F_2D5B_41AB_8DC2_342328098147__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>		// MFC socket extensions

#include <ifnet.h>
#include <NetWallAPI.h>

typedef enum {INIT = 0x00, ADAPTER = 0x01, RULE = 0x02, RULEITEM = 0x03, LOG = 0x04} NETWALL_LIST_TYPE;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__BBABF03F_2D5B_41AB_8DC2_342328098147__INCLUDED_)
