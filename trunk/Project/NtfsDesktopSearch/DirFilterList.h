// DirFilterList.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once


typedef struct SRowInfo
{
    SRowInfo():_bCheck(TRUE),_dwBasicInfo(0)
    {
    }
    BOOL    _bCheck;
    DWORD   _dwBasicInfo;        //Ŀ¼��BasicInfo
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
    void Show(int width,int height);//ָ������ʾ��Χ�Ŀ�͸�
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
    //���뵽��nItem����
    //ÿ����һ�����������Ͽ򣬲�������Ͽ�Ϊ����
    void PushBackItemText(LPWSTR lpwszItem,BOOL bRoot=FALSE);
    void PushBackItem(LPWSTR lpwszItem,BOOL bRoot=FALSE);
    BOOL InsertItem(int nItem,LPWSTR lpwszItem,BOOL bRoot=FALSE);
    
    BOOL DeleteItem(int nItem);
    BOOL SetItemText(int nItem, int nSubItem, PWCHAR pwszText);

    int GetItemText(int nItem, int nSubItem,OUT PWCHAR pwszText, int nLen) const;
    PSROWINFO GetItemInfo(int nItem)const;
    BOOL GetSubItemRect(int iItem, int iSubItem,int nArea,RECT& o_rect);

    ///////////��Ͽ�༭
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
    int  m_nRootItem;    //�����̷�����

    int  m_iRootImage;  //�̷���ICON
    int  m_iDirImage;   //Ŀ¼��ICON

    HWND m_hParent;

    HWND m_hCheck;  
    HWND m_hBtnDelAll;
    HWND m_hBtnCheckAllDri;
    HWND m_hBtnAdd;
    HWND m_hBtnDel;
    HWND m_hComb;
    int  m_iLastItem;
};
