/////////////////////////////////////////////////////////////////////////////
// SystemTray.h : header file
/////////////////////////////////////////////////////////////////////////////
#ifndef _INCLUDED_SYSTEMTRAY_H_
#define _INCLUDED_SYSTEMTRAY_H_

#define ASSUME_IE5_OR_ABOVE

#ifdef ASSUME_IE5_OR_ABOVE
#ifndef _WIN32_IE
#define _WIN32_IE	0x0500		// enable shell v5 features
#elif _WIN32_IE < 0x0500
#undef _WIN32_IE
#define _WIN32_IE	0x0500		// enable shell v5 features
#endif
#ifdef NOTIFYICONDATA_V1_SIZE	// If NOTIFYICONDATA_V1_SIZE, then we can use fun stuff
#define SYSTEMTRAY_USEW2K
#endif
#endif
#ifndef NIIF_NONE
#define NIIF_NONE	0
#endif

// #include <afxwin.h>
#include <afxtempl.h>
#include <afxdisp.h>			// COleDateTime
	
	/////////////////////////////////////////////////////////////////////////////

// CSystemTray window

/*********************************************
 * class CSystemTray: CSystemTray window
 *
 * @author 
 ********************************************/
class CSystemTray : public CWnd
{
// Construction/destruction
public:
	/*********************************************
     * Construction of class
     *
     * @return  
     ********************************************/
	CSystemTray();

	/*********************************************
  * Construction of class
  *
  * @param pWnd : The window that will recieve tray notifications
  * @param uCallbackMessage : the callback message to send to parent
  * @param szTip : tray icon tooltip
  * @param icon : Handle to icon
  * @param uID : Identifier of tray icon
  * @param bhidden : Hidden on creation?  
  * @param szBalloonTip : Ballon tip (w2k only)
  * @param szBalloonTitle : Balloon tip title (w2k)
  * @param dwBalloonIcon : Ballon tip icon (w2k)
  * @param uBalloonTimeout :  Balloon timeout (w2k)
  * @return  
  ********************************************/
	CSystemTray(CWnd *pWnd, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID, BOOL bhidden = FALSE,
				LPCTSTR szBalloonTip = NULL, LPCTSTR szBalloonTitle = NULL, DWORD dwBalloonIcon = NIIF_NONE,
				UINT uBalloonTimeout = 10);

	/*********************************************
     * destruction of class
	 * destructor automatically removes the icon from the tray.
     *
     * @return virtual 
     ********************************************/
	virtual ~CSystemTray();

	DECLARE_DYNAMIC(CSystemTray)
// Operations
public:
	BOOL Enabled()
	{
		return(m_bEnabled);
	}

	BOOL Visible()
	{
		return(!m_bHidden);
	}

	// Create the tray icon
	/*********************************************
 * Create the tray icon
 *
 * @return BOOL 
 ********************************************/
	BOOL	Create(CWnd *pParent, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID, BOOL bHidden = FALSE,
				   LPCTSTR szBalloonTip = NULL, LPCTSTR szBalloonTitle = NULL, DWORD dwBalloonIcon = NIIF_NONE,
				   UINT uBalloonTimeout = 10);

	// Change or retrieve the Tooltip text
	/*********************************************
     * Change  the Tooltip text
     *
     * @param pszTooltipText  Enter the text to set as toll tips
     * @return BOOL 
     ********************************************/
	BOOL	SetTooltipText(LPCTSTR pszTooltipText);

	/*********************************************
     * Change  the Tooltip text
     *
     * @param nID 
     * @return BOOL 
     ********************************************/
	BOOL	SetTooltipText(UINT nID);

	/*********************************************
     * retrieve  the Tooltip text
     *
     * @return CString 
     ********************************************/
	CString GetTooltipText() const;

	// Change or retrieve the icon displayed
	/*********************************************
     * Change  the icon displayed
     *
     * @param hIcon : handle of the Icon to be displayed
     * @return BOOL 
     ********************************************/
	BOOL	SetIcon(HICON hIcon);

	/*********************************************
     * Change  the icon displayed
     *
     * @param lpszIconName  : name of the Icon to be displayed
     * @return BOOL 
     ********************************************/
	BOOL	SetIcon(LPCTSTR lpszIconName);

	/*********************************************
     *  Change  the icon displayed
     *
     * @param nIDResource : Resource ID of the Icon to be displayed
     * @return BOOL 
     ********************************************/
	BOOL	SetIcon(UINT nIDResource);
	BOOL	SetStandardIcon(LPCTSTR lpIconName);
	BOOL	SetStandardIcon(UINT nIDResource);

	/*********************************************
     * retrieve the icon  to be displayed
     *
     * @return HICON 
     ********************************************/
	HICON	GetIcon() const;

	/*********************************************
     * Sets the focus to the icon (Win2000 only)
     *
     * @return void 
     ********************************************/
	void	SetFocus();

	/*********************************************
     * Hides but does not totally remove the icon
	 * from the tray. 
     *
     * @return BOOL 
     ********************************************/
	BOOL	HideIcon();

	/*********************************************
     * Show the Icon
     *
     * @return BOOL 
     ********************************************/
	BOOL	ShowIcon();

	/*********************************************
     * Redisplays a previously hidden icon
     *
     * @return BOOL 
     ********************************************/
	BOOL	AddIcon();

	/*********************************************
     * Removes the icon from the tray (icon can no 
	 * longer be manipulated)
     *
     * @return BOOL 
     ********************************************/
	BOOL	RemoveIcon();

	/*********************************************
     * Moves the icon to the far right of the tray, 
	 * so it is immediately to the left of  the clock 
     *
     * @return BOOL 
     ********************************************/
	BOOL	MoveToRight();

