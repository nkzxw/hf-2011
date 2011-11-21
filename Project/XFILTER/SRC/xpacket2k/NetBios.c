//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/28
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
// 简介：
//		NetBios名字解析相关函数，为了得到网上邻居的名字需要对
//		NetBios名字服务进行解析。
//		这里维护一个单向链表，用来保存网上邻居的IP地址和名字。
//		Win9x下，应用层程序可以通过DeviceIoControl得到链表的
//		起始地址进行链表的遍厉查询。
//		Win2000下可以通过DeviceIoControl直接通过Ip查询名称
//		或者通过名称查询IP。
//


#include "xprecomp.h"
#pragma hdrstop

PNAME_LIST	m_pFirstNameList = NULL;
int			m_iRefenceCount = 0;
BOOL		m_bLocked = FALSE;

//
// 同步处理，加锁。
//	当删除链表项时，需要进行锁定，这时不允许其它操作。
//	如果RefenceCount > 0 表示还有人在使用，加锁失败。
//
BOOL Lock(BOOL *bLocked, int *RefenceCount)
{
	if(*bLocked)
		return FALSE;
	*bLocked = TRUE;
	if(*RefenceCount > 0)
		return FALSE;
	return TRUE;
}

//
// 解锁
//
void UnLock(BOOL *bLocked)
{
	if(*bLocked) *bLocked = FALSE;
}

//
// 使用链表的计数器加一，当处于锁定状态时不允许使用链表，读取也不允许。
//
BOOL RefenceCount(BOOL *bLocked, int *RefenceCount)
{
	if(*bLocked)
		return FALSE;
	(*RefenceCount) ++;
	return TRUE;
}

//
// 使用链表的计数器减一
//
void DeRefenceCount(int *RefenceCount)
{
	(*RefenceCount) --;
}

//
// 得到名字列表的首项地址
//
PNAME_LIST GetNameList()
{
	return m_pFirstNameList;
}

//
// 将名字列表的所有记录复制到 pBuffer 缓冲区
//
DWORD GetNameListEx(char* pBuffer, DWORD nSize)
{
	DWORD nLength = 0;
	PNAME_LIST pList = m_pFirstNameList;
	WORD* pStrLen = NULL;

	if(pBuffer == NULL || nSize == 0)
		return 0;
	if(!RefenceCount(&m_bLocked, &m_iRefenceCount)) return 0;
	while(pList != NULL)
	{
		pStrLen = (WORD*)(pBuffer + nLength);
		*pStrLen = strlen(pList->Name) + 1;
		nLength += sizeof(WORD);
		if((nLength + (*pStrLen)) > nSize)
		{
			*pStrLen = 0;
			nLength -= sizeof(WORD);
			break;
		}
		strcpy(pBuffer + nLength, pList->Name);
		nLength += (*pStrLen);
		pList = pList->pNext;
	}
	DeRefenceCount(&m_iRefenceCount);

	return nLength;
}

//
// 通过名字查询IP地址
//
DWORD GetIpFromName(char* pBuffer)
{
	PNAME_LIST pFindName = NULL;
	if(pBuffer == NULL || pBuffer[0] == 0)
		return 0;

	if(!FindName(pBuffer, &pFindName))
		return 0;
	if(pFindName == NULL || m_pFirstNameList == NULL)
		return 0;
	if(pFindName == (PNAME_LIST)0xFFFFFFFF)
		pFindName = m_pFirstNameList;
	return pFindName->Address;
}

//
// 通过Ip查询名字
//
BOOL GetNameFromIp(DWORD dwIp, char* pBuffer)
{
	BOOL bIsFind = FALSE;
	PNAME_LIST pList = m_pFirstNameList;
	if(!RefenceCount(&m_bLocked, &m_iRefenceCount)) return FALSE;
	while(pList != NULL)
	{
		if(dwIp == pList->Address)
		{
			strcpy(pBuffer, pList->Name);
			bIsFind = TRUE;
			break;
		}
		pList = pList->pNext;
	}
	DeRefenceCount(&m_iRefenceCount);
	if(!bIsFind)
	{
		BYTE *pIp = (BYTE*)&dwIp;
		sprintf(pBuffer, "%u.%u.%u.%u", pIp[3], pIp[2], pIp[1], pIp[0]);
	}
	return bIsFind;
}

