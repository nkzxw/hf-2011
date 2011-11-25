//////////////////////////////////////////////////////////////////////
// IPArp.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "WS2_32.lib")


PMIB_IPNETTABLE MyGetIpNetTable(BOOL bOrder);
void MyFreeIpNetTable(PMIB_IPNETTABLE pIpNetTable);
PMIB_IPADDRTABLE MyGetIpAddrTable(BOOL bOrder);
void MyFreeIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable);
BOOL InterfaceIdxToInterfaceIp(PMIB_IPADDRTABLE pIpAddrTable, DWORD dwIndex, char str[]);


// ����IP��ַ�����ӿ�����ת��ΪIP��ַ
// pIpAddrTable��IP��ַ��
// dwIndex�ǽӿ�����
// ����ִ�гɹ�֮��str�������ӿڵ�IP��ַ
BOOL InterfaceIdxToInterfaceIp(PMIB_IPADDRTABLE pIpAddrTable, DWORD dwIndex, char str[])
{
    char* szIpAddr;

    if(pIpAddrTable == NULL ||  str == NULL)
        return FALSE;
    str[0] = '\0';
	// ����IP��ַ����������dwIndex��Ӧ��IP��ַ
    for(DWORD dwIdx = 0; dwIdx < pIpAddrTable->dwNumEntries; dwIdx++)
    {
        if(dwIndex == pIpAddrTable->table[dwIdx].dwIndex)
        {
			// ���ַ�������ʽ���ز�ѯ���
            szIpAddr = inet_ntoa(*((in_addr*)&pIpAddrTable->table[dwIdx].dwAddr));
            if(szIpAddr)
            {
                strcpy(str, szIpAddr);
                return TRUE;
            }
            else
                return FALSE;
        }
    }
    return FALSE;
}

//----------------------------------------------------------------------------
//   ARP�������¸�ʽ��ӡ������
// Interface: 157.61.239.34 on Interface 2
//   Internet Address      Physical Address      Type
//   159.61.230.39         00-aa-00-61-5d-a4     dynamic
//
// Interface: 157.54.178.219 on Interface 3
//   Internet Address      Physical Address      Type
//   159.54.170.1          00-10-54-42-c0-88     dynamic
//   159.54.170.113        00-aa-00-c0-80-2e     dynamic
//----------------------------------------------------------------------------

int main()
{
    DWORD i, dwCurrIndex;
    char szPrintablePhysAddr[256];
    char szType[128];
    char szIpAddr[128];

	// ���Ȼ�ȡARP��
	PMIB_IPNETTABLE pIpNetTable = MyGetIpNetTable(TRUE); 
    if (pIpNetTable == NULL)
    {
        printf( "pIpNetTable == NULL in line %d\n", __LINE__);
        return -1;
    }

	// ��ȡIP��ַ���Ա��������ARP�����еĽӿ�����ת��ΪIP��ַ
	PMIB_IPADDRTABLE pIpAddrTable = MyGetIpAddrTable(TRUE);

	// ��ǰ��������������ע�⣬ARP��Ӧ�ð��սӿ���������
    dwCurrIndex = pIpNetTable->table[0].dwIndex;
    if(InterfaceIdxToInterfaceIp(pIpAddrTable, dwCurrIndex, szIpAddr))
    {
        printf("\nInterface: %s on Interface 0x%X\n", szIpAddr, dwCurrIndex);
        printf("  Internet Address      Physical Address      Type\n");
    }
    else
    {
        printf("Error: Could not convert Interface number 0x%X to IP address.\n",
                    pIpNetTable->table[0].dwIndex);
        return -1;
    }
    
	// ��ӡ������ΪdwCurrIndex���������ϵ�ARP����
    for(i = 0; i < pIpNetTable->dwNumEntries; ++i)
    {
		// �������˵��Ҫ��ӡ��һ���������ϵ�ARP������
        if(pIpNetTable->table[i].dwIndex != dwCurrIndex)
        {
            dwCurrIndex = pIpNetTable->table[i].dwIndex;
            if (InterfaceIdxToInterfaceIp(pIpAddrTable, dwCurrIndex, szIpAddr))
            {
                printf("Interface: %s on Interface 0x%X\n", szIpAddr, dwCurrIndex);
                printf("  Internet Address      Physical Address      Type\n");
            }
            else
            {
                printf("Error: Could not convert Interface number 0x%X to IP address.\n",
                    pIpNetTable->table[0].dwIndex);
                return -1;
            }
        }

			// ��ӡ����ARP�����е�����
		// MAC��ַ
		u_char *p = pIpNetTable->table[i].bPhysAddr;
        wsprintf(szPrintablePhysAddr, "%02X-%02X-%02X-%02X-%02X-%02X", p[0], p[1], p[2], p[3], p[4], p[5]);
		// IP��ַ
		struct in_addr inadTmp;
        inadTmp.s_addr = pIpNetTable->table[i].dwAddr;
		// ����
        switch (pIpNetTable->table[i].dwType)
        {
        case 1:
            strcpy(szType,"other");
            break;
        case 2:
            strcpy(szType,"invalidated");
            break;
        case 3:
            strcpy(szType,"dynamic");
            break;
        case 4: 
            strcpy(szType,"static");
            break;
        default:
            strcpy(szType,"invalidType");
        }
        printf("  %-16s      %-17s     %-11s\n", inet_ntoa(inadTmp), szPrintablePhysAddr, szType);
        
    }
	return 0;
}

