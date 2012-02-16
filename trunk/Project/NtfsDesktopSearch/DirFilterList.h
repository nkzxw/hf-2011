// DirFilterList.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once


typedef struct SRowInfo
{
    SRowInfo():_bCheck(TRUE),_dwBasicInfo(0)
    {
    }
    BOOL    _bCheck;
    DWORD   _dwBasicInfo;        //目录的BasicInfo
    BOOL    _bSubDir;
}SROWINFO,*PSROWINFO;

class CRowInfoVect
{
public:
    CRowInfoVect();
    virtual ~CRowInfoVect();
public:
    int size()const;
    BOOL insert(int iItem,PSROWINFO pRowInfo);
    void push_back(PSROWINFO pRowInfo);
    void erase(int iItem);
    PSROWINFO operator[](int iItem)const;
private:
    PSROWINFO *m_pp;
    int m_maxsize;
    int m_size;
};

class CDirFilterList
{
public:
    virtual ~CDirFilterList(void);
    CDirFilterList(void);
    CDirFilterList(HWND hList);
    CDirFilterList* FromHandle(HWND hList);    
    void CreateContext(HWND hBtnAdd,HWND hBtnDel,HWND hBtnDelAll,HWND hBtnCheckAllDri,HWND hParent);
public:
    void Show(int width,int height);//指定了显示范围的宽和高
    void Hide();
    operator HWND();
    DWORD GetStyle()const;
    void SetStyle(DWORD dwStyle);
    DWORD GetExtendedStyle()const;
    void SetExtendedStyle(DWORD dwExStyle);
    void SetImageList(HIMAGELIST hImageList,int iRootImage,int iDirImage);
    void InsertColumn(int nCol, const LVCOLUMN* pColumn);
    BOOL GetCheckState(int iItem);
    BOOL GetLastCheckState(int iItem);
    void SetCheckState(int iItem,BOOL bCheck);

    int GetItemCount()const;
    int GetRootItemCount()const;
    void SetUnCheck(int iItem);
    //插入到第nItem项中
    //每插入一项都建立两个组合框，并设置组合框为隐藏
    void PushBackItemText(LPWSTR lpwszItem,BOOL bRoot=FALSE);
    void PushBackItem(LPWSTR lpwszItem,BOOL bRoot=FALSE);
    BOOL InsertItem(int nItem,LPWSTR lpwszItem,BOOL bRoot=FALSE);
    
    BOOL DeleteItem(int nItem);
    BOOL SetItemText(int nItem, int nSubItem, PWCHAR pwszText);

    int GetItemText(int nItem, int nSubItem,OUT PWCHAR pwszText, int nLen) const;
    PSROWINFO GetItemInfo(int nItem)const;
    BOOL GetSubItemRect(int iItem, int iSubItem,int nArea,RECT& o_rect);

    ///////////组合框编辑
    BOOL ClickList(int iItem,int iSubItem);
    void HideCombbox();
    BOOL ShowMenu();

    BOOL AddFilterDirectory();
    BOOL DeleteItemsSelected();
    BOOL CheckAllDriverItems();
    BOOL DeleteAllDirItems();   
    BOOL OnItemChange(int iItem);

    int GetFileCheckState(DWORD *&pBasicInfo,BOOL *&pbSubDir);
private:
    CRowInfoVect m_vInfo;
    HWND m_hWnd;

    int  m_nItemCount;
    int  m_nRootItem;    //驱动盘符项数

    int  m_iRootImage;  //盘符的ICON
    int  m_iDirImage;   //目录的ICON

    HWND m_hParent;

    HWND m_hCheck;  
    HWND m_hBtnDelAll;
    HWND m_hBtnCheckAllDri;
    HWND m_hBtnAdd;
    HWND m_hBtnDel;
    HWND m_hComb;
    int  m_iLastItem;
};
