// GlobalAppDlg.h : ͷ�ļ�
//

#pragma once


// CGlobalAppDlg �Ի���
class CGlobalAppDlg : public CDialog
{
// ����
public:
	CGlobalAppDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_GLOBALAPP_DIALOG };

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
public:
	afx_msg void OnBnClickedHook();
	afx_msg void OnBnClickedUnhook();
	afx_msg void OnBnClickedImportHookButton();
	afx_msg void OnBnClickedInjectDllButton();
};
