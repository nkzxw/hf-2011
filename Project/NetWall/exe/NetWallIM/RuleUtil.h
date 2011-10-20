// RuleUtil.h: interface for the CRuleUtil class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RULEUTIL_H__CD64F24E_FBD2_4ABD_87E8_734375295669__INCLUDED_)
#define AFX_RULEUTIL_H__CD64F24E_FBD2_4ABD_87E8_734375295669__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
                            
const DWORD  NETWALL_RULE_SIGNATURE  = 0x0000574E;  // NW00 
const DWORD  NETWALL_RULE_SIGNATURE1 = 0x4E570000;  // NW00

#pragma pack(1)

//
// Rule header format.
//
typedef struct _RULE_FILE_HEADER 
{
    DWORD   Signature;
    DWORD   dwVersion;
    DWORD   dwTotal;    
    DWORD   TimeDateStamp;
    WORD    NumberOfSections;
    WORD    Reserved;
    
} RULE_FILE_HEADER, * PRULE_FILE_HEADER;

#define RULE_FILE_HEADER_SIZE               20

//
// Section header format.
//

#define LOWER_ADAPTER_NAME_LENGTH         64

typedef struct _RULE_SECTION_HEADER {
    BYTE    Name[LOWER_ADAPTER_NAME_LENGTH];
    DWORD   VirtualSize;
    DWORD   VirtualAddress;
    DWORD   NumberOfRules;
    DWORD   TimeDateStamp;
    DWORD   Characteristics;
    
} RULE_SECTION_HEADER, *PRULE_SECTION_HEADER;

#define RULE_SECTION_HEADER_SIZE             84


//
// Rule item.
//
/*
typedef struct _RULE_ITEM 
{
    UINT	cbSize;
    UINT	bUse;
    UINT	iProto;
    
    ULONG	ulSrcStartIp;
    ULONG	ulSrcEndIp;
    USHORT	usSrcStartPort;
    USHORT	usSrcEndPort;
    
    UCHAR	ucDirection;
    
    ULONG	ulDestStartIp;
    ULONG	ulDestEndIp;
    USHORT	usDestStartPort;
    USHORT	usDestEndPort;
    
    UCHAR	ucAction;
    char	chMsg[1];
    
} RULE_ITEM, * PRULE_ITEM;
*/
typedef struct _PROTO_DESC 
{
    int		iProto;
    char	Name[64];
    
} PROTO_DESC, *PPROTO_DESC;

typedef struct _ACTION_DESC 
{
    int		iAction;
    char	Name[64];
    
} ACTION_DESC, *PACTION_DESC;

typedef struct _DIRECTION_DESC 
{
    int		iDirection;
    char	Name[64];
    
} DIRECTION_DESC, *PDIRECTION_DESC;
#pragma pack()

class CRuleUtil : public CObject  
{
    DECLARE_DYNAMIC(CRuleUtil)

public:
	CRuleUtil();
	virtual ~CRuleUtil();

public:
	static BOOL CreateRuleFileHead(RULE_FILE_HEADER * pRuleHead);
    static int		GetProtocolId(char * lpszProto);
    static char *	GetProtocolDescByIndex(int iIndex);
    static char *	GetProtocolDescById(int iId);
    
    static int		GetActionIdByDesc(char * lpszAction);
    static char *	GetDescByActionIndex(int iIndex);
    static char *	GetDescByActionId(int iAction);
    
    static int		GetDirectionIdByDesc(char * lpszDirection);
    static char *  GetDescByDirectionIndex(int iIndex);
    static char *  GetDescByDirectionId(int iDirection);
    
    static BOOLEAN	GetIpRange(const char * lpRange, ULONG * pStart, ULONG * pEnd);
    static BOOLEAN	GetPortRange(const char * lpRange, USHORT * pStart, USHORT * pEnd);
};

#endif // !defined(AFX_RULEUTIL_H__CD64F24E_FBD2_4ABD_87E8_734375295669__INCLUDED_)
