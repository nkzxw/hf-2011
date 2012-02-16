// FilterCtrl.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "FilterCtrl.h"

extern BOOL g_bFileSearch;
extern COutVector g_vDirOutPtr;
extern COutVector g_vFileOutPtr;
extern BOOL g_bCanSearch;
extern BOOL g_bJustFilter;//刚才按了过滤的
extern CLock g_Lock;//结束锁



CFilterCtrl::CFilterCtrl()
{
    m_bUseSize=FALSE;
    m_bUseDate=FALSE;
    m_bUseAttr=FALSE;
}

/*virtual */CFilterCtrl::~CFilterCtrl()
{

}

void CFilterCtrl::SetSizeUse(BOOL bUse)
{
    m_bUseSize=bUse;
}
void CFilterCtrl::SetDateUse(BOOL bUse)
{
    m_bUseDate=bUse;
}
void CFilterCtrl::SetAttrUse(BOOL bUse)
{
    m_bUseAttr=bUse;
}

//Is a>b ?
__forceinline BOOL IsGreater(const SYSTEMTIME& a,const SYSTEMTIME& b)
{
    if(a.wYear>b.wYear) return TRUE;
    else if(a.wYear==b.wYear){
        if(a.wMonth>b.wMonth) return TRUE;
        else if(a.wMonth==b.wMonth){
            if(a.wDay>b.wDay) return TRUE;
        }
    }
    return FALSE;
}



BOOL CFilterCtrl::RetrieveFilter()
{
    BOOL bValid=FALSE;
    if(m_bUseSize)
    {
        if(0==g_vFileOutPtr.size()){
            MessageBoxA(m_hParentWnd,"当前结果中没有文件(非文件夹)数为0","提示",MB_ICONINFORMATION);
            return FALSE;
        }
        BOOL bTran;
        m_llSize=GetDlgItemInt(m_hParentWnd,IDC_EDIT_SIZE,&bTran,FALSE);
        if(!bTran){
            MessageBoxA(m_hParentWnd,"指定的文件大小数无效","错误",MB_ICONHAND);
            return FALSE;
        }
        m_typeCompare=::SendMessageW(m_hSizeLeftComb,CB_GETCURSEL,0,0);
        m_typeUnit=::SendMessageW(m_hSizeRightComb,CB_GETCURSEL,0,0);
        //根据单位，将m_llSize转换为B单位
        m_llSize<<=(10*m_typeUnit);
        if(0==m_llSize)
        {
            if(2==m_typeCompare){//小于
                MessageBoxA(m_hParentWnd,"不存在小于0的文件","错误",MB_ICONHAND);
                return FALSE;
            }
            if(4==m_typeCompare)//小于等于0
                m_typeCompare=0;
        }

        bValid=TRUE;//已有效
    }

    if(m_bUseDate)
    {       
        SYSTEMTIME sysTm,curSysTm;
        FILETIME fileTm;
        m_iDateType=::SendMessageW(m_hDateTypeComb,CB_GETCURSEL,0,0);
        if(IsDlgButtonChecked(m_hParentWnd,IDC_RADIO_POS)){//>=某天
            m_typeDate=0;
            ::SendMessage(m_hPosTime,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            if(IsGreater(sysTm,curSysTm)){
                MessageBoxA(m_hParentWnd,"指定了一个未来时间！","错误",MB_ICONHAND);
                return FALSE;
            }
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate);
            //只要是大于等于m_llDate的都是合法的
        }
        else if(IsDlgButtonChecked(m_hParentWnd,IDC_RADIO_PRE)){//<某天
            m_typeDate=1;
            ::SendMessage(m_hPreTime,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate);
            //只要是<m_llDate的都是合法的
        }        
        else {//指定的天之间
            m_typeDate=2;
            ::SendMessage(m_hMidTime1,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate);
            if(IsGreater(sysTm,curSysTm)){
                MessageBoxA(m_hParentWnd,"指定了一个未来时间！","错误",MB_ICONHAND);
                return FALSE;
            }

            ::SendMessage(m_hMidTime2,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate2);
            if(m_llDate>m_llDate2){
                MessageBoxA(m_hParentWnd,"指定的时间区间不合法！","错误",MB_ICONHAND);
                return FALSE;
            }
        }
        bValid=TRUE;//已有效
    }

    if(m_bUseAttr)
    {
        m_dwAttr=0;
        if(SendMessage(m_hAttrHiddenCheck,BM_GETCHECK,0,0)==BST_CHECKED){
            m_dwAttr|=FILE_ATTRIBUTE_HIDDEN;
        }
        if(SendMessage(m_hAttrSysCheck,BM_GETCHECK,0,0)==BST_CHECKED){
            m_dwAttr|=FILE_ATTRIBUTE_SYSTEM;
        }
        m_bNormal=(SendMessage(m_hAttrNormalCheck,BM_GETCHECK,0,0)==BST_CHECKED);
        if(0==m_dwAttr && !m_bNormal){
            MessageBoxA(m_hParentWnd,"必须指定一个过滤属性！","错误",MB_ICONHAND);
            return FALSE;
        }
        bValid=TRUE;
    }

    return bValid;
}

