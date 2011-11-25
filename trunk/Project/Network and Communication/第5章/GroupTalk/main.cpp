/////////////////////////////////////////////////
// main.cpp�ļ�

#include "GroupTalk.h"

#include <windows.h>
#include "resource.h"

BOOL __stdcall DlgProc(HWND, UINT, WPARAM, LPARAM);
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	int nResult = ::DialogBoxParam(
		hInstance,		// ʵ�����
		(LPCTSTR)IDD_MAIN,	// �Ի�����ԴID��
		NULL,			// �����ھ��
		DlgProc,		// ��Ϣ������
		NULL); 			// �Ի����ʼ����ֵ����WM_INITDIALOG��Ϣ��lParam������ȡ��

	return 0;
}

CGroupTalk *g_pTalk;

void HandleGroupMsg(HWND hDlg, GT_HDR *pHeader)
{
	switch(pHeader->gt_type)
	{
	case MT_JION:		// ���û�����
		{
			// ��ʾ���û�
			char szText[56];
			wsprintf(szText, " �û�����%s�����룡", pHeader->szUser);
			::SetWindowText(::GetDlgItem(hDlg, IDC_SYSMSG), szText);

			// �����û���Ϣ��ӵ��б����
			int nCurSel = ::SendDlgItemMessage(hDlg, IDC_USERS, CB_GETCURSEL, 0, 0);
			int nIndex = ::SendDlgItemMessage(hDlg, IDC_USERS, CB_ADDSTRING, 0, (long)pHeader->szUser);
			::SendDlgItemMessage(hDlg, IDC_USERS, CB_SETITEMDATA, nIndex, (long)pHeader->dwAddr);
			if(nCurSel == -1)
				nCurSel = nIndex;
			::SendDlgItemMessage(hDlg, IDC_USERS, CB_SETCURSEL, nCurSel, 0);
		}
		break;

	case MT_LEAVE:		// �û��뿪
		{
			// ��ʾ���û�
			char szText[56];
			wsprintf(szText, " �û�����%s���뿪��", pHeader->szUser);
			::SetWindowText(::GetDlgItem(hDlg, IDC_SYSMSG), szText);

			// ���뿪���û����б�����Ƴ�
			int nCount = ::SendDlgItemMessage(hDlg, IDC_USERS, CB_GETCOUNT, 0, 0);
			for(int i=0; i<nCount; i++)
			{
				int nIndex = ::SendDlgItemMessage(hDlg, IDC_USERS, CB_FINDSTRING, i, (long)pHeader->szUser);
				if((DWORD)::SendDlgItemMessage(hDlg, IDC_USERS, CB_GETITEMDATA, nIndex, 0) == pHeader->dwAddr)
				{
					::SendDlgItemMessage(hDlg, IDC_USERS, CB_DELETESTRING, nIndex, 0);
					break;
				}
			}	
		}
		break;

	case MT_MESG:		// �û�������Ϣ
		{
			char *psz = pHeader->data();
			psz[pHeader->nDataLength] = '\0';
			char szText[1024];
			wsprintf(szText, "��%s ˵��", pHeader->szUser);
			strncat(szText, psz, 1024 - strlen(szText));
			::SendDlgItemMessage(hDlg, IDC_RECORD, LB_INSERTSTRING, 0, (long)szText);
		}
		break;
	}
}

BOOL __stdcall DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{	
	case WM_INITDIALOG: 
		{
			// ����CGroupTalk����
			g_pTalk = new CGroupTalk(hDlg, ::inet_addr("234.5.6.7"));
			::CheckDlgButton(hDlg, IDC_SELGROUP, 1);
			::SendMessage(hDlg, WM_SETICON, ICON_SMALL, 
					(long)::LoadIcon(::GetModuleHandle(NULL), (LPCTSTR)IDI_MAIN));
		}
		break;

	case WM_GROUPTALK:
		{
			// ����CGroupTalk����������Ϣ
			if(wParam != 0) 
				::MessageBox(hDlg, (LPCTSTR)lParam, "����", 0);
			else
				HandleGroupMsg(hDlg, (GT_HDR*)lParam);
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_SEND:		// �û����·�����Ϣ��ť
			{
				// ȡ��Ҫ���͵���Ϣ
				char szText[1024];
				int nLen = ::GetWindowText(::GetDlgItem(hDlg, IDC_SENDMSG), szText, 1024);
				if(nLen == 0)
					break;

				// �������飬���������û���
				BOOL bToAll = ::IsDlgButtonChecked(hDlg, IDC_SELGROUP);
				DWORD dwAddr;
				if(bToAll)
				{
					dwAddr = 0;
				}
				else
				{
					int nIndex = ::SendDlgItemMessage(hDlg, IDC_USERS, CB_GETCURSEL, 0, 0);
					if(nIndex == -1)
					{
						::MessageBox(hDlg, "��ѡ��һ���û���", "GroupTalk", 0);
						break;
					}
					// ȡ���û�IP��ַ
					dwAddr = ::SendDlgItemMessage(hDlg, IDC_USERS, CB_GETITEMDATA, nIndex, 0);
				}
				// ������Ϣ
				if(g_pTalk->SendText(szText, nLen, dwAddr) == nLen)
						::SetWindowText(::GetDlgItem(hDlg, IDC_SENDMSG), "");
			}
			break;

		case IDC_CLEAR:		// �û����������ť
				::SendDlgItemMessage(hDlg, IDC_RECORD, LB_RESETCONTENT, 0, 0);
			break;

		case IDCANCEL:		// �û��رճ���
			{
				delete g_pTalk;
				::EndDialog (hDlg, IDCANCEL);
			}
			break;
		}
		break;
	}
	return 0;
}

