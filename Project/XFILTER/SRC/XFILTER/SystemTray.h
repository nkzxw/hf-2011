//=============================================================================================
/*
	SystemTray.h

	Project	: XFILTER 1.0
	Author	: Tony Zhu
	Create Date	: 2001/08/12
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#define WM_ICON_NOTIFY	WM_USER + 7
#define WM_ICON_SPLASH	WM_USER + 8

#define ICON_SPLASH_MESSAGE		0
#define ICON_SPLASH_ALERT		1

class CSystemTray 
{
public:
    CSystemTray();
    CSystemTray(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID);
    virtual ~CSystemTray();

public:
    BOOL Create(CWnd* pParent, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID);
    BOOL  SetIcon(UINT nIDResource);
    void  HideIcon();
    void  RemoveIcon();
    BOOL SetMenuDefaultItem(UINT uItem, BOOL bByPos);
    virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);
	UINT GetCurrentIconId(){return m_CurrentIconId;}

private:
	HICON			m_hIcon;

protected:
	void			Initialise();
    BOOL            m_bEnabled;   
    BOOL            m_bHidden;    
    NOTIFYICONDATA  m_tnd;
    UINT			m_DefaultMenuItemID;
    BOOL			m_DefaultMenuItemByPos;
	UINT			m_CurrentIconId;
};

#endif
