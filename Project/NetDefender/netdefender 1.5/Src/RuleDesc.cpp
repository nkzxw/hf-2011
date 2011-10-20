#include "stdafx.h"
#include "RuleDesc.h"
#include "Fire.h"

CRuleDesc::CRuleDesc(CNetDefenderRules* pRules)
{
	m_pRules = pRules;
	
}

CRuleDesc::~CRuleDesc(void)
{
	m_pRules = NULL;
}
CString CRuleDesc::GetProtocol()
{
	CString strTemp;
	m_strProtocol = intToPotocol(m_pRules->m_nProtocol);
	strTemp.Format(_T("<b>%s</b>"),GetProtocolDesc(m_strProtocol));
	
	return strTemp ;
}
CString CRuleDesc::GetSrcIp()
{
	CString strTemp;
	m_strSourceIp = LongToIPStr(m_pRules->m_nSourceIp);
	strTemp.Format(_T("<b>%s</b>"),GetIpDesc(m_strSourceIp));
	return strTemp ;
}
CString CRuleDesc::GetDestIp()
{
	CString strTemp;
	m_strDestinationIp = LongToIPStr(m_pRules->m_nDestinationIp);
	strTemp.Format(_T("<b>%s</b>"),GetIpDesc(m_strDestinationIp));
	return strTemp ;
}
CString CRuleDesc::GetSrcMask()
{
	CString strTemp;
	m_strSourceMask = LongToIPStr(m_pRules->m_nSourceMask);
	strTemp.Format(_T("<b>%s</b>"),m_strSourceMask);
	return strTemp ;
}
CString CRuleDesc::GetDestMask()
{
	CString strTemp;
	m_strDestinationMask = LongToIPStr(m_pRules->m_nDestinationMask);
	strTemp.Format(_T("<b>%s</b>"),m_strDestinationMask);
	return strTemp ;
}
CString CRuleDesc::GetSrcPort()
{
	CString strTemp;
	m_strSourcePort= PortNoToStr(static_cast<USHORT>(m_pRules->m_nSourcePort));
	strTemp.Format(_T("<b>%s</b>"),GetPortDesc(m_strSourcePort));
	return strTemp ;
}
CString CRuleDesc::GetDestPort()
{
	CString strTemp;
	m_strDestinationPort= PortNoToStr(static_cast<USHORT>(m_pRules->m_nDestinationPort));
	strTemp.Format(_T("<b>%s</b>"),GetPortDesc(m_strDestinationPort));
	return strTemp ;
}
CString CRuleDesc::GetAction()
{
	CString strTemp;
	m_strAction = BoolToAction(m_pRules->m_bDrop);
	strTemp.Format(_T("<b>%s</b>"),m_strAction);
	return strTemp ;
}
CString CRuleDesc::GetIpDesc(CString strIp)
{
	CString strMyIp = GetCurrentIp();
	if(strIp.CompareNoCase(_T("ALL"))==0)
	{
		strIp.Format(_T("Any machine"));
	}
	else if(strIp.CompareNoCase(strMyIp)==0)
	{
		strIp.Format(_T("My machine"));
	}
	else
	{
		strIp.Format(_T("machine with Ip address: %s"),strIp);
	}
	return strIp;

}
CString CRuleDesc::GetPortDesc(CString strPortNo)
{
	if(strPortNo.CompareNoCase(_T("Any"))==0)
	{
		strPortNo.Format(_T("Any Port"));
	}
	else
	{
		CString strTemp;
		CFireApp *pFireApp = (CFireApp*)AfxGetApp();
		if(pFireApp->m_portInfo.ReturnPortInfo(strPortNo,strTemp))
		{
			strPortNo.Format(_T("Port No: %s <font color=blue> ( Port Info :</font></b> <i><font color=navy>%s</font></i><b>)"),strPortNo,strTemp);
		}
		else
		{
			strPortNo.Format(_T("Port No: %s"),strPortNo);
		}
		
	}
	return strPortNo;

}
CString CRuleDesc::GetProtocolDesc(CString strProtocol)
{
	strProtocol.Format(_T("%s protocol"),strProtocol);
	return strProtocol;
}
CString	CRuleDesc::GetRuleDesc()
{
	CString strDesc;
	strDesc.Format(_T("%s all network traffic from <br>%s on %s<br>to %s on %s <br>for the %s"),GetAction(), 
		GetSrcIp(),GetSrcPort(),GetDestIp(),GetDestPort(),GetProtocol());
	return strDesc;
}