//
// 解析NetBios名字服务，关于封包结构请参阅RFC1002, 1001
//
BOOL MakeNameList(char *pNetBiosHeader)
{
	int iReturnCode;
	static char pBuffer[NETBIOS_NAME_MAX_LENTH];
	static BYTE bFlags[3];
	PHEADER_FLAGS pHeaderFlags = NULL;
	PNETBIOS_HEADER pHeader = (PNETBIOS_HEADER)pNetBiosHeader;
	char* pName = pNetBiosHeader + NETBIOS_HEADER_LENTH;
	int nNameLenth = strlen(pName) + 1;

	dprintf(("********* MakeNameList *********\n"));

	if(pNetBiosHeader == NULL) return FALSE;

	memcpy(bFlags, pNetBiosHeader + 2, 2);
	bFlags[2] = bFlags[0];
	bFlags[0] = bFlags[1];
	bFlags[1] = bFlags[2];

	pHeaderFlags = (PHEADER_FLAGS)bFlags;

	if(pHeaderFlags->RCode != RCODE_POSITIVE)
		return FALSE;

	if(pHeaderFlags->RFlags == RFLAGS_REQUEST)
	{
		//
		// 请求包处理
		//

		PQUESTION_NAME pQn = (PQUESTION_NAME)(pName + nNameLenth);
		char* pRrName = (char*)pQn + NETBIOS_QUESTION_NAME_LENTH;
		int nRrNameLenth = strlen(pRrName) + 1;
		PRESOURCE_RECORD pRr = (PRESOURCE_RECORD)(pRrName + nRrNameLenth);

		if(pHeader->QdCount != 0x0100 || pHeader->ArCount != 0x0100
			|| pQn->QuestionType != 0x2000
			|| pQn->QuestionClass != 0x0100
			|| pRr->RrType != 0x2000 
			|| pRr->RrClass != 0x0100
			|| (pRr->RdLenth << 8 == 0 && pRr->RdLenth >> 8 < 0x06)
			)
			return FALSE;

		iReturnCode = ParseName(pRrName, pBuffer);
		if(iReturnCode == -1)
			return FALSE;

		switch(pHeaderFlags->OpCode) 
		{
		case OPCODE_REQUEST_REGISTRATION:
			//
			// 登记名字请求
			//
			if(pHeaderFlags->NmFlags == NMFLAGS_NAME_REGISTRATION_REQUEST)
				return AddName(pBuffer, pRr->NbAddress);
			else if(pHeaderFlags->NmFlags == NMFLAGS_NAME_OVERWRITE_REQUEST)
				goto UPDATE_RECODE;
			else
				return FALSE;
		case OPCODE_REQUEST_RELEASE:
			//
			// 删除名字请求
			//
			return DeleteName(pBuffer);
		case OPCODE_REQUEST_REFRESH:
			//
			// 刷新请求
			//
UPDATE_RECODE:
			{
				static char pBuffer2[NETBIOS_NAME_MAX_LENTH];
				if(ParseName(pName, pBuffer2) == -1)
					return FALSE;
				return RefreshName(pBuffer2, pBuffer, pRr->NbAddress);
			}
		default:
			return FALSE;
		}
	}
	else
	{
		//
		// 应答包处理
		//

		PRESOURCE_RECORD pRr = (PRESOURCE_RECORD)(pName + nNameLenth);

		if( pHeader->AnCount != 0x0100 
			|| pRr->RrType != 0x2000 
			|| pRr->RrClass != 0x0100
			|| (pRr->RdLenth << 8 == 0 && pRr->RdLenth >> 8 < 0x06)
			)
			return FALSE;

		iReturnCode = ParseName(pName, pBuffer);
		if(iReturnCode == -1)
			return FALSE;

		switch(pHeaderFlags->OpCode) 
		{
		case OPCODE_RESPONSE_QUERY:
			//
			// 查询应答
			//
		case OPCODE_RESPONSE_REGISTRATION:
			//
			// 登记应答
			//
			return AddName(pBuffer, pRr->NbAddress);
		case OPCODE_RESPONSE_RELEASE:
			//
			// 删除应答
			//
			return DeleteName(pBuffer);
		default:
			return FALSE;
		}
	}

	return TRUE;
}