// ��ȡIP��ַ����������ӳ���ϵ����ARP��

PMIB_IPNETTABLE MyGetIpNetTable(BOOL bOrder)
{
	PMIB_IPNETTABLE pIpNetTable = NULL;
	DWORD dwActualSize = 0;

	// ��ѯ���軺�����Ĵ�С
	if(::GetIpNetTable(pIpNetTable, 
					&dwActualSize, bOrder) == ERROR_INSUFFICIENT_BUFFER)
	{
		// ΪMIB_IPNETTABLE�ṹ�����ڴ�
		pIpNetTable = (PMIB_IPNETTABLE)::GlobalAlloc(GPTR, dwActualSize);
		// ��ȡARP��
		if(::GetIpNetTable(pIpNetTable, 
						&dwActualSize, bOrder) == NO_ERROR)
		{
			return pIpNetTable;
		}
		::GlobalFree(pIpNetTable);
	}
	return NULL;
}

void MyFreeIpNetTable(PMIB_IPNETTABLE pIpNetTable)
{
	if(pIpNetTable != NULL)
		::GlobalFree(pIpNetTable);
}


PMIB_IPADDRTABLE MyGetIpAddrTable(BOOL bOrder)
{
	PMIB_IPADDRTABLE pIpAddrTable = NULL;
	DWORD dwActualSize = 0;

	// ��ѯ���軺�����Ĵ�С
	if(::GetIpAddrTable(pIpAddrTable, 
					&dwActualSize, bOrder) == ERROR_INSUFFICIENT_BUFFER)
	{
		// ΪMIB_IPADDRTABLE�ṹ�����ڴ�
		pIpAddrTable = (PMIB_IPADDRTABLE)::GlobalAlloc(GPTR, dwActualSize);
		// ��ȡIP��ַ��
		if(::GetIpAddrTable(pIpAddrTable, 
						&dwActualSize, bOrder) == NO_ERROR)
			return pIpAddrTable;
		::GlobalFree(pIpAddrTable);
	}
	return NULL;
}

void MyFreeIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable)
{
	if(pIpAddrTable != NULL)
		::GlobalFree(pIpAddrTable);
}











/*







void PrintIpAddrTable()
{
    DWORD i;
    struct in_addr inadTmp1;
    struct in_addr inadTmp2;
    char szAddr[128];
    char szMask[128];
	
	PMIB_IPADDRTABLE pIpAddrTable = MyGetIpAddrTable(TRUE);

    if (pIpAddrTable == NULL)
    {
        printf( "pIpAddrTable == NULL in line %d\n", __LINE__);
        return;
    }
    printf("ipAdEntAddr\t ifAdEntIfIndex\t ipAdEntNetMask\t ipAdEntBcastAddr\t ipAdEntReasmMaxSize\n");
    for (i = 0; i < pIpAddrTable->dwNumEntries; ++i)
    {
        inadTmp1.s_addr = pIpAddrTable->table[i].dwAddr;
        strcpy(szAddr, inet_ntoa(inadTmp1));
        inadTmp2.s_addr = pIpAddrTable->table[i].dwMask;
        strcpy(szMask, inet_ntoa(inadTmp2));
        printf("  %s\t 0x%X\t %s\t %s\t %u\n",
                szAddr, 
                pIpAddrTable->table[i].dwIndex,
                szMask,
                (pIpAddrTable->table[i].dwBCastAddr ? "255.255.255.255" : "0.0.0.0"),
                pIpAddrTable->table[i].dwReasmSize);

    }

	MyFreeIpAddrTable(pIpAddrTable);
}



  */