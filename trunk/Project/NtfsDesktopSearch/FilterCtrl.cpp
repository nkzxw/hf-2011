// FilterCtrl.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "FilterCtrl.h"

extern BOOL g_bFileSearch;
extern COutVector g_vDirOutPtr;
extern COutVector g_vFileOutPtr;
extern BOOL g_bCanSearch;
extern BOOL g_bJustFilter;//�ղŰ��˹��˵�
extern CLock g_Lock;//������



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
            MessageBoxA(m_hParentWnd,"��ǰ�����û���ļ�(���ļ���)��Ϊ0","��ʾ",MB_ICONINFORMATION);
            return FALSE;
        }
        BOOL bTran;
        m_llSize=GetDlgItemInt(m_hParentWnd,IDC_EDIT_SIZE,&bTran,FALSE);
        if(!bTran){
            MessageBoxA(m_hParentWnd,"ָ�����ļ���С����Ч","����",MB_ICONHAND);
            return FALSE;
        }
        m_typeCompare=::SendMessageW(m_hSizeLeftComb,CB_GETCURSEL,0,0);
        m_typeUnit=::SendMessageW(m_hSizeRightComb,CB_GETCURSEL,0,0);
        //���ݵ�λ����m_llSizeת��ΪB��λ
        m_llSize<<=(10*m_typeUnit);
        if(0==m_llSize)
        {
            if(2==m_typeCompare){//С��
                MessageBoxA(m_hParentWnd,"������С��0���ļ�","����",MB_ICONHAND);
                return FALSE;
            }
            if(4==m_typeCompare)//С�ڵ���0
                m_typeCompare=0;
        }

        bValid=TRUE;//����Ч
    }

    if(m_bUseDate)
    {       
        SYSTEMTIME sysTm,curSysTm;
        FILETIME fileTm;
        m_iDateType=::SendMessageW(m_hDateTypeComb,CB_GETCURSEL,0,0);
        if(IsDlgButtonChecked(m_hParentWnd,IDC_RADIO_POS)){//>=ĳ��
            m_typeDate=0;
            ::SendMessage(m_hPosTime,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            if(IsGreater(sysTm,curSysTm)){
                MessageBoxA(m_hParentWnd,"ָ����һ��δ��ʱ�䣡","����",MB_ICONHAND);
                return FALSE;
            }
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate);
            //ֻҪ�Ǵ��ڵ���m_llDate�Ķ��ǺϷ���
        }
        else if(IsDlgButtonChecked(m_hParentWnd,IDC_RADIO_PRE)){//<ĳ��
            m_typeDate=1;
            ::SendMessage(m_hPreTime,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate);
            //ֻҪ��<m_llDate�Ķ��ǺϷ���
        }        
        else {//ָ������֮��
            m_typeDate=2;
            ::SendMessage(m_hMidTime1,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate);
            if(IsGreater(sysTm,curSysTm)){
                MessageBoxA(m_hParentWnd,"ָ����һ��δ��ʱ�䣡","����",MB_ICONHAND);
                return FALSE;
            }

            ::SendMessage(m_hMidTime2,DTM_GETSYSTEMTIME,0,(LPARAM)(&sysTm));
            SystemTimeToFileTime(&sysTm,&fileTm);
            LocalFileTimeToFileTime(&fileTm,(PFILETIME)&m_llDate2);
            if(m_llDate>m_llDate2){
                MessageBoxA(m_hParentWnd,"ָ����ʱ�����䲻�Ϸ���","����",MB_ICONHAND);
                return FALSE;
            }
        }
        bValid=TRUE;//����Ч
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
            MessageBoxA(m_hParentWnd,"����ָ��һ���������ԣ�","����",MB_ICONHAND);
            return FALSE;
        }
        bValid=TRUE;
    }

    return bValid;
}

//��������������ô
const DWORD AttrFilter=FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN;
BOOL CFilterCtrl::IsSat(DWORDLONG *pOutTimeCreate
                                 ,DWORDLONG *pOutTimeAccess
                                 ,DWORDLONG *pOutTimeChange
                                 ,DWORDLONG *pOutSize
                                 ,DWORD *pOutAttr)
{
    if(pOutSize){//���ʱ���Ƿ�����
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
        DWORD srcDest=*pOutAttr&AttrFilter;//��ȡ������
        if(0==srcDest){//Ŀ������ͨ�ļ�
            if(m_bNormal) return TRUE;
            else return FALSE;
        }else{//Ŀ�겻����ͨ�ļ�
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
        g_Lock.Lock();//��ֹ����
        g_bCanSearch=FALSE; //�������˺� ������Ӷ��ı���˽�� �����û����Ѷ��ı���
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
        int size;//��ǰ����ô����
        int nSee;
        BOOL bContinue;
        PIndexElemType pBlock,pBlockEnd;
        PNORMALFILE_RECORD pFile;
        PDIRECTORY_RECORD pDir;
        if(m_bUseSize)
        {
            pOutSize=&llFileSize;          
            g_vDirOutPtr.clear_all();//ȥ��Ŀ¼
        }else
        {
            pHead=g_vDirOutPtr.GetHeadPtr();
            size=g_vDirOutPtr.size();//��ǰ����ô����
            nSee=0;
            g_vDirOutPtr.clear();//�����ǰ���
            bContinue=TRUE;
            for(;bContinue;++pHead)//�������еĿ��еĽ��
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
        size=g_vFileOutPtr.size();//��ǰ����ô����
        nSee=0;
        g_vFileOutPtr.clear();//�����ǰ���
        bContinue=TRUE;
        for(;bContinue;++pHead)//�������еĿ��еĽ��
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
    int w=rc.right-rc.left;//����������
    int left=((width-w)>>1);//��ť����left
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
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"����");
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"����");    
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"С��");
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"���ڵ���");
    ::SendMessageW(m_hSizeLeftComb,CB_ADDSTRING,0,(LPARAM)L"С�ڵ���");
    ::SendMessageW(m_hSizeLeftComb,CB_SETCURSEL,0,0);m_typeCompare=0;
    SendMessage(m_hSizeEdit,EM_LIMITTEXT,32,0);
    ::SetWindowTextW(m_hSizeEdit,L"0");m_llSize=0;
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"B");
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"KB");    
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"MB");
    ::SendMessageW(m_hSizeRightComb,CB_ADDSTRING,0,(LPARAM)L"GB");
    ::SendMessageW(m_hSizeRightComb,CB_SETCURSEL,0,1);m_typeUnit=1;

    ::SendMessageW(m_hDateTypeComb,CB_ADDSTRING,0,(LPARAM)L"������ʱ��");
    ::SendMessageW(m_hDateTypeComb,CB_ADDSTRING,0,(LPARAM)L"������޸�ʱ��");    
    ::SendMessageW(m_hDateTypeComb,CB_ADDSTRING,0,(LPARAM)L"���������ʱ��");
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