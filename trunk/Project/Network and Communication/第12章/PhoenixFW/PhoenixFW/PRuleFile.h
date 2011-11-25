//////////////////////////////////////
// PRuleFile.h

// �����ļ�

#include "../common/TypeStruct.h"
#include "../common/PMacRes.h"

//  �ӽ�������ļ����������浽CPRuleFile�����С������෴�Ķ���
// Phoenix Personal firewall 

#ifndef __PRULEFILE_H__
#define __PRULEFILE_H__

class CPRuleFile
{
public:
	CPRuleFile();
	~CPRuleFile();

	// ���ļ����ع��˹���
	BOOL LoadRules();
	// �����˹��򱣴浽�ļ�
	BOOL SaveRules();

	// ���nCount��Ӧ�ò㣨���Ĳ㣩���˹���
	BOOL AddLspRules(RULE_ITEM *pItem, int nCount);
	BOOL AddKerRules(PassthruFilter *pItem, int nCount);

	// ɾ��һ��Ӧ�ò㣨���Ĳ㣩���˹���
	BOOL DelLspRule(int nIndex);
	BOOL DelKerRule(int nIndex);

	// �ļ�����
	RULE_FILE_HEADER m_header;		// �ļ�ͷ
	RULE_ITEM *m_pLspRules;			// Ӧ�ò����ָ��
	PassthruFilter *m_pKerRules;	// ���Ĳ����ָ��

private:
	// ��ʼ���ļ�����
	void InitFileData();
	// �򿪴����ļ������û�л��Զ������������г�ʼ��
	BOOL OpenFile();
	// �����򱣴浽ָ���ļ�
	BOOL WriteRules(TCHAR *pszPathName);

	HANDLE m_hFile;
	TCHAR m_szPathName[MAX_PATH];
	int m_nLspMaxCount;
	int m_nKerMaxCount;
	BOOL m_bLoad;
};

#endif // __PRULEFILE_H__