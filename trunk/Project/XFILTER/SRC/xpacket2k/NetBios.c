//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

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
// ��飺
//		NetBios���ֽ�����غ�����Ϊ�˵õ������ھӵ�������Ҫ��
//		NetBios���ַ�����н�����
//		����ά��һ�����������������������ھӵ�IP��ַ�����֡�
//		Win9x�£�Ӧ�ò�������ͨ��DeviceIoControl�õ������
//		��ʼ��ַ��������ı�����ѯ��
//		Win2000�¿���ͨ��DeviceIoControlֱ��ͨ��Ip��ѯ����
//		����ͨ�����Ʋ�ѯIP��
//


#include "xprecomp.h"
#pragma hdrstop

PNAME_LIST	m_pFirstNameList = NULL;
int			m_iRefenceCount = 0;
BOOL		m_bLocked = FALSE;

//
// ͬ������������
//	��ɾ��������ʱ����Ҫ������������ʱ����������������
//	���RefenceCount > 0 ��ʾ��������ʹ�ã�����ʧ�ܡ�
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
// ����
//
void UnLock(BOOL *bLocked)
{
	if(*bLocked) *bLocked = FALSE;
}

//
// ʹ������ļ�������һ������������״̬ʱ������ʹ��������ȡҲ������
//
BOOL RefenceCount(BOOL *bLocked, int *RefenceCount)
{
	if(*bLocked)
		return FALSE;
	(*RefenceCount) ++;
	return TRUE;
}

//
// ʹ������ļ�������һ
//
void DeRefenceCount(int *RefenceCount)
{
	(*RefenceCount) --;
}

//
// �õ������б�������ַ
//
PNAME_LIST GetNameList()
{
	return m_pFirstNameList;
}

//
// �������б�����м�¼���Ƶ� pBuffer ������
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
// ͨ�����ֲ�ѯIP��ַ
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
// ͨ��Ip��ѯ����
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
// ����NetBios���ַ��񣬹��ڷ���ṹ�����RFC1002, 1001
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
		// ���������
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
			// �Ǽ���������
			//
			if(pHeaderFlags->NmFlags == NMFLAGS_NAME_REGISTRATION_REQUEST)
				return AddName(pBuffer, pRr->NbAddress);
			else if(pHeaderFlags->NmFlags == NMFLAGS_NAME_OVERWRITE_REQUEST)
				goto UPDATE_RECODE;
			else
				return FALSE;
		case OPCODE_REQUEST_RELEASE:
			//
			// ɾ����������
			//
			return DeleteName(pBuffer);
		case OPCODE_REQUEST_REFRESH:
			//
			// ˢ������
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
		// Ӧ�������
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
			// ��ѯӦ��
			//
		case OPCODE_RESPONSE_REGISTRATION:
			//
			// �Ǽ�Ӧ��
			//
			return AddName(pBuffer, pRr->NbAddress);
		case OPCODE_RESPONSE_RELEASE:
			//
			// ɾ��Ӧ��
			//
			return DeleteName(pBuffer);
		default:
			return FALSE;
		}
	}

	return TRUE;
}

//
// �������֣����ڷ���ṹ�����RFC1002, 1001
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
// �������ֲ��Ҽ�¼�������е�λ�á�
// ����ֵ��
//		TRUE: �ҵ���*ppListReturn �����ҵ���¼����һ����¼��Ҳ����˵��
//			*ppListReturn->pNext���ҵ��ļ�¼�����*ppListReturn == 0xFFFFFFFF
//			���ʾ�ҵ��ļ�¼λ������ͷ����m_pFirstNameList
//		FALSE: û���ҵ�
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
// FindName���Ӻ���
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
// ����һ����¼���ڴ�ռ䣬�������¼��ֵ��
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
// ������������һ����¼��׷�ӵ�����β����
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
// Add Name ���Ӻ���
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
// ���б���ɾ��һ����¼
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
// ˢ��һ����¼������
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
// RefreshName���Ӻ���
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
// �ͷ��б�����м�¼�Լ��ڴ�ռ�
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
