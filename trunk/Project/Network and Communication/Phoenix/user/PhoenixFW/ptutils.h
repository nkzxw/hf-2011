////////////////////////////////////////////////////////
// ptutils.h�ļ�

#ifndef __PTUTILS_H__
#define __PTUTILS_H__

HANDLE PtOpenControlDevice();
BOOL PtEnumerateBindings(HANDLE hDriver, TCHAR *pszBuffer, DWORD *pdwBufferLength);

HANDLE PtOpenAdapter(PWSTR pszAdapterName);
BOOL PtAdapterRequest(HANDLE hAdapter, PPTUSERIO_OID_DATA pOidData, BOOL bQuery);

BOOL PtQueryStatistics(HANDLE hAdapter, PPassthruStatistics pStats);
BOOL PtResetStatistics(HANDLE hAdapter);

BOOL PtAddFilter(HANDLE hAdapter, PPassthruFilter pFilter);
BOOL PtClearFilter(HANDLE hAdapter);

//////////////////////////////////

BOOL IMClearRules();
BOOL IMSetRules(PPassthruFilter pRules, int nRuleCount);


#define MAX_ADAPTERS 10

class CIMAdapters		// ptutils.h�ļ�
{
public:
	// ö��IM�󶨵�������
	BOOL EnumAdapters(HANDLE hControlDevice);
	int m_nAdapters;
	LPWSTR m_pwszAdapterName[MAX_ADAPTERS];
	LPWSTR m_pwszVirtualName[MAX_ADAPTERS];
protected:
	char m_buffer[MAX_ADAPTERS*256];
};


#endif //  __PTUTILS_H__