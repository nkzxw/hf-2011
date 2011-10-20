// MainFrm.cpp : implementation of the CMainFrame class
//
#include "stdafx.h"
#include "fire.h"

#include "MainFrm.h"
#include "ProcessView.h"
#include "..\inc\mainfrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char				THIS_FILE[] = __FILE__;
#endif
#define WM_ICON_NOTIFY	WM_APP + 10
static const int		kImageWidth(48);
static const int		kImageHeight(48);
static const int		kNumImages(8);
static const UINT		kToolBarBitDepth(ILC_COLOR24);
static const RGBTRIPLE	kBackgroundColor = { 192, 192, 192 };
static const RGBTRIPLE	kMYBackgroundColor = { 0, 0, 0 };
static const RGBTRIPLE	kMYBackgroundColor2 = { 23, 23, 23 };
static const RGBTRIPLE	kMYBackgroundColor3 = { 168, 168, 168 };

/////////////////////////////////////////////////////////////////////////////
// CMainFrame
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_POPUP_EXIT, OnPopupExit)
	ON_COMMAND(ID_POPUP_MINIMIZE, OnPopupMinimize)
	ON_COMMAND(ID_POPUP_MAXIMIZE, OnPopupMaximize)
	ON_COMMAND(ID_EXIT, OnExit)
	ON_COMMAND(ID_ICON, OnIcon)
	ON_COMMAND(ID_TOOLS_PORTSCANNER, OnToolsPortscanner)
// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
//}}AFX_MSG_MAP
	ON_COMMAND(ID_TOOLS_BLOCKAPPLICATION, OnToolsBlockapplication)
	ON_COMMAND(ID_HELP_VISITNETDEFENDERHOMEPAGE, OnHelpVisitnetdefenderhomepage)
END_MESSAGE_MAP()
static UINT indicators[] =
{
	ID_SEPARATOR,	// status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
	ID_INDICATOR_ONLINELED,
	ID_INDICATOR_OFFLINELED
};

/////////////////////////////////////////////////////////////////////////////