//结果满足过滤条件么
const DWORD AttrFilter=FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN;
BOOL CFilterCtrl::IsSat(DWORDLONG *pOutTimeCreate
                                 ,DWORDLONG *pOutTimeAccess
                                 ,DWORDLONG *pOutTimeChange
                                 ,DWORDLONG *pOutSize
                                 ,DWORD *pOutAttr)
{
    if(pOutSize){//检查时间是否满足
        if(0==m_typeCompare){//==
            if(!(*pOutSize==m_llSize)) return FALSE;
        }else if(1==m_typeCompare){//>
            if(!(*pOutSize>m_llSize)) return FALSE;  
        }else if(2==m_typeCompare){//<
            if(!(*pOutSize<m_llSize)) return FALSE;  
        }else if(3==m_typeCompare){//>=
            if(!(*pOutSize>=m_llSize)) return FALSE;  
        }else {//4==m_typeCompare//<=
            if(!(*pOutSize<=m_llSize)) return FALSE; 
        }
    }
    if(m_bUseDate){
        DWORDLONG _tm;
        if(pOutTimeAccess) _tm=*pOutTimeAccess;
        else if(pOutTimeChange) _tm=*pOutTimeChange;
        else _tm=*pOutTimeCreate;

        if(0==m_typeDate){
            if(!(_tm>=m_llDate)) return FALSE;
        }else if(1==m_typeDate){
            if(!(_tm<m_llDate)) return FALSE;
        }else{
            if(_tm<m_llDate || _tm>=m_llDate2) return FALSE;
        }
    }

    if(pOutAttr){
        DWORD srcDest=*pOutAttr&AttrFilter;//获取的属性
        if(0==srcDest){//目标是普通文件
            if(m_bNormal) return TRUE;
            else return FALSE;
        }else{//目标不是普通文件
            if(m_dwAttr&srcDest) return TRUE;
            else return FALSE;
        }
    }

    return TRUE;
}

