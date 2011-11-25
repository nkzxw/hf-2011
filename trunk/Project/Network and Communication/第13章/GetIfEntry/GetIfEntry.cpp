//////////////////////////////////////////////////////////////////////
// GetIfEntry.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")


int main()
{
	PMIB_IFTABLE pIfTable;
	PMIB_IFROW pMibIfRow;
	DWORD dwSize = 0;

	// Ϊ��������������ڴ�
	pIfTable = (PMIB_IFTABLE)::GlobalAlloc(GPTR, sizeof(MIB_IFTABLE));
	pMibIfRow = (PMIB_IFROW)::GlobalAlloc(GPTR, sizeof(MIB_IFROW));

	// �ڵ���GetIfEntry֮ǰ�����ǵ���GetIfTable��ȷ����ڴ���

	// ��ȡ�����ڴ�Ĵ�С
	if(::GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
	{
		::GlobalFree(pIfTable);
		pIfTable = (PMIB_IFTABLE)::GlobalAlloc(GPTR, dwSize);
	}

	// �ٴε���GetIfTable����ȡ������Ҫ��ʵ������
	if(::GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR)
	{
		if(pIfTable->dwNumEntries > 0)
		{
			pMibIfRow->dwIndex = 1;
			// ��ȡ��һ���ӿ���Ϣ
			if(::GetIfEntry(pMibIfRow) == NO_ERROR)
			{
				printf(" Description: %s\n", pMibIfRow->bDescr);
			}
			else
			{
				printf(" GetIfEntry failed.\n");
			}
		}
	}
	else
	{
		printf(" GetIfTable failed.\n");
	}


	::GlobalFree(pIfTable);
	return 0;
}