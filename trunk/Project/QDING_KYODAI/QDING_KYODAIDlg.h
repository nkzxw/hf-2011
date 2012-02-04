// QDING_KYODAIDlg.h : 头文件
//

#pragma once


// CQDING_KYODAIDlg 对话框
class CQDING_KYODAIDlg : public CDialog
{
// 构造
public:
	CQDING_KYODAIDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_QDING_KYODAI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
