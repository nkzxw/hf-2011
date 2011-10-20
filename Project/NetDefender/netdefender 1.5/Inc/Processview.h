#pragma once
#include "afxcmn.h"
#include "UDPClass.h"
#include "TCPTable.h"
#include "ConnContainer.h"
#include "reportctrl.h"
#include "BtnST.h"
#include "Label.h"

#include <map>
#include "afxwin.h"
using std::					map;

typedef map<CString, DWORD> Ico2Index;

// CProcessView dialog
typedef enum				_SUBITEMS
{
	hProto			= 0x00,
	hPid			= 0x01,
	hProcessName,
	hLocalAddress,
	hLocalPort,
	hRemoteAddress,
	hRemotePort,
	hState
} SUBITEMS;

class CProcessView : public CDialog
{
	DECLARE_DYNAMIC(CProcessView)
public:
	CProcessView(CWnd *pParent = NULL);					// standard constructor
	virtual ~CProcessView();

	// Dialog Data
	enum
	{
		IDD			= IDD_PROCESS_VIEW
	};
protected:
	virtual void	DoDataExchange(CDataExchange *pDX); // DDX/DDV support
	CTCPTable		*m_pTcp;
	CUDPClass		*m_pUdp;

	CImageList		*m_pImageListMain;
	Ico2Index		m_mProcName2IcoIndex;

	DECLARE_MESSAGE_MAP()
public:
	CReportCtrl		m_process_ListCtrl;
	virtual BOOL	OnInitDialog();
	void			InitListAll();
	void			InitListTcpEx(bool exclusive);
	void			InitListUdpEx(bool exclusive);
	void			InitCtrlList();
	int				GetProcessIcon(CString strAppName, CString strPathAppName);
	CButtonST m_btnOK;
	CButtonST m_btnCancel;
	CLabel	m_lRunningAppMsg;
};
