//////////////////////////////////////////////////////////////////////
// ChangeGateway.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "WS2_32.lib")


PMIB_IPFORWARDTABLE MyGetIpForwardTable(BOOL bOrder);
void MyFreeIpForwardTable(PMIB_IPFORWARDTABLE pIpRouteTab);
void PrintIpForwardTable();

int main()
{
	// �����ص�ַ
	DWORD dwNewGateway = ::inet_addr("192.168.0.1");

	// �ڱ��в���������Ҫ����ڡ�Ĭ�����ص�Ŀ�ĵ�ַΪ0.0.0.0
	PMIB_IPFORWARDTABLE pIpRouteTable = MyGetIpForwardTable(TRUE);
	PMIB_IPFORWARDROW pRow = NULL;
	if(pIpRouteTable != NULL)
	{
		for(DWORD i=0; i<pIpRouteTable->dwNumEntries; i++)
		{
			if(pIpRouteTable->table[i].dwForwardDest == 0)	// �ҵ���Ĭ������
			{
				// �����ڴ������������ڡ�
				// ����Լ����MIB_IPFORWARDROW�ṹ����࣬���ǽ���Ҫ�ı����ص�ַ
				if(pRow == NULL)
				{
					pRow = (PMIB_IPFORWARDROW)::GlobalAlloc(GPTR, sizeof(MIB_IPFORWARDROW));
					memcpy(pRow, &pIpRouteTable->table[i], sizeof(MIB_IPFORWARDROW));
				}

				// ɾ���ɵ�Ĭ���������
				if(::DeleteIpForwardEntry(&pIpRouteTable->table[i]) != ERROR_SUCCESS)
				{
					printf("Could not delete old gateway \n");
					exit(1);
				}
			}
		}
		MyFreeIpForwardTable(pIpRouteTable);
	}

	if(pRow != NULL)
	{
		// ����dwForwardNextHop��Ϊ���ǵ������أ�����������·�����Խ�����ǰ����ͬ
		pRow->dwForwardNextHop = dwNewGateway;

		// ΪĬ�����ش����µ�·�����
		if(::SetIpForwardEntry(pRow) == NO_ERROR)
			printf(" Gateway changed successfully \n");
		else
			printf(" SetIpForwardEntry() failed \n");

		::GlobalFree(pRow);
	}

	return 0;
}

PMIB_IPFORWARDTABLE MyGetIpForwardTable(BOOL bOrder)
{
	PMIB_IPFORWARDTABLE pIpRouteTab = NULL;
    DWORD dwActualSize = 0;
    
    // ��ѯ���軺�����Ĵ�С
    if(::GetIpForwardTable(pIpRouteTab, &dwActualSize, bOrder) == ERROR_INSUFFICIENT_BUFFER)
	{
        // ΪMIB_IPFORWARDTABLE�ṹ�����ڴ�
        pIpRouteTab = (PMIB_IPFORWARDTABLE)::GlobalAlloc(GPTR, dwActualSize);
        // ��ȡ·�ɱ�
		if(::GetIpForwardTable(pIpRouteTab, &dwActualSize, bOrder) == NO_ERROR)
			return pIpRouteTab;
		::GlobalFree(pIpRouteTab);
    }
	return NULL;
}

void MyFreeIpForwardTable(PMIB_IPFORWARDTABLE pIpRouteTab)
{
	if(pIpRouteTab != NULL)
		::GlobalFree(pIpRouteTab);
}