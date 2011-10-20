#pragma once
#include "DrvFltIp.H"
typedef unsigned long   uint32;
#define _WINVER_NT4_    0x0004
#define _WINVER_95_     0x0004
#define _WINVER_98_     0x0A04
#define _WINVER_ME_     0x5A04
#define _WINVER_2K_     0x0005
#define _WINVER_XP_     0x0105


class CNetDefenderRules : public CObject
{
public:
	int	m_nProtocol;			//protocol used
	int	m_nSourceIp;			//source ip address
	int	m_nDestinationIp;		//destination ip address
	int	m_nSourceMask;			//source mask
	int	m_nDestinationMask;		//destination mask

public:
	int	m_nSourcePort;			//source port
public:
	int	m_nDestinationPort;		//destination port
	BOOLEAN m_bDrop;			//if true, the packet will be drop, otherwise the packet pass

	DECLARE_SERIAL( CNetDefenderRules )
	// empty constructor is necessary
	CNetDefenderRules(){};
	CNetDefenderRules( const CNetDefenderRules& rules ) // Copy constructor
	{
		m_nProtocol = rules.m_nProtocol; 
		m_nSourceIp = rules.m_nSourceIp; 
		m_nDestinationIp = rules.m_nDestinationIp; 
		m_nSourceMask = rules.m_nSourceMask; 
		m_nDestinationMask = rules.m_nDestinationMask; 
		m_nSourcePort = rules.m_nSourcePort; 
		m_nDestinationPort = rules.m_nDestinationPort; 
		m_bDrop = rules.m_bDrop; 
	} 
	const CNetDefenderRules& operator=( const CNetDefenderRules& rules )
	{
		m_nProtocol = rules.m_nProtocol; 
		m_nSourceIp = rules.m_nSourceIp; 
		m_nDestinationIp = rules.m_nDestinationIp; 
		m_nSourceMask = rules.m_nSourceMask; 
		m_nDestinationMask = rules.m_nDestinationMask; 
		m_nSourcePort = rules.m_nSourcePort; 
		m_nDestinationPort = rules.m_nDestinationPort; 
		m_bDrop = rules.m_bDrop; 
		return *this;
	}


	void Serialize( CArchive& archive );
	void SetIpFilter(IPFilter& filterObj);
	IPFilter GetIpFilter();

};




WORD                    DetectWinVersion();
BOOL                    IsRunningXPSP2();

///////////////////////////////////////////////////////////////////////////////
// Error strings, Debugging, Logging
//
int                     GetSystemErrorString(DWORD dwError, CString &rstrError);
int                     GetModuleErrorString(DWORD dwError, CString &rstrError, LPCTSTR pszModule);
int                     GetErrorMessage(DWORD dwError, CString &rstrErrorMsg, DWORD dwFlags = 0);
CString                 GetErrorMessage(DWORD dwError, DWORD dwFlags = 0);
BOOL                    RegularExpressionMatch(CString regexpr, CString teststring);
BOOL                    IsUnicodeFile(LPCTSTR pszFilePath);
BOOL                    HaveAdminAccess();

//Simple encryption
CString                 CryptString(const CString &strNormal);
CString                 DecryptString(const CString &strCrypted);
CString                 LongToIPStr(UINT nIpAddress);
CString                 PortNoToStr(USHORT nPortNo);
CString                 intToPotocol(int nPortNo);
CString                 BoolToAction(BOOL bAction);
CString                 GetCurrentIp();
CString GiveDescIp(UINT nIpAddress);
