
#if !defined(AFX_GRAVEDIGGER_H__1506EDF6_6F2C_46D3_B73D_65553DAA461A__INCLUDED_)
#define AFX_GRAVEDIGGER_H__1506EDF6_6F2C_46D3_B73D_65553DAA461A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

#include "../R3Dbg/R3Dbg.h"

#define DLLIMPORT __declspec(dllimport)

typedef VOID (APIENTRY *PAPCFUNC)( DWORD dwParam );
typedef DWORD (__stdcall   *MYPROC)(PAPCFUNC, DWORD, DWORD);
typedef HANDLE (APIENTRY *OPENTHREAD)( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId);

#pragma comment(lib,"../R3Dbg/Debug/R3Dbg.lib")

#endif // !defined(AFX_GRAVEDIGGER_H__1506EDF6_6F2C_46D3_B73D_65553DAA461A__INCLUDED_)
