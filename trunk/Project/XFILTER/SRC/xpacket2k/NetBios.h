//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/28
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
//

#ifndef __NET_BIOS__
#define __NET_BIOS__

//#define Sleep(x)	Time_Slice_Sleep(x)
#define Sleep(x)	return FALSE;

#define NETBIOS_HEADER_LENTH				12
#define NETBIOS_QUESTION_NAME_LENTH			4
#define NETBIOS_MIN_PACKET_SIZE				48

#define RFLAGS_REQUEST						0x0
#define RFLAGS_RESPOINSE					0x1

#define OPCODE_REQUEST_REGISTRATION			0x5	// 0101
#define OPCODE_REQUEST_RELEASE				0x6	// 0110
#define OPCODE_REQUEST_REFRESH				0x8	// 1000
#define OPCODE_RESPONSE_QUERY				0x0	// 0000
#define OPCODE_RESPONSE_REGISTRATION		0x5	// 0101
#define OPCODE_RESPONSE_RELEASE				0x8	// 1000

#define NMFLAGS_NAME_REGISTRATION_REQUEST	0x11 // 0010001
#define NMFLAGS_NAME_OVERWRITE_REQUEST		0x01 // 0000001

#define RCODE_POSITIVE						0x0

typedef struct 
{
	WORD RCode				: 4;
	WORD NmFlags			: 7;
	WORD OpCode				: 4;
	WORD RFlags				: 1;
} HEADER_FLAGS, *PHEADER_FLAGS;

typedef struct _NETBIOS_HEADER
{
	WORD	NameTrnId;
	HEADER_FLAGS HeaderFlags;
	WORD	QdCount;
	WORD	AnCount;
	WORD	NsCount;
	WORD	ArCount;	
} NETBIOS_HEADER, *PNETBIOS_HEADER;

typedef struct _QUESTION_NAME
{
	WORD	QuestionType;
	WORD	QuestionClass;
} QUESTION_NAME, *PQUESTION_NAME;

typedef struct _RESOURCE_RECORD
{
	WORD	RrType;
	WORD	RrClass;
	DWORD	Ttl;
	WORD	RdLenth;
	WORD	NbFlags;
	DWORD	NbAddress;
} RESOURCE_RECORD, *PRESOURCE_RECORD;




extern BOOL Lock(BOOL *bLocked, int *RefenceCount);
extern void UnLock(BOOL *bLocked);
extern BOOL RefenceCount(BOOL *bLocked, int *RefenceCount);
extern void DeRefenceCount(int *RefenceCount);



extern PNAME_LIST	m_pFirstNameList;
extern BOOL MakeNameList(char *pNetBiosHeader);
extern BOOL GetNameFromIp(DWORD dwIp, char* pBuffer);
extern PNAME_LIST GetNameList();
extern DWORD GetNameListEx(char* pBuffer, DWORD nSize);
extern DWORD GetIpFromName(char* pBuffer);

int ParseName(
	IN	char *pCompressedName, 
	OUT char *pUnCompressedName
	);
BOOL FindName(IN char *pName, OUT PNAME_LIST* ppListReturn);
BOOL FindNameEx(IN char *pName, OUT PNAME_LIST* ppListReturn);
BOOL AddName(char* pName, DWORD dwIp);
BOOL AddNameEx(char* pName, DWORD dwIp);
BOOL DeleteName(char* pName);
BOOL RefreshName(char *pName, char *pNewName, DWORD dwIp);
BOOL RefreshNameEx(char *pName, char *pNewName, DWORD dwIp);
BOOL FreeNameList();


#endif //__NET_BIOS__

