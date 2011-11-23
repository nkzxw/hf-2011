// GalaxyApMGR.h: interface for the GalaxyApMGR class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GALAXYAPMGR_H__7216C12F_5320_403B_8186_0EF23AD89338__INCLUDED_)
#define AFX_GALAXYAPMGR_H__7216C12F_5320_403B_8186_0EF23AD89338__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\TEST_KIDISPAT\TEST_KIDISPAT.h"

class GalaxyApMGR  
{
public:
	GalaxyApMGR();
	virtual ~GalaxyApMGR();


	ULONG	m_uNewOsBaseDelt;
	ULONG	m_OsBaseAddress;
	ULONG	m_uNewOsBase;
	DWORD	m_dwProtectPID;
	char szExceptionDriverName[128];
	char szReloadOsDriverName[128];
	char szStrongOD[128];
	PROTECT_INFO	m_protect_info;
	bool		m_bObtainedSyms;

	
	bool	m_bSetupKidisp;
	bool	m_bSetupReloadOS;
	//////////////////////////////
	bool	Initialize(bool binitAgain=false);

	bool	SetupMyKidispatchexcepion(char *pDriverName);
	bool	HookDIspatchException();
	bool	HookOldFuns();
	bool	FixExceptionDriverSymAddress();
	bool	SetProtectInfo();
	bool	HookNew2My();
	bool	SendReplace();

	bool	unHookKidisp();
	bool	unSetupMyKidispatchexcepion();

	bool	SetupReloadOS(char *pDriverName);
	bool	SendHookPort(DWORD dwIOcode);
	bool	UnSetupReloadOS();


private:
	bool SetNewOsAddress();
};

#endif // !defined(AFX_GALAXYAPMGR_H__7216C12F_5320_403B_8186_0EF23AD89338__INCLUDED_)
