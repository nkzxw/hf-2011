#pragma once
#include "commonFn.h"
class CRuleDesc
{
public:
	CRuleDesc(CNetDefenderRules* pRules);
	CNetDefenderRules* m_pRules;
	CString GetProtocol();
	CString GetSrcIp();
	CString GetDestIp();
	CString GetSrcMask();
	CString GetDestMask();
	CString GetSrcPort();
	CString GetDestPort();
	CString GetAction();
	CString	GetRuleDesc();
private:
	CString m_strProtocol;		//protocol used
	CString	m_strSourceIp;			//source ip address
	CString	m_strDestinationIp;		//destination ip address
	CString	m_strSourceMask;			//source mask
	CString	m_strDestinationMask;		//destination mask
	CString	m_strSourcePort;			//source port
	CString	m_strDestinationPort;		//destination port
	CString m_strAction;			//if true, the packet will be drop, otherwise the packet pass
	CString GetIpDesc(CString strIp);
	CString GetPortDesc(CString strPortNo);
	CString GetProtocolDesc(CString strProtocol);

public:
	virtual ~CRuleDesc(void);
};
