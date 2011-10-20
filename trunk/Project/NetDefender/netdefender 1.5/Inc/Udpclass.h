// UDPClass.h: interface for the CUDPClass class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_UDPCLASS_H__79B2923A_E26B_11D6_B451_0030052162DB__INCLUDED_)
#define AFX_UDPCLASS_H__79B2923A_E26B_11D6_B451_0030052162DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "StdAfx.h"
#include "Base.h"

class CUDPClass : public CBase
{
public:
	CUDPClass();
	virtual					~CUDPClass();

	//vars
	static PMIB_UDPTABLE_EX m_pBuffUdpTableEx;
	static DWORD			m_dwLastError;
private:
//methods
public:
	//methods
	DWORD	GetTableEx(void);
};
#endif // !defined(AFX_UDPCLASS_H__79B2923A_E26B_11D6_B451_0030052162DB__INCLUDED_)
