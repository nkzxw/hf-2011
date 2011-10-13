//---------------------------------------------------------------------------
//
// stdafx.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook server / Common header
//				
//             
// AUTHOR:		Ivo Ivanov
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------

#if !defined(_STDAFX_H_)
#define _STDAFX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// Exclude rarely-used stuff from Windows headers
//
#define VC_EXTRALEAN		

#include "..\Common\Common.h"

#ifdef UNICODE
//
// MFC UNICODE applications use wWinMainCRTStartup as the entry point.
// For more details see Q125750
//
#pragma comment(linker, "/ENTRY:wWinMainCRTStartup")
#else
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")
#endif // #ifdef UNICODE

#include <afxwin.h>         // MFC core and standard components


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_STDAFX_H_)
//----------------------------End of the file -------------------------------