//
// 解析名字，关于封包结构请参阅RFC1002, 1001
//
int ParseName(
	IN	char *pCompressedName, 
	OUT char *pUnCompressedName
)
{
	static int nLenth, i, nCount;
	if(pCompressedName == NULL || pUnCompressedName == NULL) return -1;

	dprintf(("********> Parse Name ********* %s\n", pUnCompressedName));

	nLenth = pCompressedName[0];
	if(nLenth != 32) return -1;
	nCount = 0;
	for(i = 0; i < nLenth; i += 2)
	{
		pUnCompressedName[nCount++] = ((pCompressedName[i + 1] - 'A') << 4) 
										| (pCompressedName[i + 2] - 'A');
	}

	for(i = 15; i >= 0; i--)
	{
		if(pUnCompressedName[i] == ' ' || pUnCompressedName[i] == 0)
			pUnCompressedName[i] = 0;
		else
			break;
	}
	if(i == 0) return -1;
	nCount = i + 1;

	pCompressedName = pCompressedName + nLenth + 1; 
	nLenth = pCompressedName[0];
	while(nLenth != 0)
	{
		if((nCount + nLenth) > NETBIOS_NAME_MAX_LENTH) return nCount;
		pUnCompressedName[nCount++] = '.';
		memcpy(pUnCompressedName + nCount,  pCompressedName + 1, nLenth);
		nCount += nLenth;

		pCompressedName = pCompressedName + nLenth + 1; 
		nLenth = pCompressedName[0];
	}

	pUnCompressedName[nCount] = 0;
	
	dprintf(("<******** Parse Name ********* %s\n", pUnCompressedName));

	return nCount;
}

//
// 根据名字查找记录在链表中的位置。
// 返回值：
//		TRUE: 找到，*ppListReturn 保存找到记录的上一条记录，也就是说：
//			*ppListReturn->pNext是找到的记录。如果*ppListReturn == 0xFFFFFFFF
//			则表示找到的记录位于链表头，即m_pFirstNameList
//		FALSE: 没有找到
//
//
BOOL FindName(IN char *pName, OUT PNAME_LIST* ppListReturn)
{
	BOOL bRet = FALSE;
	if(!RefenceCount(&m_bLocked, &m_iRefenceCount)) return FALSE;
	bRet = FindNameEx(pName, ppListReturn);
	DeRefenceCount(&m_iRefenceCount);
	return bRet;
}
//
// FindName的子函数
//
BOOL FindNameEx(IN char *pName, OUT PNAME_LIST* ppListReturn)
{
	PNAME_LIST pListPrev;
	PNAME_LIST pListTemp;

	if(m_pFirstNameList == NULL)
	{
		*ppListReturn = NULL;
		return FALSE;
	}

	if(strcmp(pName, m_pFirstNameList->Name) == 0)
	{
		*ppListReturn = (PNAME_LIST)0xFFFFFFFF;
		return TRUE;
	}

	pListPrev = m_pFirstNameList;
	pListTemp = m_pFirstNameList->pNext;
	while(pListTemp != NULL)
	{
		if(strcmp(pName, pListTemp->Name) == 0)
		{
			*ppListReturn = pListPrev;
			return TRUE;
		}
		pListPrev = pListTemp;
		pListTemp = pListTemp->pNext;
	}

	*ppListReturn = pListPrev;
	return FALSE;
}

//
// 申请一条记录的内存空间，并填如记录的值。
//
PNAME_LIST MallocName(char* pName, DWORD dwIp)
{
	PNAME_LIST pList = (PNAME_LIST)malloc(NAME_LIST_LENTH);
	if(pList == NULL) return NULL;
	memset(pList, 0, NAME_LIST_LENTH);
	pList->Address = dwIp;
	strcpy(pList->Name, pName);
	return pList;
}

