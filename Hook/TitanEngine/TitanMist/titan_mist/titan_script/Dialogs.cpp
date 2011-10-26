#include "Dialogs.h"

#include <windows.h>
#include "resource.h"
#include "globals.h"

const char GlobalTitle[] = "TitanScript";

INT_PTR CALLBACK ASKDialogProc(HWND, UINT, WPARAM, LPARAM);

bool DialogASK(const string& title, string& input)
{
	char* returned_buffer = (char*)DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_INPUT), hwnd, &ASKDialogProc, (LPARAM)title.c_str());
	if(returned_buffer)
	{
		input = returned_buffer;
		delete[] returned_buffer;
		return true;
	}
	return false;
}

bool DialogMSG(const string& content, int& input)
{
	string title = string(GlobalTitle) + " [MSG]";
	input = MessageBox(hwnd, content.c_str(), title.c_str(), MB_ICONINFORMATION | MB_OKCANCEL);
	return true;
}

bool DialogMSGYN(const string& content, int& input)
{
	string title = string(GlobalTitle) + " [MSGYN]";
	input = MessageBox(hwnd, content.c_str(), title.c_str(), MB_ICONQUESTION | MB_YESNOCANCEL);
	return true;
}

INT_PTR CALLBACK ASKDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) 
	{
	case WM_INITDIALOG:
		{
		string title = string(GlobalTitle) + " [ASK]";
		SetWindowText(hwndDlg, title.c_str());
		SetDlgItemText(hwndDlg, IDC_TITLE, (char*)lParam);
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) 
		{
		case IDOK:
			size_t len;
			len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_INPUT));
			if(len++)
			{
				char* buffer = new char[len];
				if(!GetDlgItemText(hwndDlg, IDC_INPUT, buffer, len))
				{
					delete[] buffer;
					buffer = 0;
				}
				EndDialog(hwndDlg, (INT_PTR)buffer);
				break;
			}
		case IDCANCEL: 
			EndDialog(hwndDlg, 0);
			break; 
		}
		break;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;

	default:
		return false;
	}

	return true; 
}

void MsgInfo(const string& sMsg)
{
	MessageBox(hwnd, sMsg.c_str(), GlobalTitle, MB_OK | MB_ICONINFORMATION);
}

void MsgWarning(const string& sMsg)
{
	MessageBox(hwnd, sMsg.c_str(), GlobalTitle, MB_OK | MB_ICONEXCLAMATION);
}

void MsgError(const string& sMsg)
{
	MessageBox(hwnd, sMsg.c_str(), GlobalTitle, MB_OK | MB_ICONERROR);
}
