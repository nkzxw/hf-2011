// QDING_KYODAIDlg.h : ͷ�ļ�
//

#pragma once


// CQDING_KYODAIDlg �Ի���
class CQDING_KYODAIDlg : public CDialog
{
// ����
public:
	CQDING_KYODAIDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_QDING_KYODAI_DIALOG };

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
	CString m_data;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedPracButton();
	afx_msg void OnBnClickedFirstButton();
	afx_msg void OnBnClickedExec();
};
