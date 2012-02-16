// DirFilterList.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "DirFilterList.h"


CRowInfoVect::CRowInfoVect()
{
    m_maxsize=50;
    m_size=0;
    m_pp=new PSROWINFO[m_maxsize];
    assert(m_pp);
}
CRowInfoVect::~CRowInfoVect()
{
    delete []m_pp;
}

int CRowInfoVect::size()const{return m_size;}
BOOL CRowInfoVect::insert(int iItem,PSROWINFO pRowInfo)
{
    assert(iItem>=0 && iItem<=m_size);
    for(int i=0;i<m_size;++i)
    {
        if(m_pp[i]->_dwBasicInfo==pRowInfo->_dwBasicInfo) return FALSE;
    }
    ++m_size;
    if(m_size>m_maxsize)
    {
        PSROWINFO *pp=m_pp;
        m_pp=new PSROWINFO[m_maxsize+=10];
        for(int i=0;i<iItem;++i) m_pp[i]=pp[i];
        m_pp[iItem]=pRowInfo;
        for(int i=iItem+1;i<m_size;++i) m_pp[i]=pp[i-1];
        delete []pp;
    }else
    {
        for(int i=m_size-1;i>iItem;--i) m_pp[i]=m_pp[i-1];
        m_pp[iItem]=pRowInfo;
    }
    return TRUE;
}
void CRowInfoVect::push_back(PSROWINFO pRowInfo)
{
    if(m_size==m_maxsize)
    {
        PSROWINFO *pp=m_pp;
        m_pp=new PSROWINFO[m_maxsize+=10];
        for(int i=0;i<m_size;++i) m_pp[i]=pp[i];
        delete []pp;
    }
    m_pp[m_size++]=pRowInfo;
}
void CRowInfoVect::erase(int iItem)
{
    assert(iItem>=0 && iItem<m_size);
    --m_size;
    delete m_pp[iItem];
    for(int i=iItem;i<m_size;++i)
    {
        m_pp[i]=m_pp[i+1];
    }
}
PSROWINFO CRowInfoVect::operator[](int iItem)const
{
    assert(iItem>=0 && iItem<m_size);
    return m_pp[iItem];
}



/*virtual */CDirFilterList::~CDirFilterList(void)
{
    if(m_hComb && IsWindow(m_hComb)) CloseHandle(m_hComb);
}

CDirFilterList::CDirFilterList(void)
{
    m_hWnd=0;
    m_nRootItem=0;
    m_nItemCount=0;
}
CDirFilterList::CDirFilterList(HWND hList)
{
    assert(hList && IsWindow(hList));
    m_hWnd=hList;
}
CDirFilterList* CDirFilterList::FromHandle(HWND hList)
{
    assert(hList && IsWindow(hList));
    m_hWnd=hList;
    return this;
}
void CDirFilterList::CreateContext(HWND hBtnAdd,HWND hBtnDel,HWND hBtnDelAll,HWND hBtnCheckAllDri,HWND hParent)
{
    m_hBtnDelAll=hBtnDelAll;
    m_hBtnCheckAllDri=hBtnCheckAllDri;
    m_hBtnAdd=hBtnAdd;
    m_hBtnDel=hBtnDel;
    m_hParent=hParent;
    SetStyle(
        GetStyle()
        |WS_CLIPSIBLINGS
        |WS_CLIPCHILDREN
        |LVS_ICON
        |LVS_SHOWSELALWAYS
        |LVS_SHAREIMAGELISTS
        );
    SetExtendedStyle(
        GetExtendedStyle()|
        WS_EX_LEFT|
        WS_EX_LTRREADING|
        WS_EX_RIGHTSCROLLBAR|
        LVS_EX_FULLROWSELECT |
        LVS_EX_INFOTIP|
        LVS_EX_UNDERLINEHOT|
        LVS_EX_CHECKBOXES|
        LVS_EX_GRIDLINES
        );
    m_hComb=CreateWindowW(L"COMBOBOX",NULL,WS_CHILD |
        WS_VISIBLE |
        WS_TABSTOP |
        CBS_DROPDOWNLIST |
        WS_VSCROLL |
        WS_HSCROLL,0,0,1,1,m_hWnd,0,0,0);
    HFONT hFont=(HFONT)::SendMessage(m_hWnd,WM_GETFONT,0,0);
    assert(m_hComb && hFont);
    ::ShowWindow(m_hComb,SW_HIDE);       
    ::SendMessage(m_hComb,WM_SETFONT,(WPARAM)hFont,NULL);
    ::SendMessage(m_hComb,CB_ADDSTRING,0,(LPARAM)L"是");
    ::SendMessage(m_hComb,CB_ADDSTRING,0,(LPARAM)L"否");

    m_iLastItem=-1;
}

