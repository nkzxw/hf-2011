// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__CA74D12C_8B7C_416A_9E15_945C4A2D879D__INCLUDED_)
#define AFX_STDAFX_H__CA74D12C_8B7C_416A_9E15_945C4A2D879D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// owner add begin
//#define UNICODE
//#define _UNICODE

#include <afxwin.h>
#include <afxcmn.h>

#include <ws2spi.h>
#include <io.h>				// to use access function
#include <winioctl.h>

#define XF_COMMON_FUNCTION
#define XF_DLL_COMMON_FUNCTION

#include "..\common\ControlCode.h"
#include "..\common\xcommon.h"
#include "..\common\debug.h"
#include "..\common\FILT.h"
#include "Session.h"
#include "ProtocolInfo.h"
#include "MemoryFile.h"
#include "TcpIpDog.h"


// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__CA74D12C_8B7C_416A_9E15_945C4A2D879D__INCLUDED_)