void CFilterCtrl::Go()
{ 
    if(g_bCanSearch||g_bJustFilter)
    {
        if(!RetrieveFilter()) return;
        g_Lock.Lock();//禁止监视
        g_bCanSearch=FALSE; //当按过滤后 不因监视而改变过滤结果 仅因用户重搜而改变结果
        g_bJustFilter=TRUE;

        DWORDLONG llFileSize;
        DWORDLONG llTimeCreate;
        DWORDLONG llTimeAccess;
        DWORDLONG llTimeChange;
        DWORD     dwAttr;

        DWORDLONG *pOutTimeCreate=NULL;
        DWORDLONG *pOutTimeAccess=NULL;
        DWORDLONG *pOutTimeChange=NULL;
        DWORDLONG *pOutSize=NULL;
        DWORD *pOutAttr=NULL;

        DWORD dwDri;
        DWORD dwFrn;

        if(m_bUseDate){
            if(0==m_iDateType) pOutTimeCreate=&llTimeCreate;
            else if(1==m_iDateType) pOutTimeChange=&llTimeChange;
            else pOutTimeAccess=&llTimeAccess;
        }
        if(m_bUseAttr) pOutAttr=&dwAttr;

        PIndexElemType* pHead;
        const int nSizeOfBlock=COutVector::MaxNumOfElemPerBlock;
        int size;//当前有这么多结果
        int nSee;
        BOOL bContinue;
        PIndexElemType pBlock,pBlockEnd;
        PNORMALFILE_RECORD pFile;
        PDIRECTORY_RECORD pDir;
        if(m_bUseSize)
        {
            pOutSize=&llFileSize;          
            g_vDirOutPtr.clear_all();//去掉目录
        }else
        {
            pHead=g_vDirOutPtr.GetHeadPtr();
            size=g_vDirOutPtr.size();//当前有这么多结果
            nSee=0;
            g_vDirOutPtr.clear();//清除当前结果
            bContinue=TRUE;
            for(;bContinue;++pHead)//考察所有的块中的结果
            {
                pBlock=*pHead;
                pBlockEnd=pBlock+nSizeOfBlock;
                for(;pBlock<pBlockEnd;++pBlock)
                {
                    if(nSee==size) {
                        bContinue=FALSE;
                        break;
                    }
                    pDir=(PDIRECTORY_RECORD)*pBlock; 
                    Helper_GetBasicInformation(pDir->BasicInfo>>27,pDir->BasicInfo&0x7ffffff
                        ,pOutTimeCreate,pOutTimeAccess,pOutTimeChange
                        ,pOutSize
                        ,pOutAttr
                        );
                    if(IsSat(pOutTimeCreate,pOutTimeAccess,pOutTimeChange
                        ,pOutSize
                        ,pOutAttr)
                        ){
                            g_vDirOutPtr.push_back(*pBlock);
                    }                    
                    ++nSee;
                }
            }
        }
        pHead=g_vFileOutPtr.GetHeadPtr();
        size=g_vFileOutPtr.size();//当前有这么多结果
        nSee=0;
        g_vFileOutPtr.clear();//清除当前结果
        bContinue=TRUE;
        for(;bContinue;++pHead)//考察所有的块中的结果
        {
            pBlock=*pHead;
            pBlockEnd=pBlock+nSizeOfBlock;
            for(;pBlock<pBlockEnd;++pBlock)
            {
                if(nSee==size) {
                    bContinue=FALSE;
                    break;
                }
                pFile=(PNORMALFILE_RECORD)*pBlock; 
                dwDri=pFile->ParentPtr->BasicInfo>>27;
                dwFrn=pFile->BasicInfo&0x7ffffff;
                Helper_GetBasicInformation(dwDri,dwFrn
                    ,pOutTimeCreate,pOutTimeAccess,pOutTimeChange
                    ,pOutSize
                    ,pOutAttr
                    );
                if(IsSat(pOutTimeCreate,pOutTimeAccess,pOutTimeChange
                    ,pOutSize
                    ,pOutAttr)
                    ){
                        g_vFileOutPtr.push_back(*pBlock);
                }                    
                ++nSee;
            }
        }

        Helper_SetCurrentState(g_vFileOutPtr.size()+g_vDirOutPtr.size(),NULL);
        ListView_SetItemCount(g_hListCtrl,g_vFileOutPtr.size()+g_vDirOutPtr.size());
        g_Lock.UnLock();
    }
}