// CMainFrame construction/destruction
CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CFrameWnd :: OnCreate(lpCreateStruct) == -1)
	{
		return(-1);
	}

	if(!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT,
	   WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
	   !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return(-1); // fail to create
	}

	// attach the hicolor bitmaps to the toolbar
	AttachToolbarImages(IDB_COLORTOOLBAR, IDB_TOOLBARDISABLE);

	try
	{

		if(!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
		{
			TRACE0("Failed to create status bar\n");
			return(-1); // fail to create
		}
	}
	catch(...)
	{
	}

	//********************************************************
	m_wndStatusBar.SetPaneInfo(0, m_wndStatusBar.GetItemID(0), SBPS_STRETCH, NULL);
	SetOnlineLed(FALSE);
	SetOfflineLed(TRUE);

	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_ONLINELED),
							   ID_INDICATOR_ONLINELED,
							   SBPS_NOBORDERS,
							   14);

	//		m_wndStatusBar.GetStatusBarCtrl().SetTipText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_ONLINELED), "This status light is green when the server is online");
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_OFFLINELED),
							   ID_INDICATOR_OFFLINELED,
							   SBPS_NOBORDERS,
							   14);

	//		m_wndStatusBar.GetStatusBarCtrl().SetTipText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_OFFLINELED), "This status light is green when the server is online");
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	//***************************************************************
	HICON	hIcon = :: LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MAINFRAME));	//Icon to be Used
	if(!m_SysTray.Create(NULL,	// Let icon deal with its own messages
	   WM_ICON_NOTIFY,			// Icon notify message to use
	   _T("Firewall Running"),	// tooltip
	   hIcon, IDR_MENU1,		// ID of tray Menu
	   FALSE))
	{	/**
	 * MinimiseToTray:Minimise the firewall to system tray
	 */
		CSystemTray :: MinimiseToTray(this);
	}
	m_menu.LoadMenu(IDR_MAINFRAME); // Load the main menu
	// Setup menu style
	m_menu.SetMenuDrawMode(BCMENU_DRAWMODE_XP); // <<<!=>>> BCMENU_DRAWMODE_ORIGINAL
	m_menu.SetSelectDisableMode(FALSE);
	m_menu.SetXPBitmap3D(TRUE);
	m_menu.SetBitmapBackground(RGB(255,0,255));
	m_menu.SetIconSize(16, 16);
	m_bMenu = SetMenu(&m_menu); ASSERT(m_bMenu == TRUE);
	CreateHiColorImageList(&m_pImgList, IDB_CLIENTICONS_EX, 16);
	m_menu.ModifyODMenu(NULL, (UINT)ID_EXIT, &m_pImgList, 1);
	m_menu.ModifyODMenu(NULL, (UINT)ID_TOOLS_PORTSCANNER, &m_pImgList, 31)   ;
	m_menu.ModifyODMenu(NULL, (UINT)ID_TOOLS_BLOCKAPPLICATION, &m_pImgList, 32)   ;
	m_menu.ModifyODMenu(NULL, (UINT)ID_HELP_VISITNETDEFENDERHOMEPAGE, &m_pImgList, 33)   ;
	m_menu.ModifyODMenu(NULL, (UINT)ID_HELP_FINDER, &m_pImgList, 45);
	
	m_menu.ModifyODMenu(NULL, (UINT)ID_APP_ABOUT, &m_pImgList,2);
	m_tooltip.Create(this);
	//Adds tooltip for toolbar
	m_tooltip.AddToolBar(&m_wndToolBar);
	m_tooltip.SetColorBk(RGB(255, 255, 255), RGB(240, 247, 255), RGB(192, 192, 208));
	m_tooltip.SetEffectBk(CPPDrawManager :: EFFECT_SOFTBUMP);
	m_tooltip.SetMaxTipWidth(400);
	m_tooltip.EnableEscapeSequences(TRUE);
	

	//	m_SysTray.SetMenuDefaultItem (ID_POPUP_MAXIMIZE,FALSE);
	return(0);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT &cs)
{
	if(!CFrameWnd :: PreCreateWindow(cs))
	{
		return(FALSE);
	}

	cs.style &= ~WS_THICKFRAME;
	cs.style &= ~WS_MAXIMIZEBOX;

	return(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics
#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd :: AssertValid();
}

void CMainFrame::Dump(CDumpContext &dc) const
{
	CFrameWnd :: Dump(dc);
}
#endif //_DEBUG
	
	/////////////////////////////////////////////////////////////////////////////

// CMainFrame message handlers

/**
 * OnClose: Called when the main screen of the filewall is closed
 *
 * @return void 
 */
void CMainFrame::OnClose()
{
	CSystemTray :: MinimiseToTray(this);
	m_SysTray.SetMenuDefaultItem(ID_POPUP_MAXIMIZE, FALSE);

	//CFrameWnd::OnClose();
}

/**
 * OnPopupExit:Called when the firewall is shutDown from pop up menu by clicking on System tray.
 * It will close the firewall application
 *
 * @return void 
 */
void CMainFrame::OnPopupExit()
{
	int i = AfxMessageBox(_T("Closing Firewall will disable its security engine and leave your computer\n unprotected.Are you sure you want to exit ?"),
					   MB_YESNO | MB_ICONQUESTION);
	if(i == IDYES)
	{
		CFrameWnd :: OnClose();
	}
	else
	{
	}
}

void CMainFrame::OnPopupMinimize()
{
	CSystemTray :: MinimiseToTray(this);
	m_SysTray.SetMenuDefaultItem(ID_POPUP_MAXIMIZE, FALSE);
}

void CMainFrame::OnPopupMaximize()
{
	// TODO: Add your command handler code here
	CSystemTray :: MaximiseFromTray(this);
	m_SysTray.SetMenuDefaultItem(ID_POPUP_MINIMIZE, FALSE);
}

void CMainFrame::OnExit()
{
	AfxMessageBox(_T("To Shutdown the firewall right click on the System Tray Icon & choose Shutdown from there"),MB_ICONINFORMATION);
	OnClose();
}

void CMainFrame::OnIcon()
{
	// TODO: Add your command handler code here
}

/**
 * OnToolsPortscanner: Event handler for menu -> tools -> Port Scanner
 * It wil show the port scanner dialog box
 *
 * @return void 
 */
void CMainFrame::OnToolsPortscanner()
{
	CPortScanDlg	m_PortScan;
	m_PortScan.DoModal();

	//WinExec ("PortScan.exe",SW_SHOW);
}

/**
 * SetOnlineLed:Turn online LED on/off.	
 *
 * @param bOnline 
 * @return void 
 */
void CMainFrame::SetOnlineLed(BOOL bOnline)
{
	HICON	hIcon = (HICON) :: LoadImage(AfxGetInstanceHandle(),
										 bOnline ? MAKEINTRESOURCE(IDI_LED_GREEN) : MAKEINTRESOURCE(IDI_LED_OFF),
										 IMAGE_ICON,
										 16,
										 16,
										 LR_SHARED);
	m_wndStatusBar.GetStatusBarCtrl().SetIcon(m_wndStatusBar.CommandToIndex(ID_INDICATOR_ONLINELED), hIcon);
	m_wndStatusBar.GetStatusBarCtrl().Invalidate();
	m_wndStatusBar.GetStatusBarCtrl().UpdateWindow();
	DestroyIcon(hIcon);
}

/********************************************************************/
/*																	*/
/* Function name : SetOfflineLed									*/
/* Description   : Turn offline LED on/off.							*/
/*																	*/

/********************************************************************/
void CMainFrame::SetOfflineLed(BOOL bOffline)
{
	HICON	hIcon = (HICON) :: LoadImage(AfxGetInstanceHandle(),
										 bOffline ? MAKEINTRESOURCE(IDI_LED_RED) : MAKEINTRESOURCE(IDI_LED_OFF),
										 IMAGE_ICON,
										 16,
										 16,
										 LR_SHARED);
	m_wndStatusBar.GetStatusBarCtrl().SetIcon(m_wndStatusBar.CommandToIndex(ID_INDICATOR_OFFLINELED), hIcon);
	m_wndStatusBar.GetStatusBarCtrl().Invalidate();
	m_wndStatusBar.GetStatusBarCtrl().UpdateWindow();
	DestroyIcon(hIcon);
}

void CMainFrame::OnToolsBlockapplication()
{
	CProcessView	m_ProcessView;
	m_ProcessView.DoModal();
}

// find every pixel of the default background color in the specified

// bitmap and set each one to the user's button color.
static void ReplaceBackgroundColor(CBitmap &ioBM)
{
	// figure out how many pixels there are in the bitmap
	BITMAP	bmInfo;

	VERIFY(ioBM.GetBitmap(&bmInfo));

	// add support for additional bit depths here if you choose
	VERIFY(bmInfo.bmBitsPixel == 24);
	VERIFY(bmInfo.bmWidthBytes == (bmInfo.bmWidth*3));

	const UINT	numPixels(bmInfo.bmHeight * bmInfo.bmWidth);

	// get a pointer to the pixels
	DIBSECTION	ds;

	VERIFY(ioBM.GetObject(sizeof(DIBSECTION), &ds) == sizeof(DIBSECTION));

	RGBTRIPLE	*pixels = reinterpret_cast < RGBTRIPLE * > (ds.dsBm.bmBits);
	VERIFY(pixels != NULL);

	// get the user's preferred button color from the system
	const COLORREF	buttonColor( :: GetSysColor(COLOR_BTNFACE));
	const RGBTRIPLE userBackgroundColor = { GetBValue(buttonColor), GetGValue(buttonColor), GetRValue(buttonColor) };

	// search through the pixels, substituting the user's button
	// color for any pixel that has the magic background color
	for(UINT i = 0; i < numPixels; ++i)
	{
		try
		{
			if((pixels[i].rgbtBlue == kBackgroundColor.rgbtBlue || pixels[i].rgbtBlue == kMYBackgroundColor.rgbtBlue ||
				pixels[i].rgbtBlue == kMYBackgroundColor2.rgbtBlue || pixels[i].rgbtBlue == kMYBackgroundColor3.rgbtBlue) &&
				(pixels[i].rgbtGreen == kBackgroundColor.rgbtGreen || pixels[i].rgbtGreen == kMYBackgroundColor.rgbtGreen || pixels[i].rgbtGreen == kMYBackgroundColor2.rgbtGreen || pixels[i].rgbtGreen == kMYBackgroundColor3.rgbtGreen
				) && (pixels[i].rgbtRed == kBackgroundColor.rgbtRed || pixels[i].rgbtRed == kMYBackgroundColor.rgbtRed ||
				pixels[i].rgbtRed == kMYBackgroundColor2.rgbtRed || pixels[i].rgbtRed == kMYBackgroundColor3.rgbtRed))
			{
				pixels[i] = userBackgroundColor;
			}
		}
		catch (...)
		{
			
		}
		
	}
}

// create an image list for the specified BMP resource
static void MakeToolbarImageList(UINT inBitmapID, CImageList &outImageList)
{
	CBitmap bm;

	// if we use CBitmap::LoadBitmap() to load the bitmap, the colors
	// will be reduced to the bit depth of the main screen and we won't
	// be able to access the pixels directly. To avoid those problems,
	// we'll load the bitmap as a DIBSection instead and attach the
	// DIBSection to the CBitmap.
	VERIFY(bm.Attach( :: LoadImage( :: AfxFindResourceHandle(MAKEINTRESOURCE(inBitmapID), RT_BITMAP),
								   MAKEINTRESOURCE(inBitmapID),
								   IMAGE_BITMAP,
								   0,
								   0,
								   (LR_DEFAULTSIZE | LR_CREATEDIBSECTION))));

	// replace the specified color in the bitmap with the user's
	// button color
	:: ReplaceBackgroundColor(bm);

	// create a 24 bit image list with the same dimensions and number
	// of buttons as the toolbar
	VERIFY(outImageList.Create(kImageWidth, kImageHeight, kToolBarBitDepth, kNumImages, 0));

	// attach the bitmap to the image list
	VERIFY(outImageList.Add(&bm, RGB(0, 0, 0)) != -1);
}

// load the high color toolbar images and attach them to m_wndToolBar
void CMainFrame::AttachToolbarImages(UINT inNormalImageID, UINT inDisabledImageID)
{
	// make high-color image lists for each of the bitmaps
	:: MakeToolbarImageList(inNormalImageID, m_ToolbarImages);
	:: MakeToolbarImageList(inDisabledImageID, m_ToolbarImagesDisabled);

	// get the toolbar control associated with the CToolbar object
	CToolBarCtrl	&barCtrl = m_wndToolBar.GetToolBarCtrl();

	// attach the image lists to the toolbar control
	barCtrl.SetImageList(&m_ToolbarImages);
	barCtrl.SetDisabledImageList(&m_ToolbarImagesDisabled);
}

void CMainFrame::OnHelpVisitnetdefenderhomepage()
{
	ShellExecute(NULL, _T("open"), NETDEFENDER_HOMEPAGE, NULL, NULL, SW_SHOW);
}
void CMainFrame::CreateHiColorImageList(CImageList *pImageList, WORD wResourceID, int czSize)
{
	ASSERT(pImageList != NULL); if(pImageList == NULL) return;

	CBitmap bmpImages;
	VERIFY(bmpImages.LoadBitmap(MAKEINTRESOURCE(wResourceID)));

	VERIFY(pImageList->Create(czSize, czSize, ILC_COLOR24 | ILC_MASK, bmpImages.GetBitmapDimension().cx / czSize, 0));
	pImageList->Add(&bmpImages, RGB(255,0,255));

	bmpImages.DeleteObject();
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	// must add RelayEvent function call to pass a mouse message to a tool tip control for processing.
	m_tooltip.RelayEvent(pMsg);
	return CFrameWnd::PreTranslateMessage(pMsg);
}
