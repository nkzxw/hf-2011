// CheckModuleDlg.h : ͷ�ļ�
//

#pragma once

#include <vector>
#include "afxwin.h"
using namespace std ;

// CCheckModuleDlg �Ի���
class CCheckModuleDlg : public CDialog
{
// ����
public:
	CCheckModuleDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_CHECKMODULE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


private:
	vector<CString>	ModuleVect ;

public:
	VOID	InitModuleVect () ;
	BOOL	IsModuleValid ( CString szModuleName ) ;
public:
	afx_msg void OnBnClickedFlush();
public:
	afx_msg void OnBnClickedCheck();
};