void CFilterCtrl::Show(int width,int hight)
{
    RECT rc;
    GetWindowRect(m_hBtnFilter,&rc);
    ScreenToClient(m_hParentWnd,&rc);
    int w=rc.right-rc.left;//所画区域宽度
    int left=((width-w)>>1);//按钮的新left
    int left_delt=left-rc.left;
    for(int i=0;i<FILTER_HWND_COUNT-1;++i)
    {
        GetWindowRect(m_vhWnd[i],&rc);
        ScreenToClient(m_hParentWnd,&rc);
        ::SetWindowPos(m_vhWnd[i], NULL,rc.left+left_delt,rc.top,
            rc.right-rc.left,rc.bottom-rc.top,SWP_NOZORDER | SWP_NOACTIVATE); 
        ShowWindow(m_vhWnd[i],SW_SHOW);
    }  
}

void CFilterCtrl::Hide()
{
    for(int i=0;i<FILTER_HWND_COUNT-1;++i)
    {
        ShowWindow(m_vhWnd[i],SW_HIDE);
    }
}

void CFilterCtrl::Initialize()
{
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"等于");
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"大于");    
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"小于");
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"大于等于");
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"小于等于");
    ::SendMessageW(m_hSizeLeftComb,CB_SETCURSEL,0,0);m_typeCompare=0;
    SendMessage(m_hSizeEdit,EM_LIMITTEXT,32,0);
    ::SetWindowTextW(m_hSizeEdit,L"0");m_llSize=0;
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"B");
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"KB");    
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"MB");
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"GB");
    ::SendMessageW(m_hSizeRightComb,CB_SETCURSEL,0,1);m_typeUnit=1;

    ::SendMessageW(m_hDateTypeComb,CB_ADDSTRING,0,(LPARAM)L"按创建时间");
    ::SendMessageW(m_hDateTypeComb,CB_ADDSTRING,0,(LPARAM)L"按最近修改时间");    
    ::SendMessageW(m_hDateTypeComb,CB_ADDSTRING,0,(LPARAM)L"按最近访问时间");
    ::SendMessageW(m_hDateTypeComb,CB_SETCURSEL,0,0);m_iDateType=0;

    ::SendMessageW(m_hPosRadio,BM_SETCHECK,BST_CHECKED,0);m_typeDate=0;

}


#define SET_VALUE(val,i) {m_##val=val;m_vhWnd[i++]=val;}
void CFilterCtrl::CreateContext(HWND hSizeCheck,HWND hSizeLeftComb,HWND hSizeEdit,HWND hSizeRightComb
                                ,HWND hDateCheck,HWND hDateTypeComb,HWND hPreRadio,HWND hPreTime,HWND hPosRadio,HWND hPosTime,HWND hMidRadio,HWND hMidTime1,HWND hMidTime2
                                ,HWND hAttrCheck,HWND hAttrNormalCheck,HWND hAttrHiddenCheck,HWND hAttrSysCheck
                                ,HWND hBtnFilter,HWND hParentWnd)
{
    int o_i=0;
    SET_VALUE(hSizeCheck,o_i);
    SET_VALUE(hSizeLeftComb,o_i);
    SET_VALUE(hSizeEdit,o_i);
    SET_VALUE(hSizeRightComb,o_i);

    SET_VALUE(hDateCheck,o_i);
    SET_VALUE(hDateTypeComb,o_i);
    SET_VALUE(hPreRadio,o_i);
    SET_VALUE(hPreTime,o_i);
    SET_VALUE(hPosRadio,o_i);
    SET_VALUE(hPosTime,o_i);
    SET_VALUE(hMidRadio,o_i);
    SET_VALUE(hMidTime1,o_i);
    SET_VALUE(hMidTime2,o_i);


    SET_VALUE(hAttrCheck,o_i);
    SET_VALUE(hAttrNormalCheck,o_i);
    SET_VALUE(hAttrHiddenCheck,o_i);
    SET_VALUE(hAttrSysCheck,o_i);

    SET_VALUE(hBtnFilter,o_i);
    SET_VALUE(hParentWnd,o_i);
    assert(FILTER_HWND_COUNT==o_i);

    Initialize();
}