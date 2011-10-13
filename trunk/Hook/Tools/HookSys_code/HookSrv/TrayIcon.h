//---------------------------------------------------------------------------
//
// TrayIcon.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook server
//
// DESCRIPTION: This code has been based on the Paul DiLascia's sample
//              Copyright 1996 Microsoft Systems Journal.
// 				
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------
#ifndef _TRAYICON_H_
#define _TRAYICON_H_

//---------------------------------------------------------------------------
//
// CTrayIcon manages an icon in the Windows 9x/NT/2K system tray. 
//
//---------------------------------------------------------------------------
class CTrayIcon : public CCmdTarget 
{
protected:
	DECLARE_DYNAMIC(CTrayIcon)
	NOTIFYICONDATA m_nid;			// struct for Shell_NotifyIcon args

public:
	CTrayIcon(UINT uID);
	~CTrayIcon();

	// Call this to receive tray notifications
	void SetNotificationWnd(CWnd* pNotifyWnd, UINT uCbMsg);
	//
	// SetIcon functions. To remove icon, call SetIcon(0)
	//
	BOOL SetIcon(UINT uID); // main variant you want to use
	BOOL SetIcon(HICON hicon, char* lpTip);
	BOOL SetIcon(LPCTSTR lpResName, char* lpTip)
	{ 
		return SetIcon(lpResName ? 
			AfxGetApp()->LoadIcon(lpResName) : NULL, lpTip); 
	}
	BOOL SetStandardIcon(LPCTSTR lpszIconName, char* lpTip)
	{ 
		return SetIcon(::LoadIcon(NULL, lpszIconName), lpTip); 
	}

	virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);
};

#endif
//----------------------------End of the file -------------------------------