	/*********************************************
    * Shows the balloon tip (Win2000 only)
    ********************************************/
	BOOL	ShowBalloon(LPCTSTR szText, LPCTSTR szTitle = NULL, DWORD dwIcon = NIIF_NONE, UINT uTimeout = 10);

	// For icon animation
	/*********************************************
     * Set list of icons for animation 
     * @return BOOL 
     ********************************************/
	BOOL	SetIconList(UINT uFirstIconID, UINT uLastIconID);

	/*********************************************
     * Set list of icons for animation 
     * @return BOOL 
     ********************************************/
	BOOL	SetIconList(HICON *pHIconList, UINT nNumIcons);

	/*********************************************
     * Start animation
     *
     * @param nDelayMilliSeconds 
     * @param nNumSeconds 
     * @return BOOL 
     ********************************************/
	BOOL	Animate(UINT nDelayMilliSeconds, int nNumSeconds = -1);

	/*********************************************
     * Step to next icon
     *
     * @return BOOL 
     ********************************************/
	BOOL	StepAnimation();

	/*********************************************
     * Stop animation
     *
     * @return BOOL 
     ********************************************/
	BOOL	StopAnimation();

	// Change menu default item
	/*********************************************
     * Get default menu item
     * @return void 
     ********************************************/
	void	GetMenuDefaultItem(UINT &uItem, BOOL &bByPos);

	/*********************************************
     * Set default menu item
     * @return BOOL 
     ********************************************/
	BOOL	SetMenuDefaultItem(UINT uItem, BOOL bByPos);

	// Change or retrieve the window to send notification messages to
	/*********************************************
     *  Change the window to send notification messages to
     *
     * @param pNotifyWnd 
     * @return BOOL 
     ********************************************/
	BOOL	SetNotificationWnd(CWnd *pNotifyWnd);

	/*********************************************
     *  Retrieve the window to send notification messages to
     *
     * @return CWnd* 
     ********************************************/
	CWnd	*GetNotificationWnd() const;

	// Change or retrieve the window to send menu commands to
	/*********************************************
     * Change  the window to send menu commands to
     *
     * @return BOOL 
     ********************************************/
	BOOL	SetTargetWnd(CWnd *pTargetWnd);

	/*********************************************
     * Retrieve the window to send menu commands to
     *
     * @return CWnd* 
     ********************************************/
	CWnd	*GetTargetWnd() const;

	// Change or retrieve  notification messages sent to the window
	/*********************************************
     * Change notification messages sent to the window
     * @return BOOL 
     ********************************************/
	BOOL	SetCallbackMessage(UINT uCallbackMessage);

	/*********************************************
     * Retrieve  notification messages sent to the window
     *
     * @return UINT 
     ********************************************/
	UINT	GetCallbackMessage() const;

	UINT GetTimerID() const
	{
		return(m_nTimerID);
	}

// Static functions
public:
	/*********************************************
     * Minimize the application to System tray
     * @return static void 
     ********************************************/
	static void MinimiseToTray(CWnd *pWnd);

	/*********************************************
     * Minimize the application from System tray
     *
     * @param pWnd 
     * @return static void 
     ********************************************/
	static void MaximiseFromTray(CWnd *pWnd);
public:
	// Default handler for tray notification message
	/*********************************************
     * Default handler for tray notification message
     *
     * @return virtual LRESULT 
     ********************************************/
	virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemTray)
protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL
// Implementation
protected:
	/*********************************************
     * Intitialize the class to defaults values 
     *
     * @return void 
     ********************************************/
	void	Initialise();

	void	InstallIconPending();

// Implementation
protected:
	NOTIFYICONDATA			m_tnd;

	/*********************************************
     * does O/S support tray icon?
     ********************************************/
	BOOL					m_bEnabled;			// does O/S support tray icon?

	/*********************************************
     * Has the icon been hidden?
     ********************************************/
	BOOL					m_bHidden;			// Has the icon been hidden?

	/*********************************************
     * Has the icon been removed?
     ********************************************/
	BOOL					m_bRemoved;			// Has the icon been removed?

	/*********************************************
     * Show the icon once tha taskbar has been created
     ********************************************/
	BOOL					m_bShowIconPending; // Show the icon once tha taskbar has been created

	/*********************************************
     * Use new W2K features?
     ********************************************/
	BOOL					m_bWin2K;			// Use new W2K features?

	/*********************************************
	 * Window that menu commands are sent
	 ********************************************/
	CWnd					*m_pTargetWnd;		// Window that menu commands are sent
	CArray<HICON, HICON>	m_IconList;
	UINT					m_uIDTimer;
	int						m_nCurrentIcon;
	COleDateTime			m_StartTime;
	int						m_nAnimationPeriod;
	HICON					m_hSavedIcon;
	UINT					m_DefaultMenuItemID;
	BOOL					m_DefaultMenuItemByPos;
	UINT					m_uCreationFlags;

// Static data
protected:
	static BOOL			RemoveTaskbarIcon(CWnd *pWnd);

	static const UINT	m_nTimerID;
	static UINT			m_nMaxTooltipLength;
	static const UINT	m_nTaskbarCreatedMsg;
	static CWnd			m_wndInvisible;

	static BOOL			GetW2K();
#ifndef _WIN32_WCE
	static void			GetTrayWndRect(LPRECT lprect);
	static BOOL			GetDoWndAnimation();
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CSystemTray)
	afx_msg void	OnTimer(UINT nIDEvent);
	//}}AFX_MSG
#ifndef _WIN32_WCE
	afx_msg void	OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
#endif
	LRESULT			OnTaskbarCreated(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};
#endif

/////////////////////////////////////////////////////////////////////////////
