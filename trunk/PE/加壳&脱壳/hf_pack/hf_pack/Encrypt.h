#ifndef _HF_ENCRYPT_H_
#define _HF_ENCRYPT_H_

int __stdcall AttachStart ();

void __stdcall AttachEnd ();

void __stdcall InitAttachData ();

void __stdcall AttachProc();

LRESULT __stdcall AttachWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DWORD __stdcall CalcCrc32(char *lpSource, int nLength);

void Encrypt(char *pchFileName, char *pchPassword, bool bBackup);

void checkMessageBox ();

int DlgEntry ();

#endif //_HF_ENCRYPT_H_