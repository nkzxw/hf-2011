// ColorStatic.h

#ifndef COLORSTATIC_H
#define COLORSTATIC_H

#define COLOR_WHITE				RGB(255, 255, 255)
#define COLOR_RED				RGB(255, 0, 0)
#define COLOR_BLACK				RGB(0, 0, 0)
#define COLOR_GLOD				RGB(255, 204 ,0)
#define COLOR_GLOD_BLOD			RGB(255, 153 ,0)		
#define COLOR_GLOD_BLOD_Ex		RGB(255, 102 ,0)		
#define COLOR_BLUE				RGB(0, 0, 255)
#define COLOR_DEEP_BLUE			RGB(102, 102, 153)
#define COLOR_SKY_BLUE			RGB(147, 147, 232)
#define COLOR_DEEP_BLUE_EX		RGB(51, 51, 153)
#define COLOR_WHITE_YELLOW		RGB(255, 255, 204)
#define COLOR_WHITE_BLUE		RGB(204, 236, 255)
#define COLOR_WHITE_YELLOW_EX	RGB(255, 255, 153)
#define COLOR_GRAY				RGB(192, 192, 192)
#define COLOR_WHITE_GRAY		RGB(234, 234, 234)
#define COLOR_GRENN				RGB(0, 255, 0)

#define PASSECK_DIALOG_BKCOLOR	COLOR_DEEP_BLUE
#define COLOR_MAIN_NORMAL		COLOR_WHITE
#define COLOR_MAIN_FOCUS		COLOR_WHITE
#define COLOR_MAIN_SELECT		COLOR_GRAY
#define COLOR_DIALOG_BK			COLOR_DEEP_BLUE	
#define COLOR_LABLE_BK			PASSECK_DIALOG_BKCOLOR
#define COLOR_TEXT_NORMAL		COLOR_WHITE
#define COLOR_TEXT_FOCUS		COLOR_WHITE_GRAY
#define COLOR_TEXT_SELECT		COLOR_GRAY
#define COLOR_TEXT_LABLE		COLOR_WHITE
#define COLOR_BK_NORMAL			PASSECK_DIALOG_BKCOLOR
#define COLOR_BK_FOCUS			PASSECK_DIALOG_BKCOLOR
#define COLOR_BORDER			COLOR_WHITE
#define COLOR_BUTTON_SELECT_BK	PASSECK_DIALOG_BKCOLOR	
#define COLOR_LABLE_FG			COLOR_WHITE
#define COLOR_SHADOW			COLOR_BLACK
#define COLOR_LIST_BK			COLOR_SKY_BLUE
#define COLOR_LIST_TEXT			COLOR_WHITE
#define COLOR_TREE_BK			COLOR_WHITE
#define COLOR_TREE_TEXT			COLOR_BLACK
#define COLOR_TORJAN_BK			COLOR_WHITE_GRAY
#define COLOR_TORJAN_TEXT		COLOR_GLOD_BLOD_Ex
#define COLOR_DISABLE_TEXT		COLOR_TEXT_SELECT
#define COLOR_QUERY_BUTTON_BK	COLOR_SKY_BLUE

#define COLOR_BUTTON_EX_TEXT_NORMAL		COLOR_BLACK
#define COLOR_BUTTON_EX_TEXT_FOCUS		COLOR_BLACK
#define COLOR_BUTTON_EX_BK_NORMAL		COLOR_WHITE_GRAY
#define COLOR_BUTTON_EX_BK_FOCUS		COLOR_WHITE_GRAY

#define CHINESE_FONT_DEFAULT	"ËÎÌו"
#define ENGLISH_FONT_DEFUALT	"Arial Black"

#define DEFAULT_FONT			CHINESE_FONT_DEFAULT
#define DEFAULT_HEIGHT			-12
#define DEFAULT_HEIGHT_BIG		-14
#define DEFAULT_ITALIC			FALSE

class CColorStatic : public CStatic
{
public:
	CColorStatic(COLORREF nColor = COLOR_TEXT_LABLE);	
	void SetColor(COLORREF nColor = COLOR_TEXT_LABLE);
	void SetLogoFont(
		IN	CString Name,
		IN	int nHeight		= DEFAULT_HEIGHT,
		IN	int nWeight		= FW_NORMAL,
		IN	BYTE bItalic	= false,
		IN	BYTE bUnderline = false,
		IN  BOOL bIsChinese = TRUE
		);
	void SetTransParent(BOOL bIsTransParent){m_bIsTransParent = bIsTransParent;}
	void SetBkColor(COLORREF nBkColor){m_bIsTransParent = FALSE; m_nBkColor = nBkColor;}
	void SetLabel(UINT nID, CWnd *pParent);
	void SetLabelEx(UINT nID, CWnd *pParent);
	void SetLabelQuery(UINT nID, CWnd *pParent);

private:
	COLORREF	m_nBkColor;
	COLORREF	m_nColor;
	CFont		m_Font;
	BOOL		m_bIsTransParent;
protected:
    //{{AFX_MSG(CHyperLink)
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

class CBkStatic : public CStatic
{
public:
	CBkStatic(COLORREF m_nBkColor = COLOR_BLACK);	
	void SetBkColor(COLORREF nBkColor);

private:
	COLORREF	m_nBkColor;

protected:
    //{{AFX_MSG(CBkStatic)
	afx_msg void OnPaint();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#define DEFAULT_MAX_POS					10240
#define DEFAULT_POS_UNIT				3	
#define DEFAULT_GRID_UNIT				24
#define DEFAULT_MOVE_UNIT				12	

DWORD WINAPI RefreshGraph(PVOID pVoid);

class COsilloGraph : public CStatic
{
public:
	COsilloGraph();	
	virtual ~COsilloGraph();

	void Startup();
	void Stop();
	void SetMaxPos(int MaxPos){m_MaxPos = MaxPos;}
	void SetCurrentPos(int CurrentPos);
	void SetLineColor(COLORREF nColor){m_nColorLine = nColor;}
	void SetGridColor(COLORREF nColor){m_nColorGrid = nColor;}

	void DrawGrid();
	void DrawPos();

private:
	int		m_MaxPos;
	int		m_CurrentPos;
	int		m_ShowPos;
	int		m_PosUnit;
	int		m_GridUnit;
	int		m_MoveUnit;
	int		m_Height;
	int		m_Width;
	int		m_Unit;

	COLORREF m_nColorLine;
	COLORREF m_nColorGrid;
	HANDLE	 m_ThreadHandle;
	CDC		 m_MemDC;
	CBitmap  m_Bitmap;
	CRect	 m_Rect;

	CPen	 m_penLine;

protected:
    //{{AFX_MSG(COsilloGraph)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // COLORSTATIC_H