void CDirFilterList::Show(int width,int height)
{
    RECT rc;
    GetWindowRect(m_hWnd,&rc);
    ScreenToClient(m_hParent,&rc);
    int rc_right=width-25;
    ::SetWindowPos(m_hWnd, NULL,rc.left,3,
        rc_right-rc.left,rc.bottom-rc.top,SWP_NOZORDER | SWP_NOACTIVATE); 

    ShowWindow(m_hBtnDelAll,SW_SHOW);
    ShowWindow(m_hBtnCheckAllDri,SW_SHOW);
    ShowWindow(m_hCheck,SW_SHOW);
    ShowWindow(m_hBtnAdd,SW_SHOW);
    ShowWindow(m_hBtnDel,SW_SHOW);
    ShowWindow(m_hWnd,SW_SHOW);
}
void CDirFilterList::Hide()
{
    ShowWindow(m_hBtnDelAll,SW_HIDE);
    ShowWindow(m_hBtnCheckAllDri,SW_HIDE);
    ShowWindow(m_hBtnAdd,SW_HIDE);
    ShowWindow(m_hBtnDel,SW_HIDE);
    ShowWindow(m_hWnd,SW_HIDE); 
    HideCombbox();
}
CDirFilterList::operator HWND(){return m_hWnd;}
DWORD CDirFilterList::GetStyle()const
{
    return GetWindowLong(m_hWnd,GWL_STYLE);
}   
void CDirFilterList::SetStyle(DWORD dwStyle)
{
    SetWindowLong(m_hWnd,GWL_STYLE,dwStyle);
}
DWORD CDirFilterList::GetExtendedStyle()const
{
    return ListView_GetExtendedListViewStyle(m_hWnd);
}
void CDirFilterList::SetExtendedStyle(DWORD dwExStyle)
{
    ListView_SetExtendedListViewStyle(m_hWnd,dwExStyle);
}
void CDirFilterList::SetImageList(HIMAGELIST hImageList,int iRootImage,int iDirImage)
{
    ListView_SetImageList(m_hWnd,hImageList,LVSIL_SMALL);
    m_iRootImage=iRootImage;
    m_iDirImage=iDirImage;
}
void CDirFilterList::InsertColumn(int nCol, const LVCOLUMN* pColumn)
{
    ListView_InsertColumn(m_hWnd,nCol,pColumn);
}
BOOL CDirFilterList::GetCheckState(int iItem)
{
    return ListView_GetCheckState(m_hWnd,iItem);
}
BOOL CDirFilterList::GetLastCheckState(int iItem)
{
    return m_vInfo[iItem]->_bCheck;
}
void CDirFilterList::SetCheckState(int iItem,BOOL bCheck)
{
    m_vInfo[iItem]->_bCheck=bCheck;
}

int CDirFilterList::GetItemCount()const
{
    return m_nItemCount;
}
int CDirFilterList::GetRootItemCount()const
{
    return m_nRootItem;
}

//勾选了第iItem项 其父目录自动失去勾选
void CDirFilterList::SetUnCheck(int iItem)
{
    DWORD iDri=(m_vInfo[iItem]->_dwBasicInfo>>27);
    if(iItem<m_nRootItem){//将所有的子UnCheck       
        for(int i=m_nRootItem;i<m_nItemCount;++i)
        {
            if((m_vInfo[i]->_dwBasicInfo>>27)==iDri){
                m_vInfo[i]->_bCheck=FALSE;
                ListView_SetCheckState(m_hWnd,i,FALSE);
            }
        }
    }else{//将父UnCheck
        for(int i=0;i<m_nRootItem;++i)
        {
            if((m_vInfo[i]->_dwBasicInfo>>27)==iDri){
                m_vInfo[i]->_bCheck=FALSE;
                ListView_SetCheckState(m_hWnd,i,FALSE);
                break;
            }
        }
    }
}

