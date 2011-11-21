// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__7775F969_C59B_4803_977A_ED4F509C6EC5__INCLUDED_)
#define AFX_STDAFX_H__7775F969_C59B_4803_977A_ED4F509C6EC5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#define XF_COMMON_FUNCTION
#define XF_GUI_COMMON_FUNCTION

#pragma comment (linker,"/NODEFAULTLIB:libc.lib")

//
// 2002/05/24 add for Qinghua Ziguang Bote
//
//#define ZIGUANG


#include <winsock2.h>
#include <winioctl.h>
#include "serviceControl.h"
#include "..\common\guires.h"
#include "..\common\filt.h"
#include "..\common\ControlCode.h"
#include "..\common\ColorStatic.h"
#include "..\common\BtnST.h"
#include "..\common\PasseckDialog.h"
#include "..\common\xfile.h"
#include "..\common\xcommon.h"
#include "..\common\Process.h"
#include "..\common\xinstall.h"
#include "..\common\PacketMonitor.h"
#include "..\common\XLogFile.h"
#include "HyperLink.h"
#include "Internet.h"
#include "SystemTray.h"
#include "HtmlHelp\HtmlHelp.h"

//
// 2002/12/19 for v2.1.0
//
#include ".\Shell\Shell.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7775F969_C59B_4803_977A_ED4F509C6EC5__INCLUDED_)
