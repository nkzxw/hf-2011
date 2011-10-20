/**
* Copyright(c) 2003  NetWall Technology Co.,Ltd.
*
* Module Name  : $Id: RuleUtil.cpp,v 1.0.0 2003/03/04 01:47:15 Exp $
*
* Abstract     : Ndis Intermediate Miniport driver. 
*
* Author       : yjchen
*
* Environment  : Windows 2K
*
* Function List:
*
*   GetProtocolId            
*   GetProtocolDescByIndex   
*   GetProtocolDescById      
*   GetActionIdByDesc     
*   GetDescByActionIndex   
*   GetDescByActionId    
*   GetDirectionIdByDesc     
*   GetDescByDirectionIndex  
*   GetDescByDirectionId
*   GetIpRange
*   GetPortRange
*   
* Revision History:
*   27-Feb-2003	   1.0.0	   creation, initial version
*   03-Mar-2003	   1.0.1	   Create a Makefile Project to make use of VC's IDE
*   03-Mar-2003    1.0.2       Add Filter Struct
*   04-Mar-2003    1.0.3       Add Filter management
*   12-Mar-2003    1.0.4       Change Htonl/Htons to Macro HTONL/HTONS
*/
#include "stdafx.h"
#include "netwall.h"
#include "RuleUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

PROTO_DESC	g_Protocol[NW_MAX_PROTOCOL]	=       { { PROTOCOL_ARP,   "ARP"  },
                                                  { IPPROTO_ICMP,   "ICMP" },
                                                  { IPPROTO_IGMP,   "IGMP" },
                                                  { IPPROTO_TCP,    "TCP"  },
                                                  { IPPROTO_UDP,    "UDP"  }
                                                };

ACTION_DESC	g_Action[NW_MAX_ACTION]		=       { { NETWALL_ACTION_PASS,  "通过" },
                                                  { NETWALL_ACTION_DROP,  "拒绝" },
                                                  { NETWALL_ACTION_LOG,   "日志" }
                                                };

DIRECTION_DESC	g_Direction[NW_MAX_DIRECTION] = { { NETWALL_DIRECTION_IN,  "进"  },
                                                  { NETWALL_DIRECTION_OUT, "出"  },
                                                  { NETWALL_DIRECTION_BOTH,"双向"}
                                                };


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CRuleUtil, CObject)

CRuleUtil::CRuleUtil() : CObject()
{
}

CRuleUtil::~CRuleUtil()
{
}


int	CRuleUtil::GetProtocolId(char * lpszProto) 
{
    int i = 0;

	for (i = 0; i < NW_MAX_PROTOCOL; i++) 
    {
		if (!stricmp(lpszProto, g_Protocol[i].Name)) 
        {
            return g_Protocol[i].iProto;		
        }
	}
	
	return 0;
}

char * CRuleUtil::GetProtocolDescByIndex(int iIndex) 
{
	return g_Protocol[iIndex].Name;
}

char * CRuleUtil::GetProtocolDescById(int iId)
{
    int i = 0;

	for (i = 0; i < NW_MAX_PROTOCOL; i++) 
    {
		if (iId == g_Protocol[i].iProto) 
        {
            return g_Protocol[i].Name;
        }
	}

	return NULL;
}

int	CRuleUtil::GetActionIdByDesc(char * lpszAction)
{
    int i = 0;

	for (i = 0; i < NW_MAX_ACTION; i++) 
    {
		if (!stricmp(lpszAction, g_Action[i].Name)) 
        {
            return g_Action[i].iAction;
        }
	}

	return 0;
}

char * CRuleUtil::GetDescByActionIndex(int iIndex)
{
	return g_Action[iIndex].Name;
}

char * CRuleUtil::GetDescByActionId(int iAction) 
{
    int i = 0;

	for (i = 0; i < NW_MAX_ACTION; i++) 
    {
		if (g_Action[i].iAction == iAction) 
        {
            return g_Action[i].Name;
        }
	}

	return NULL;
}

int	CRuleUtil::GetDirectionIdByDesc(char * lpszDirection)
{
    int i = 0;

	for (i = 0; i < NW_MAX_DIRECTION; i++) 
    {
		if (!stricmp(g_Direction[i].Name, lpszDirection)) 
        {
            return g_Direction[i].iDirection;
        }
	}

	return 0;
}

char * CRuleUtil::GetDescByDirectionIndex(int iIndex)
{
	return g_Direction[iIndex].Name;
}

char *  CRuleUtil::GetDescByDirectionId(int iDirection)
{
    int i = 0;

	for (i = 0; i < NW_MAX_DIRECTION; i++) 
    {
		if (g_Direction[i].iDirection == iDirection) 
        {
            return g_Direction[i].Name;
        }
	}

	return NULL;
}

BOOLEAN CRuleUtil::GetIpRange(const char * lpRange, ULONG * pStart, ULONG * pEnd) {
	
	char	buffer[MAX_PATH];
	char *	ptr;

	if (!stricmp(lpRange, "Any")) 
    {
		*pStart = 0;
		*pEnd	= 0xFFFFFFFF;
	} 
    else 
    {
		strcpy(buffer, lpRange);
		ptr = strrchr(buffer, ':');
		if (ptr) 
        {
			*ptr = 0;
			*pStart = inet_addr(buffer);
			*pEnd	= inet_addr(ptr+1);
		} 
        else 
        {
			*pStart	= inet_addr(buffer);
			*pEnd	= *pStart;
		}
	}

	if (*pStart == INADDR_NONE || *pEnd == INADDR_NONE) 
    {
        return FALSE;
    }

	return TRUE;
}

BOOLEAN CRuleUtil::GetPortRange(const char * lpRange, USHORT * pStart, USHORT * pEnd) {
	if (!stricmp(lpRange, "Any")) 
    {
		*pStart	= 0;
		*pEnd	= 0xFFFF;
	} 
    else 
    {
		char buffer[MAX_PATH];
		strcpy(buffer, lpRange);
		char * ptr = strrchr(buffer, '\\');
		
		if (ptr) 
        {
			*ptr = 0;
			*pStart	= atoi(buffer);
			*pEnd	= atoi(ptr + 1);
		} 
        else 
        {
			*pStart	= atoi(buffer);
			*pEnd	= *pStart;
		}
	}

	return FALSE;
}



BOOL CRuleUtil::CreateRuleFileHead(RULE_FILE_HEADER *pRuleHead)
{
    RtlZeroMemory(pRuleHead, RULE_FILE_HEADER_SIZE);
    
    time_t  ltime;
    time(&ltime);

    pRuleHead->Signature        = NETWALL_RULE_SIGNATURE;
    pRuleHead->dwVersion        = ((CNetWallApp *)AfxGetApp())->m_nAppAPIVersion;
    pRuleHead->dwTotal          = RULE_FILE_HEADER_SIZE;            
    pRuleHead->TimeDateStamp    = ltime;//CTime::GetCurrentTime().GetTime();
    pRuleHead->NumberOfSections = 0;

    return TRUE;
}
