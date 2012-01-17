#if !defined(PAGEVIEW_H)
#define PAGEVIEW_H


//pageview.h
#include "Resource.h"
#include "registry.h"

class Cpageview
{
public:

	Cpageview();
	~Cpageview();
	CRegistry m_reg;
    struct HKEY__ *RootKey;
    TCHAR *SubKey;
    TCHAR *VName;
    DWORD date[1];

    TCHAR * GetSubKey(UINT sID);
    TCHAR * GetVName(UINT vID);
	int DWORD_Getdate(UINT dID);
	void DWORD_SetKey(UINT skID,DWORD date);
    CString CString_Getdate(UINT cgID);
    void CString_SetKey(UINT csID,LPTSTR edit_ch);
};
#endif