//
// 在链表中增加一条记录，追加到链表尾部。
//
BOOL AddName(char* pName, DWORD dwIp)
{
	BOOL bRet = FALSE;
	if(!RefenceCount(&m_bLocked, &m_iRefenceCount)) return FALSE;
	bRet = AddNameEx(pName, dwIp);
	DeRefenceCount(&m_iRefenceCount);
	return bRet;
}
//
// Add Name 的子函数
//
BOOL AddNameEx(char* pName, DWORD dwIp)
{
	PNAME_LIST pListPrev = NULL;

	dwIp = ntohl(dwIp);

	if(pName == NULL || pName[0] == 0 || strlen(pName) > 64) 
		return FALSE;

	dprintf(("******* Add Name ********* %s\n", pName));

	if(m_pFirstNameList == NULL)
	{
		m_pFirstNameList = MallocName(pName, dwIp);
		return TRUE;
	}

	if(FindName(pName, &pListPrev))
	{
		if(pListPrev == NULL)
			return FALSE;
		if(pListPrev == (PNAME_LIST)0xFFFFFFFF)
			pListPrev = m_pFirstNameList;
		else
			pListPrev = pListPrev->pNext;
		if(pListPrev->Address != dwIp)
			pListPrev->Address = dwIp;
		return TRUE;
	}

	if(pListPrev != NULL)
	{
		//if(!Lock(&m_bLocked, &m_iRefenceCount)) return FALSE;
		pListPrev->pNext = MallocName(pName, dwIp);
		//UnLock(&m_bLocked);
		return TRUE;
	}

	return FALSE;
}

//
// 从列表中删除一条记录
//
BOOL DeleteName(char* pName)
{
	PNAME_LIST pListTemp;
	PNAME_LIST pListPrev = NULL;

	dprintf(("******* Delete Name ********* %s\n", pName));

	if(!FindName(pName, &pListPrev) || pListPrev == NULL)
		return FALSE;

	if(!Lock(&m_bLocked, &m_iRefenceCount)) return FALSE;

	if(pListPrev == (PNAME_LIST)0xFFFFFFFF)
	{
		pListPrev = m_pFirstNameList;
		m_pFirstNameList = m_pFirstNameList->pNext;
		free(pListPrev);
		UnLock(&m_bLocked);
		return TRUE;
	}
	pListTemp = pListPrev->pNext;
	if(pListTemp == NULL)
		return FALSE;
	pListPrev->pNext = pListTemp->pNext;
	free(pListTemp);
	UnLock(&m_bLocked);
	return TRUE;
}

//
// 刷新一条记录的内容
//
BOOL RefreshName(char *pName, char *pNewName, DWORD dwIp)
{
	BOOL bRet = FALSE;
	if(!RefenceCount(&m_bLocked, &m_iRefenceCount)) return FALSE;
	bRet = RefreshNameEx(pName, pNewName, dwIp);
	DeRefenceCount(&m_iRefenceCount);
	return bRet;
}
//
// RefreshName的子函数
//
BOOL RefreshNameEx(char *pName, char *pNewName, DWORD dwIp)
{
	PNAME_LIST pListPrev = NULL;

	BYTE *pIp = (BYTE*)&dwIp;
	dprintf(("******* Refresh Name ********* %s to %s, %u.%u.%u.%u\n", pName, pNewName, pIp[0], pIp[1], pIp[2], pIp[3]));

	if(!FindName(pName, &pListPrev) || pListPrev == NULL || m_pFirstNameList == NULL)
		return FALSE;

	//if(!Lock(&m_bLocked, &m_iRefenceCount)) return FALSE;

	if(pListPrev == (PNAME_LIST)0xFFFFFFFF)
	{
		m_pFirstNameList->Address = dwIp;
		strcpy(m_pFirstNameList->Name, pNewName);
		//UnLock(&m_bLocked);
		return TRUE;
	}
	pListPrev = pListPrev->pNext;
	if(pListPrev == NULL)
		return FALSE;
	pListPrev->Address = dwIp;
	strcpy(pListPrev->Name, pNewName);
	//UnLock(&m_bLocked);
	return TRUE;
}

//
// 释放列表的所有记录以及内存空间
//
BOOL FreeNameList()
{
	PNAME_LIST pListTemp = NULL;
	PNAME_LIST pList = m_pFirstNameList;

	if(!Lock(&m_bLocked, &m_iRefenceCount)) return FALSE;

	m_pFirstNameList = NULL;

	while(pList != NULL)
	{
		pListTemp = pList->pNext;
		free(pList);
		pList = pListTemp;
	}

	UnLock(&m_bLocked);

	return TRUE;
}


#pragma comment( exestr, "B9D3B8FD2A706776646B71752B")
