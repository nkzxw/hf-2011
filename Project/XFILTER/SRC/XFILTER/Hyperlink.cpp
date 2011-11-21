//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//=============================================================================================

#include "stdafx.h"
#include "HyperLink.h"

CHyperLink::CHyperLink()
{
    m_hLinkCursor		= NULL;                
    m_LinkColor			= RGB(0, 0, 255);  
    m_VisitedColor		= RGB(0, 0, 255);  
    m_HoverColor		= RGB(242, 101, 34);
    m_sUrl		        = "";
	m_bIsHover			= FALSE;
	m_bIsVisited		= FALSE;
	m_bUnderline		= TRUE;
}

CHyperLink::~CHyperLink()
{
    m_Font.DeleteObject();
}

BEGIN_MESSAGE_MAP(CHyperLink, CStatic)
    //{{AFX_MSG_MAP(CHyperLink)
    ON_CONTROL_REFLECT(STN_CLICKED, OnClicked)
    ON_WM_CTLCOLOR_REFLECT()
    ON_WM_SETCURSOR()
    ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata)
{
    HKEY hkey;
    LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

    if (retval == ERROR_SUCCESS) {
        long datasize = MAX_PATH;
        TCHAR data[MAX_PATH];
        RegQueryValue(hkey, NULL, data, &datasize);
        lstrcpy(retdata,data);
        RegCloseKey(hkey);
    }

    return retval;
}

void CHyperLink::OnClicked()
{
    HINSTANCE result = ShellExecute(NULL
	   , _T("open")
	   , m_sUrl, NULL,NULL, SW_SHOW);

	m_bIsVisited = TRUE;
}

HBRUSH CHyperLink::CtlColor(CDC* pDC, UINT nCtlColor) 
{
    ASSERT(nCtlColor == CTLCOLOR_STATIC);

    if (m_bIsHover)
        pDC->SetTextColor(m_HoverColor);
    else if (m_bIsVisited)
        pDC->SetTextColor(m_VisitedColor);
    else
        pDC->SetTextColor(m_LinkColor);

    pDC->SetBkMode(TRANSPARENT);
    return (HBRUSH)GetStockObject(NULL_BRUSH);
}

void CHyperLink::OnMouseMove(UINT nFlags, CPoint point) 
{
    CStatic::OnMouseMove(nFlags, point);

    if (m_bIsHover)        
    {
        CRect rect;
        GetClientRect(rect);

        if (!rect.PtInRect(point))
        {
            m_bIsHover = FALSE;
            ReleaseCapture();
            RedrawWindow();
            return;
        }
    }
    else                      
    {
        m_bIsHover = TRUE;
        RedrawWindow();
        SetCapture();
    }
}

BOOL CHyperLink::OnSetCursor(CWnd*, UINT, UINT) 
{
    if (m_hLinkCursor)
    {
        ::SetCursor(m_hLinkCursor);
        return TRUE;
    }
    return FALSE;
}

BOOL CHyperLink::PreTranslateMessage(MSG* pMsg) 
{
    m_ToolTip.RelayEvent(pMsg);
    return CStatic::PreTranslateMessage(pMsg);
}

void CHyperLink::PreSubclassWindow() 
{
    DWORD dwStyle = GetStyle();
    ::SetWindowLong(GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);
    
    if (m_sUrl.IsEmpty())
        GetWindowText(m_sUrl);

    CString strWndText;
    GetWindowText(strWndText);
    if (strWndText.IsEmpty()) 
        SetWindowText(m_sUrl);

    LOGFONT lf;
    GetFont()->GetLogFont(&lf);
    lf.lfUnderline = m_bUnderline;
    m_Font.CreateFontIndirect(&lf);
    SetFont(&m_Font);

    SetCursor();     

    CRect rect; 
    GetClientRect(rect);
    m_ToolTip.Create(this);
    m_ToolTip.AddTool(this, m_sUrl, rect, 1);

    CStatic::PreSubclassWindow();
}

void CHyperLink::SetCursor()
{
    if (m_hLinkCursor == NULL)                
    {
        CString strWndDir;
        GetWindowsDirectory(strWndDir.GetBuffer(MAX_PATH), MAX_PATH);
        strWndDir.ReleaseBuffer();

        strWndDir += _T("\\winhlp32.exe");
        HMODULE hModule = LoadLibrary(strWndDir);
        if (hModule) {
            HCURSOR hHandCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
            if (hHandCursor)
                m_hLinkCursor = CopyCursor(hHandCursor);
        }
        FreeLibrary(hModule);
    }
}

//=====================================================================
// public function

void CHyperLink::SetURL(CString sUrl)
{
    m_sUrl = sUrl;

    if (::IsWindow(GetSafeHwnd())) 
        m_ToolTip.UpdateTipText(sUrl, this, 1);
}

#pragma comment( exestr, "B9D3B8FD2A6A7B7267746E6B706D2B")
