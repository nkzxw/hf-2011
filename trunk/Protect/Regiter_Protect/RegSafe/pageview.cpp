//pageview.cpp

#include "stdafx.h"
#include "pageview.h"

Cpageview::Cpageview()
{
  RootKey=HKEY_CURRENT_USER;
}
Cpageview::~Cpageview()
{
 
}
/*struct HKEY__ Cpageview::GetRootKey(UINT rID)
{
	struct HKEY__ *rkey;
	switch(rID)
	{
	case IDC_CHECK1:
		rkey=HKEY_CURRENT_USER;
	case IDC_CHECK2:
		rkey=HKEY_CURRENT_USER;
	}
	return rkey;
}*/
TCHAR * Cpageview::GetSubKey(UINT sID)
{
	TCHAR *skey;

	switch(sID)
	{
	case IDC_EDIT1:
		skey="Software\\Microsoft\\Internet Explorer\\Main";
		break;
	/*case :
        skey="";
		break;*/
	default:
    skey="Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";
	}
return skey;
}
TCHAR * Cpageview::GetVName(UINT vID)
{
	TCHAR *name;
	switch(vID)
	{
case IDC_P1CHECK1:
		name="NoLogOff";
		break;
case IDC_P1CHECK2:
		name="NoClose";
		break;
case IDC_P1CHECK3:
		name="NoRun";
		break;
case IDC_P1CHECK4:
		name="NoRecentDocsMenu";
		break;
case IDC_P1CHECK5:
		name="NoFavprotesMenu";
		break;
case IDC_P1CHECK6:
		name="NoSMHelp";
		break;
case IDC_P1CHECK7:
		name="NoFind";
		break;
case IDC_P1CHECK8:
		name="NoCommonGroups";
		break;
case IDC_P1CHECK9:
		name="NoControlPanel";
		break;
case IDC_P1CHECK10:
		name="NoWindowsUpdate";
		break;
case IDC_P1CHECK11:
		name="NoNetworkConnection";
		break;
case IDC_P1CHECK12:
		name="NoSetTaskbar";
		break;
case IDC_P1CHECK13:
		name="NoSetFolders";
		break;
case IDC_P1CHECK14:
		name="NoChangeStartMenu";
		break;
case IDC_EDIT1:
	    name="Start Page";
		break;
	}
	return name;
}
int Cpageview::DWORD_Getdate(UINT dID)
{
//RootKey=GetRootKey(dID);
SubKey=GetSubKey(dID);
VName=GetVName(dID);
m_reg.Open(RootKey,SubKey);
m_reg.ReadDWORD(VName,date,NULL);
if(date[0]==1)
   return 1;
else
   return 0;
}
void Cpageview::DWORD_SetKey(UINT skID,DWORD date)
{
    //RootKey=GetRootKey(skID);
	SubKey=GetSubKey(skID);
    VName=GetVName(skID);
    m_reg.Open(RootKey,SubKey);
    m_reg.WriteDWORD(VName,date,NULL);
}
CString Cpageview::CString_Getdate(UINT cgID)
{
char ch[256];
//RootKey=GetRootKey(cgID);
SubKey=GetSubKey(cgID);
VName=GetVName(cgID);
m_reg.Open(RootKey,SubKey);
m_reg.ReadString(VName,ch,256);
return ch;
}
void Cpageview::CString_SetKey(UINT csID,LPTSTR edit_ch)
{
SubKey=GetSubKey(csID);
VName=GetVName(csID);
m_reg.Open(RootKey,SubKey);
m_reg.WriteString(VName,edit_ch,NULL);
}