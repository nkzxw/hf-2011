// Base.h: interface for the CBase class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_BASE_H__7781CAF1_E76D_11D6_91CB_000039E099AC__INCLUDED_)
#define AFX_BASE_H__7781CAF1_E76D_11D6_91CB_000039E099AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "StdAfx.h"
#include "AddDefines.h"

class	CBase
{
public:
	CBase();
	virtual ~CBase();

//methods
private:
	CString								GetProcById4Other(DWORD dwPid);
	int									GetSysVer(void);
	static HMODULE						m_hModuleTH;
	static pCreateToolhelp32Snapshot	m_pToolhelp;
	static pProcess32First				m_pProc32First;
	static pProcess32Next				m_pProc32Next;

	static bool							m_bInit;

//imported method
public:
	static HMODULE								m_hModuleTcp;
	static HMODULE								m_hModuleUdp;
	static DWORD								m_dwLastError;
	static pAllocateAndGetTcpExTableFromStack	m_pGetTcpTableEx;
	static pAllocateAndGetUdpExTableFromStack	m_pGetUdpTableEx;
	DWORD										Initialize(void);
	DWORD										Unitialize(void);
	CString										GetProcById(DWORD dwPid);
	CString										GetSysPath(void);
	CString										GetProcPath(DWORD dwPid);
	CString										GetProcPathEx(DWORD dwPid);
};
#endif // !defined(AFX_BASE_H__7781CAF1_E76D_11D6_91CB_000039E099AC__INCLUDED_)
