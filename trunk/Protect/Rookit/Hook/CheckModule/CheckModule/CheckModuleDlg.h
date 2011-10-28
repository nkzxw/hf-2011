// CheckModuleDlg.h : 头文件
//

#pragma once

#include <vector>
#include "afxwin.h"
using namespace std ;

// CCheckModuleDlg 对话框
class CCheckModuleDlg : public CDialog
{
// 构造
public:
	CCheckModuleDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CHECKMODULE_DIALOG };

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
