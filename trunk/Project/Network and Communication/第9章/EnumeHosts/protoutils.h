////////////////////////////////////////////
// protoutils.h�ļ�




#ifndef __PROTOUTILS_H__
#define __PROTOUTILS_H__

#include "nuiouser.h"

//////////////////////////////////////
// ��������

BOOL ProtoStartService();
void ProtoStopService();
HANDLE ProtoOpenControlDevice();


//////////////////////////////////////
// CPROTOAdapters�࣬ö�ٰ󶨵�������

#define MAX_ADAPTERS 10

class CPROTOAdapters
{
public:
	BOOL EnumAdapters(HANDLE hControlDevice);
	
	int m_nAdapters;
	LPWSTR m_pwszAdapterName[MAX_ADAPTERS];
	LPWSTR m_pwszSymbolicLink[MAX_ADAPTERS];
protected:
	char m_buffer[MAX_ADAPTERS*256];
};

////////////////////////////////////////
// CAdapter�࣬����󶨵�������

class CAdapter
{
public:
	CAdapter();
	~CAdapter();

	// �򿪡��ر�������
	BOOL OpenAdapter(LPCWSTR pwszSymbolicLink, BOOL bAsyn = FALSE);
	void CloseAdapter();

	// ���ù������ԣ���NDIS_PACKET_TYPE_PROMISCUOUS��NDIS_PACKET_TYPE_DIRECTED��
	BOOL SetFilter(ULONG nFilters);

	// ���ա���������
	int RecieveData(PVOID pBuffer, int nLen, LPOVERLAPPED lpOverlapped = NULL);
	int SendData(PVOID pBuffer, int nLen, LPOVERLAPPED lpOverlapped = NULL);

	// �����²�NIC������OID��Ϣ
	BOOL ResetAdapter();
	BOOL ProtoRequest(PPROTOCOL_OID_DATA pOidData, BOOL bQuery);

protected:
	HANDLE m_hAdapter;
};


#endif // __PROTOUTILS_H__