//插入到第nItem项中
//每插入一项都建立两个组合框，并设置组合框为隐藏
void CDirFilterList::PushBackItemText(LPWSTR lpwszItem,BOOL bRoot/*=FALSE*/)
{
    int i=GetItemCount();
    if(InsertItem(i,lpwszItem,bRoot))
        SetItemText(i,1,L"是");    
}
void CDirFilterList::PushBackItem(LPWSTR lpwszItem,BOOL bRoot/*=FALSE*/)
{
    InsertItem(GetItemCount(),lpwszItem,bRoot);
}
BOOL CDirFilterList::InsertItem(int nItem,LPWSTR lpwszItem,BOOL bRoot/*=FALSE*/)
{ 
    PSROWINFO pRow=new SROWINFO;
    if(bRoot) {
        ++m_nRootItem;
        PWCHAR pWch=lpwszItem;
        while(*pWch++);
        pWch-=4;
        pRow->_dwBasicInfo=(DWORD(*pWch-L'A')<<27|5);
    }
    else{
        assert(nItem>=m_nRootItem && "保持目录项在ROOT项之后");
        HANDLE hFile=CreateFileW(lpwszItem,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
        assert(hFile!=INVALID_HANDLE_VALUE);
        BY_HANDLE_FILE_INFORMATION fileInfo;
        GetFileInformationByHandle(hFile,&fileInfo);
        CloseHandle(hFile);
        if(*lpwszItem>=L'a')
            pRow->_dwBasicInfo=(DWORD(*lpwszItem-L'a')<<27|fileInfo.nFileIndexLow);
        else
            pRow->_dwBasicInfo=(DWORD(*lpwszItem-L'A')<<27|fileInfo.nFileIndexLow);
    }
    pRow->_bCheck=TRUE;
    pRow->_bSubDir=TRUE;
    if(!m_vInfo.insert(nItem,pRow)) {
        delete pRow;
        return FALSE;
    }

    LV_ITEM lvItem;
    lvItem.mask=LVIF_TEXT|LVIF_IMAGE;
    lvItem.iItem=nItem;
    lvItem.iSubItem=0;
    lvItem.pszText=lpwszItem;
    lvItem.cchTextMax=wcslen(lpwszItem)+1;
    if(bRoot) lvItem.iImage=m_iRootImage;
    else lvItem.iImage=m_iDirImage;          
    ListView_InsertItem(m_hWnd,&lvItem);
    ++m_nItemCount;
    ListView_SetCheckState(m_hWnd,nItem,TRUE);
    SetUnCheck(nItem);
    return TRUE;
}

BOOL CDirFilterList::DeleteItemsSelected()
{
    int iItem;
    int iStart=-1;
    BOOL bChange=FALSE;
    for(;(iItem=ListView_GetNextItem(m_hWnd,iStart, LVNI_ALL|LVNI_SELECTED))>=0;)
    {
        if(iItem<m_nRootItem){
            ++iStart;//不允许删除根目录
        }
        else {
            if(DeleteItem(iItem)) bChange=TRUE;
        }
    }
    return bChange;
}


BOOL CDirFilterList::DeleteItem(int nItem)
{
    BOOL bChange=FALSE;
    if(m_vInfo[nItem]->_bCheck) bChange=TRUE;
    m_vInfo.erase(nItem);
    assert(::IsWindow(m_hWnd)); 
    --m_nItemCount;
    ::SendMessage(m_hWnd, LVM_DELETEITEM, nItem, 0L);
    return bChange;
}

BOOL CDirFilterList::SetItemText(int nItem, int nSubItem, PWCHAR pwszText)
{
    assert(::IsWindow(m_hWnd));
    assert((GetStyle() & LVS_OWNERDATA)==0);
    if(*pwszText==L'是'){
        m_vInfo[nItem]->_bSubDir=TRUE;
    }else{
        m_vInfo[nItem]->_bSubDir=FALSE;
    }
    LVITEM lvi;
    lvi.iSubItem = nSubItem;
    lvi.pszText = pwszText;
    return (BOOL) ::SendMessage(m_hWnd, LVM_SETITEMTEXT, nItem, (LPARAM)&lvi);
}

int CDirFilterList::GetItemText(int nItem, int nSubItem,OUT PWCHAR pwszText, int nLen) const
{
    assert(::IsWindow(m_hWnd));
    LVITEM lvi;
    memset(&lvi, 0, sizeof(LVITEM));
    lvi.iSubItem = nSubItem;
    lvi.cchTextMax = nLen;
    lvi.pszText = pwszText;
    return (int)::SendMessage(m_hWnd, LVM_GETITEMTEXT, (WPARAM)nItem,(LPARAM)&lvi);
}

PSROWINFO CDirFilterList::GetItemInfo(int nItem)const
{
    assert(nItem<m_vInfo.size());
    return m_vInfo[nItem];
}

BOOL CDirFilterList::GetSubItemRect(int iItem, int iSubItem,int nArea,RECT& o_rect)
{
    assert(nArea == LVIR_BOUNDS || nArea == LVIR_ICON || nArea == LVIR_LABEL);
    RECT rect;
    rect.top = iSubItem;
    rect.left = nArea;
    BOOL bRet = (BOOL) ::SendMessage(m_hWnd, LVM_GETSUBITEMRECT,
        iItem, (LPARAM) &rect);
    if (bRet) o_rect = rect;
    return bRet;
}


///////////组合框编辑
BOOL CDirFilterList::ClickList(int iItem,int iSubItem)
{
    BOOL bChange=FALSE;
    if(m_iLastItem!=-1)//当前有组框显示
    {
        int curcel=::SendMessage(m_hComb,CB_GETCURSEL,0,0);
        ::ShowWindow(m_hComb,SW_HIDE);
        if(0==curcel) {
            if(!m_vInfo[m_iLastItem]->_bSubDir && m_vInfo[m_iLastItem]->_bCheck) bChange=TRUE;
            SetItemText(m_iLastItem,1,L"是");
        }else if(1==curcel)
        {
            if(m_vInfo[m_iLastItem]->_bSubDir && m_vInfo[m_iLastItem]->_bCheck) bChange=TRUE;
            SetItemText(m_iLastItem,1,L"否");
        }  
        m_iLastItem=-1;
        return bChange;
    }

    if(iItem>=0 && 1==iSubItem)
    {    
        RECT rect;
        GetSubItemRect(iItem,iSubItem,LVIR_LABEL,rect);
        PSROWINFO pRow=GetItemInfo(iItem);
        WCHAR wszText[5]={0};
        GetItemText(iItem,iSubItem,wszText,5);
        if(1==iSubItem){
            int sel=::SendMessage(m_hComb,CB_SELECTSTRING,(WPARAM)-1,(LPARAM)wszText);
            if( CB_ERR == sel) ::SendMessage(m_hComb,CB_SETCURSEL,0,0);
            else ::SendMessage(m_hComb,CB_SETCURSEL,sel,0);
        }
        else assert(0);
        m_iLastItem=iItem;

        RECT rcList;
        GetWindowRect(m_hWnd,&rcList);
        int width=rcList.right-rcList.left-rect.left-5;
        if(rect.right-rect.left<width) width=rect.right-rect.left;
        //显示组合框
        ::SetWindowPos(m_hComb,NULL,rect.left,rect.top,width,rect.bottom-rect.top,SWP_NOZORDER);
        ::ShowWindow(m_hComb,SW_SHOW);
    }
    return FALSE;
}

void CDirFilterList::HideCombbox()
{
    ::ShowWindow(m_hComb,SW_HIDE);
    m_iLastItem=-1;
}

BOOL CDirFilterList::ShowMenu()
{
    HMENU hMenu=LoadMenu(GetModuleHandle(0),MAKEINTRESOURCE(IDR_MENU1));
    HMENU hPopupMenu=GetSubMenu(hMenu,0);
    POINT pt;
    GetCursorPos(&pt);
    int cmd=TrackPopupMenu(hPopupMenu,TPM_RETURNCMD|TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y,
        0,m_hWnd,0);
    
    if(IDR_ADD_DIR==cmd) return AddFilterDirectory();
    else if(IDR_DEL_DIR==cmd) return DeleteItemsSelected();
    else if(IDR_CHECKALLDRIVER==cmd) return CheckAllDriverItems();
    else if(IDR_DELALLDIR==cmd) return DeleteAllDirItems();
    else return FALSE;
}

//如果成功增加 返回TRUE
//否则FALSE
BOOL CDirFilterList::AddFilterDirectory()
{
    BROWSEINFOW  bi=
    {
        m_hParent,
        NULL,NULL,
        L"请选择目录: ",
        BIF_RETURNONLYFSDIRS|BIF_DONTGOBELOWDOMAIN,
        NULL,NULL,0
    };
    ITEMIDLIST *pidl=SHBrowseForFolderW(&bi); 
    if(NULL==pidl) return FALSE; //未选文件夹，退出!
    WCHAR  path[MAX_PATH]={0};
    if(!SHGetPathFromIDListW(pidl,path))
    {
        ::MessageBox(0,L"BROWSEINFO参数设置错误!",0,0); return FALSE;
    } 
    else{
        //检查是否NTFS
        BOOL bNtfs=FALSE;
        WCHAR cDri=*path;
        if(cDri>=L'a') cDri-=32;
        for(int i=0;i<m_nRootItem;++i)
        {
            if((m_vInfo[i]->_dwBasicInfo>>27)+L'A'==cDri)
            {
                bNtfs=TRUE;
                break;
            }
        }
        if(!bNtfs) ::MessageBoxA(m_hWnd,"对不住，所选的不是NTFS卷，无法指定！",0,MB_ICONHAND);
        else{
            if(path[3]){
                PushBackItemText(path);
                return TRUE;
            }
        }
    }  
    return FALSE;
}

//勾选所有的驱动盘
BOOL CDirFilterList::CheckAllDriverItems()
{
    BOOL bChange=FALSE;
    for(int i=0;i<m_nRootItem;++i)
    {
        if(!m_vInfo[i]->_bCheck) bChange=TRUE;
        ListView_SetCheckState(m_hWnd,i,TRUE);
        SetCheckState(i,TRUE);
    }
    return bChange;
}

//删除所有的目录项
BOOL CDirFilterList::DeleteAllDirItems()
{
    BOOL bChange=FALSE;
    while(m_nItemCount>m_nRootItem)
    {
        --m_nItemCount;
        if(m_vInfo[m_nItemCount]->_bCheck) bChange=TRUE;
        m_vInfo.erase(m_nItemCount);
        ListView_DeleteItem(m_hWnd,m_nItemCount);
    }
    return bChange;
}

BOOL CDirFilterList::OnItemChange(int iItem)
{
    if(iItem>=GetItemCount()) return FALSE;
    BOOL bCheck=GetCheckState(iItem);
    BOOL bLastCheck=GetLastCheckState(iItem);
    if(bCheck!=bLastCheck)
    {
        if(bCheck)//勾选第pNMLV->iItem项
        {
            SetUnCheck(iItem);
        }else//去勾第pNMLV->iItem项
        {
        }
        SetCheckState(iItem,bCheck);
        return TRUE;
    }  
    return FALSE;
}

//返回-1表示默认的勾选方式：全卷勾选
//返回0表示一个都没勾选
int CDirFilterList::GetFileCheckState(
                DWORD *&pBasicInfo     //返回BasicInfo数组
                ,BOOL *&pbSubDir        //返回与上面对应的是否包含子目录信息
                )
{
     int nCheck=0;
     int i;
     BOOL bSub=TRUE;
     for(i=0;i<m_nRootItem;++i)
     {
         if(m_vInfo[i]->_bCheck) nCheck++;
         if(!m_vInfo[i]->_bSubDir) bSub=FALSE;   
     }
     if(nCheck==m_nRootItem && bSub) return -1;//全卷勾选
     
     for(;i<m_nItemCount;++i)
     {
        if(m_vInfo[i]->_bCheck) nCheck++;
     }
     if(0==nCheck) return 0;

     pBasicInfo=new DWORD[nCheck];
     pbSubDir=new BOOL[nCheck];
     PSROWINFO pRow;
     int j=0;
     for(i=0;i<m_nItemCount;++i)
     {
         pRow=m_vInfo[i];
         if(pRow->_bCheck)
         {
            pBasicInfo[j]=pRow->_dwBasicInfo;
            pbSubDir[j]=pRow->_bSubDir;
            ++j;
         }
     }
     assert(j==nCheck);
     return nCheck;
}
