// DesktopSearch.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include <map>
using namespace std;

//#define TEST
#ifdef TEST
#define TEST_ROOT_DIR   L"D:\\"
#endif

BOOL g_bJournalIdFailure=FALSE;
CDebugTrace g_dbgTrace;
CMemoryMgr g_MemoryMgr; 

COutVector g_vDirOutPtr//Ŀ¼�Ľ������
        ,g_vFileOutPtr;//�ļ��Ľ���϶�

CLock g_Lock;//������

USN         g_curFirstUSN[26]={0};
USN         g_curNextUSN[26]={0};
DWORDLONG   g_curJournalID[26]={0};
HANDLE      g_hVols[26]={0};//����A~Z�Ķ�Ӧ�ľ��� -'A'��ȡ
HANDLE      g_hThread[26]={0};//ÿ����һ�������߳�
DWORD       g_BytesPerCluster[26];//ÿ���ֽ���
DWORD       g_FileRecSize[26];//MFT�ļ���¼��С
PBYTE       g_pOutBuffer[26]={0};//ÿ���̵��ļ����¼��ַ ��ʼʱ���� ����ʱ��


SearchStrOpt g_StrOptCase,g_LastStrOptCase,g_StrOptNoCase,g_LastStrOptNoCase;


BOOL g_bMonitorChange=FALSE;
HANDLE g_hEvent=NULL;
HWND g_hMainWnd=NULL;
HWND g_hStateWnd=NULL;
HWND g_hListCtrl=NULL;
HWND g_hEdit=NULL;
HWND g_hExtEdit=NULL;
BOOL g_bExtChange=FALSE;

BOOL g_bCanSearch=FALSE;
BOOL g_bJustFilter=FALSE;//�ղŰ��˹��˵�

int g_iRootIcon;
int g_iDirIcon;

//����ѡ��
BOOL g_bOptChanged=FALSE;//����ѡ��ı�
BOOL g_bCase=FALSE;
BOOL g_bDirSearch=TRUE;
BOOL g_bFileSearch=TRUE;
BOOL g_bDirSetExpand=FALSE; //Ŀ¼����չ��
CDirFilterList g_listDirFilter;
BOOL g_bDirSetting=FALSE;//������Ϣ����ʱ�ı�ΪTRUE ����KernelSearch


//����ѡ��
CFilterCtrl g_Filter;
BOOL g_bFilterExpand=FALSE; //Ŀ¼����չ��
BOOL g_bSizeFilter=FALSE;
BOOL g_bDateFilter=FALSE;
BOOL g_bAttrFilter=FALSE;


IContextMenu2* g_pIContext2;
IContextMenu3* g_pIContext3;
WNDPROC oldWndProc;



void UpdateLayout(HWND hMainWnd)
{
    RECT rectWnd;
    GetClientRect(hMainWnd,&rectWnd);
    HWND hStaticText=GetDlgItem(hMainWnd,IDC_STATIC_EXT);
    HWND hStaticText2=GetDlgItem(hMainWnd,IDC_STATIC_EXT2);
    RECT rcText;
    GetWindowRect(hStaticText,&rcText);
    int static_widht=rcText.right-rcText.left;//�ı���
    int editWidth=rectWnd.right-rectWnd.left-2*static_widht;
    int fnameEditWidth=editWidth*0.7;//�ļ����༭�򳤶�   

    int editTop;//��ֵ��Ҫ���ݹ���ѡ�� ��̬����
    if(g_bDirSetExpand){
        g_Filter.Hide();
        editTop=170;
        g_listDirFilter.Show(rectWnd.right-rectWnd.left,editTop);//��ʾ����Ŀ¼����       
    }else{
        g_listDirFilter.Hide();
        if(g_bFilterExpand){//��ʾ������չ
            editTop=145;
            g_Filter.Show(rectWnd.right-rectWnd.left,editTop);    
        }
        else {
            g_Filter.Hide();
            editTop=2;
        }
    }

    const int editHeight=20;
    int listTop=editTop+editHeight+2;

    int left;
    int width;
    if(g_hEdit != NULL && ((DWORD)::GetWindowLong(g_hEdit,GWL_STYLE) & WS_VISIBLE))
    {
        //�ļ���:
        left=3;
        width=static_widht;
        ::SetWindowPos(hStaticText2,NULL,left,
            editTop+3,width,editHeight-4,SWP_NOZORDER | SWP_NOACTIVATE); 

        //�ļ����༭��
        left+=width;
        width=fnameEditWidth;
        ::SetWindowPos(g_hEdit, NULL,left,editTop,
            width, editHeight,SWP_NOZORDER | SWP_NOACTIVATE);  

        //��չ����
        left+=width;
        width=static_widht;
        ::SetWindowPos(hStaticText,NULL,left,
            editTop+3,width,editHeight-4,SWP_NOZORDER | SWP_NOACTIVATE);  

        //��չ���༭��
        left+=width;
        width=rectWnd.right-left;
        ::SetWindowPos(g_hExtEdit, NULL,left,editTop,
            width,editHeight,SWP_NOZORDER | SWP_NOACTIVATE); 

        //����״̬��
        ::SendMessage(g_hStateWnd, WM_SIZE, 0, 0);
        RECT rectSB;
        ::GetWindowRect(g_hStateWnd, &rectSB);
        ::ScreenToClient(hMainWnd,&rectSB);

        RECT rectList;
        ::GetWindowRect(g_hListCtrl,&rectList);
        ScreenToClient(hMainWnd,&rectList);

        ::SetWindowPos(g_hListCtrl, HWND_TOP,rectList.left,listTop,
            rectWnd.right-rectWnd.left,rectSB.top-listTop,
            SWP_NOZORDER |SWP_NOACTIVATE);

        InvalidateRect(hMainWnd,&rectWnd,TRUE);
    }   
}



#define OUT_PUT_MONITOR

PDIRECTORY_RECORD OldParentPtr,NewParentPtr;//
void RenameNewDirectory(DWORD dwDri,DWORDLONG frn,DWORDLONG parent_frn,PWCHAR pFileName,int nameLen)
{
#ifdef OUT_PUT_MONITOR
    char buf[MAX_PATH]={0};
    WideCharToMultiByte(CP_ACP,0,pFileName,nameLen,buf,MAX_PATH,0,0);
    DebugTrace("IN RenameNewDirectory:%s\n",buf);
#endif

    BYTE szFileNameCode[768];//�����Ҫ255*3���ֽ�<768

    //��Ŀ¼���ӵ�Ŀ¼����  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,nameLen);
    g_vDirIndex.Lock();/****************����**********************/
    NewParentPtr=(PDIRECTORY_RECORD)g_MemDir.Alloc(GetDirectoryRecordLength(codeNameLen));

    DebugTrace("in map-insert\n");
    g_DirMap.insert(GetBasicInfo(dwDri,frn),(IndexElemType)NewParentPtr);//���뱾Ŀ¼ ��Ŀ¼FRN��
    DebugTrace("out map-insert\n");

    DebugTrace("in map-find\n");
    PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(GetBasicInfo(dwDri,parent_frn)); ++ppDir;
    DebugTrace("out map-find\n");

    NewParentPtr->SetData(dwDri,frn,*ppDir,szFileNameCode,codeNameLen,codeNameLen>nameLen);

    //����¼����Ŀ¼��������Ŀ¼���������ļ��������
    //�������ϸ���
    //����֤��粻�ܷ���Ŀ¼������
    g_vDirIndex.insert((IndexElemType)NewParentPtr,TRUE);

    //������Index�и���OldParentPtr��Ԫ�� �丸��ΪNewParentPtr
    //ע�⣬�˲�һ��Ҫ�ڸ�Ŀ¼����д��֮��������
    //��Ϊ�˲���ı������������б�ؼ����ܴ�ʱ���ڷ���������
    //����˲���������Ŀ¼�����ж�
    {
        PDIRECTORY_RECORD pDir;
        PNORMALFILE_RECORD pFile;

        int i;
        PIndexElemType  pData, pDataEnd;
        PINDEX_BLOCK_NODE   *pIndex;
        PINDEX_BLOCK_NODE   pNode;
        int cBlock;
        //*****����Ŀ¼����
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                if(pDir->ParentPtr==OldParentPtr) pDir->ParentPtr=NewParentPtr;
            }
        }           
        //*****�����ļ�����
        g_vFileIndex.Lock();
        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData){
                pFile=PNORMALFILE_RECORD(*pData);
                if(pFile->ParentPtr==OldParentPtr) pFile->ParentPtr=NewParentPtr;
            }
        }
        g_vFileIndex.UnLock();
    }
    g_vDirIndex.UnLock();/****************����**********************/
    ListView_RedrawItems(g_hListCtrl,0,100);//�ػ���

#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT RenameNewDirectory:%s\n",buf);
#endif
}

VOID RenameOldDirectory(DWORD dwDri,DWORDLONG frn,PWCHAR fileName,int nameLen)
{
#ifdef OUT_PUT_MONITOR
    char buf[MAX_PATH]={0};
    WideCharToMultiByte(CP_ACP,0,fileName,nameLen,buf,MAX_PATH,0,0);
    DebugTrace("IN RenameOldDirectory:%s\n",buf);
#endif
    PDIRECTORY_RECORD pDir;
    BYTE szFileNameCode[768];//�����Ҫ255*3���ֽ�<768

    //��Ŀ¼���ӵ�Ŀ¼����  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,fileName,nameLen);
    pDir=(PDIRECTORY_RECORD)g_MemoryMgr.GetMemory(GetDirectoryRecordLength(codeNameLen));
    pDir->SimpleSetData(dwDri,frn,szFileNameCode,codeNameLen,codeNameLen>nameLen);
    DWORD dwBasicInfo=GetBasicInfo(dwDri,frn);
    g_vDirIndex.Lock();/****************����**********************/
    {
        OldParentPtr=(PDIRECTORY_RECORD)g_DirMap.erase(dwBasicInfo);
        g_vDirIndex.erase((IndexElemType)pDir,TRUE);
        g_MemDir.Free(OldParentPtr,OldParentPtr->GetLength());//�ڴ��ͷ��ڴ�(δʵ��)
    }
    g_vDirIndex.UnLock();/****************������**********************/
    g_MemoryMgr.FreeMemory((PBYTE)pDir);

#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT RenameOldDirectory:%s\n",buf);
#endif   
}


void AddDirectoryBase(DWORD dwDri,DWORDLONG frn,DWORDLONG parent_frn,PWCHAR pFileName,int nameLen)
{
#ifdef OUT_PUT_MONITOR
    char buf[MAX_PATH]={0};
    WideCharToMultiByte(CP_ACP,0,pFileName,nameLen,buf,MAX_PATH,0,0);
    DebugTrace("IN AddDirectoryBase:%s\n",buf);
#endif
    PDIRECTORY_RECORD pDir;
    BYTE szFileNameCode[768];//�����Ҫ255*3���ֽ�<768

    //��Ŀ¼���ӵ�Ŀ¼����  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,nameLen);
    g_vDirIndex.Lock();/****************����**********************/

#ifdef OUT_PUT_MONITOR
    DebugTrace("in map-find\n");
#endif
    PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(GetBasicInfo(dwDri,parent_frn)); 
#ifdef OUT_PUT_MONITOR
    DebugTrace("out map-find\n");
#endif

    if(ppDir)//����Ŀ¼���ڲŲ���
    {
        ++ppDir;

        pDir=(PDIRECTORY_RECORD)(
            g_MemDir.Alloc(GetDirectoryRecordLength(codeNameLen))
            );   

#ifdef OUT_PUT_MONITOR
        DebugTrace("in map-insert\n");
#endif
        g_DirMap.insert(GetBasicInfo(dwDri,frn),(IndexElemType)pDir);//���뱾Ŀ¼ ��Ŀ¼FRN��
#ifdef OUT_PUT_MONITOR
        DebugTrace("out map-insert\n");
#endif

        pDir->SetData(dwDri,frn,*ppDir,szFileNameCode,codeNameLen,codeNameLen>nameLen);

        //����¼����Ŀ¼��������Ŀ¼���������ļ��������
        //�������ϸ���
        //����֤��粻�ܷ���Ŀ¼������
        g_vDirIndex.insert((IndexElemType)pDir,TRUE);
    }

    g_vDirIndex.UnLock();/****************����**********************/

#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT AddDirectoryBase:%s\n",buf);
#endif
}

VOID DeleteDirectoryBase(DWORD dwDri,DWORDLONG frn,PWCHAR fileName,int nameLen)
{
#ifdef OUT_PUT_MONITOR
    char buf[MAX_PATH]={0};
    WideCharToMultiByte(CP_ACP,0,fileName,nameLen,buf,MAX_PATH,0,0);
    DebugTrace("IN DeleteDirectoryBase:%s\n",buf);
#endif
    PDIRECTORY_RECORD pDir;
    BYTE szFileNameCode[768];//�����Ҫ255*3���ֽ�<768

    //��Ŀ¼���ӵ�Ŀ¼����  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,fileName,nameLen);
    pDir=(PDIRECTORY_RECORD)g_MemoryMgr.GetMemory(GetDirectoryRecordLength(codeNameLen));
    pDir->SimpleSetData(dwDri,frn,szFileNameCode,codeNameLen,codeNameLen>nameLen);
    DWORD dwBasicInfo=GetBasicInfo(dwDri,frn);
    g_vDirIndex.Lock();/****************����**********************/
    {
        g_DirMap.erase(dwBasicInfo);
        g_vDirIndex.erase((IndexElemType)pDir,TRUE);
    }
    g_vDirIndex.UnLock();/****************������**********************/
    g_MemoryMgr.FreeMemory((PBYTE)pDir);
#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT DeleteDirectoryBase:%s\n",buf);
#endif   
}

VOID AddFileBase(DWORD dwDri,DWORDLONG frn,DWORDLONG parent_frn,PWCHAR pFileName,int nameLen)
{ 
#ifdef OUT_PUT_MONITOR
    DebugTrace("IN AddFileBase!!!!!!!\n");
#endif  
    PNORMALFILE_RECORD pFile;
    BYTE szFileNameCode[768];//�����Ҫ255*3���ֽ�<768
    int idExt=0;//��չ��ID
    DWORD dwExtIdLen;//��չ����ռ���ֽ��� 
    DWORD dwIconLen=0;
    int i;
    for(i=nameLen-1;i>=0&&pFileName[i]!=L' '&&pFileName[i]!=L'.';--i);
    if(i<=0 ||pFileName[i]==L' '){
        dwExtIdLen=0;//����չ��
    }else{//
        idExt=g_ExtMgr.insert(pFileName+i+1,nameLen-1-i);
        if(idExt<CExtArray::s_dwOmitExt) dwIconLen=4;
        dwExtIdLen=GetExtIdLength(idExt);
        nameLen=i;
    }
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,nameLen);
    DWORD dwNameLenLength=GetNameLenLength(codeNameLen);
    DWORD dwMemRecord=GetNormalFileRecordLength(codeNameLen,dwNameLenLength,dwExtIdLen,dwIconLen);

    g_vFileIndex.Lock();/****************����**********************/
    {
        PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(GetBasicInfo(dwDri,parent_frn)); 
        if(ppDir)
        {
            ++ppDir;  
            pFile=(PNORMALFILE_RECORD)g_MemFile.Alloc(dwMemRecord);

            pFile->SetData(codeNameLen>nameLen,dwNameLenLength,dwExtIdLen
                ,frn
                ,dwDri,*ppDir
                ,szFileNameCode,codeNameLen
                ,idExt
                );
#ifdef OUT_PUT_MONITOR
            char buf[MAX_PATH]={0};
            WideCharToMultiByte(CP_ACP,0,pFileName,nameLen,buf,MAX_PATH,0,0);
            DebugTrace("IN AddFileBase:%s %x\n",buf,pFile->BasicInfo);
#endif  
            //�ļ��� ��չ���������
            g_vFileIndex.insert((IndexElemType)pFile,FALSE);
        }
    }
    g_vFileIndex.UnLock();/****************������**********************/
#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT AddFileBase!!!!!!!\n");
#endif   
}

VOID DeleteFileBase(DWORD dwDri,DWORDLONG frn,PWCHAR pFileName,int nameLen)
{
    PNORMALFILE_RECORD pFile;
    BYTE szFileNameCode[768];//�����Ҫ255*3���ֽ�<768
    int idExt=0;//��չ��ID
    DWORD dwExtIdLen;//��չ����ռ���ֽ��� 
    DWORD dwIconLen=0;
    int i;
    for(i=nameLen-1;i>=0&&pFileName[i]!=L' '&&pFileName[i]!=L'.';--i);
    if(i<=0 ||pFileName[i]==L' '){
        dwExtIdLen=0;//����չ��
    }else{//
        idExt=g_ExtMgr.find(pFileName+i+1,nameLen-1-i);
        if(idExt<CExtArray::s_dwOmitExt) dwIconLen=4;
        assert(idExt>=0 && "��չ������Ҫ����");
        dwExtIdLen=GetExtIdLength(idExt);
        nameLen=i;
    }
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,nameLen);
    DWORD dwNameLenLength=GetNameLenLength(codeNameLen);
    DWORD dwMemRecord=GetNormalFileRecordLength(codeNameLen,dwNameLenLength,dwExtIdLen,dwIconLen);
    pFile=(PNORMALFILE_RECORD)g_MemoryMgr.GetMemory(dwMemRecord);
    pFile->SimpleSetData(
        codeNameLen>nameLen,dwNameLenLength,dwExtIdLen
        ,frn
        ,dwDri
        ,szFileNameCode,codeNameLen
        ,idExt
        );
#ifdef OUT_PUT_MONITOR
    char buf[MAX_PATH]={0};
    WideCharToMultiByte(CP_ACP,0,pFileName,nameLen,buf,MAX_PATH,0,0);
    DebugTrace("IN DeleteFileBase:%s  %x\n",buf,pFile->BasicInfo);
#endif 

    g_vFileIndex.Lock();/****************����**********************/
    {
        g_vFileIndex.erase((IndexElemType)pFile,FALSE);
    }
    g_vFileIndex.UnLock();/****************������**********************/
    g_MemoryMgr.FreeMemory((PBYTE)pFile);
#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT DeleteFileBase:%s\n",buf);
#endif   
}



BOOL GetReasonString(DWORD dwReason, LPSTR pszReason,int cchReason) {
   static LPCSTR szCJReason[] = {
      "DataOverwrite",         // 0x00000001
      "DataExtend",            // 0x00000002
      "DataTruncation",        // 0x00000004
      "0x00000008",            // 0x00000008
      "NamedDataOverwrite",    // 0x00000010
      "NamedDataExtend",       // 0x00000020
      "NamedDataTruncation",   // 0x00000040
      "0x00000080",            // 0x00000080
      "FileCreate",            // 0x00000100
      "FileDelete",            // 0x00000200
      "PropertyChange",        // 0x00000400
      "SecurityChange",        // 0x00000800
      "RenameOldName",         // 0x00001000
      "RenameNewName",         // 0x00002000
      "IndexableChange",       // 0x00004000
      "BasicInfoChange",       // 0x00008000
      "HardLinkChange",        // 0x00010000
      "CompressionChange",     // 0x00020000
      "EncryptionChange",      // 0x00040000
      "ObjectIdChange",        // 0x00080000
      "ReparsePointChange",    // 0x00100000
      "StreamChange",          // 0x00200000
      "0x00400000",            // 0x00400000
      "0x00800000",            // 0x00800000
      "0x01000000",            // 0x01000000
      "0x02000000",            // 0x02000000
      "0x04000000",            // 0x04000000
      "0x08000000",            // 0x08000000
      "0x10000000",            // 0x10000000
      "0x20000000",            // 0x20000000
      "0x40000000",            // 0x40000000
      "*Close*"                // 0x80000000
   };
   CHAR sz[1024]={0};
   sz[0] = sz[1] = sz[2] = 0;
   for (int i = 0; dwReason != 0; dwReason >>= 1, i++) {
      if ((dwReason & 1) == 1) {
         strcat(sz, ", ");
         strcat(sz, szCJReason[i]);
      }
   }
   BOOL fOk = FALSE;
   if (cchReason > strlen(&sz[2])) {
      strcpy(pszReason, &sz[2]);
      fOk = TRUE;
   }
   return(fOk);
}



VOID ChangeDataBase(DWORD dwDri
                    ,DWORD dwAttr
                    ,DWORD dwReason
                    ,DWORDLONG frn
                    ,DWORDLONG parent_frn
                    ,PWCHAR wszFileName
                    ,DWORD dwNameLen
                    )
{
    BOOL bFileNameChange=FALSE;
    BOOL bDir=dwAttr&FILE_ATTRIBUTE_DIRECTORY;

#ifdef OUT_PUT_MONITOR
    char buf[MAX_PATH]={0},buf2[1024]={0};
    WideCharToMultiByte(CP_ACP,0,wszFileName,dwNameLen,buf,MAX_PATH,0,0);
    GetReasonString(dwReason,buf2,1024);
    DebugTrace("\nIN ChangeDataBase:%s  %x bDir=%d \n",buf,(DWORD)frn,bDir);
    DebugTrace("ԭ��%s\n",buf2);
#endif
    if((USN_REASON_FILE_CREATE&dwReason)&&(USN_REASON_CLOSE&dwReason)){//��
        if(bDir)AddDirectoryBase(dwDri,frn,parent_frn,wszFileName,dwNameLen);
        else AddFileBase(dwDri,frn,parent_frn,wszFileName,dwNameLen); 
        bFileNameChange=TRUE;
    }else if((USN_REASON_RENAME_NEW_NAME&dwReason) && (dwReason&USN_REASON_CLOSE)){//��
        if(bDir)//������Ŀ¼ ����Ŀ¼�仯
            RenameNewDirectory(dwDri,frn,parent_frn,wszFileName,dwNameLen);
        else AddFileBase(dwDri,frn,parent_frn,wszFileName,dwNameLen); 
        bFileNameChange=TRUE;      
    }
    else if(USN_REASON_RENAME_OLD_NAME&dwReason){//ɾ
        if(USN_REASON_FILE_CREATE&dwReason){
            //old ���溬�д���
        }else{
            if(bDir) //������Ŀ¼ ����Ŀ¼�仯
                RenameOldDirectory(dwDri,frn,wszFileName,dwNameLen);
            else DeleteFileBase(dwDri,frn,wszFileName,dwNameLen);
            bFileNameChange=TRUE;
        }
    }else if((dwReason&USN_REASON_FILE_DELETE)&&(USN_REASON_CLOSE&dwReason)){//ɾ
        if(USN_REASON_FILE_CREATE&dwReason){
            //new close ���溬���������ڴ����д���
        }else{
            if(bDir) DeleteDirectoryBase(dwDri,frn,wszFileName,dwNameLen);
            else DeleteFileBase(dwDri,frn,wszFileName,dwNameLen);
            bFileNameChange=TRUE;
        }
    }else {
#ifdef OUT_PUT_MONITOR
        DebugTrace("����������ԭ��\n");
#endif
        //�����ǣ�
        //[RenameNewName|Close] ��ֹ�ظ�����
        //[FileCreate|Close]    ��ֹ�ظ�����
    }


    if(bFileNameChange) {//�����ļ����ı䣬���ǵ�ǰ���봮�Ľṹʱ�����û�����
        if(bDir && !g_bDirSearch) return;//�ı����Ŀ¼ ����ǰ������Ŀ¼
        if(!bDir && !g_bFileSearch) return;//�ı�����ļ� ����ǰ�������ļ�

        //��鱾�ļ��Ƿ�Ե�ǰ��ѯ����Ӱ��
        SearchStrOpt *pStrOpt;
        if(g_bCase){//��Сд����
            pStrOpt=&g_StrOptCase;
        }else{//������                         
            pStrOpt=&g_StrOptNoCase;
        }
#ifdef OUT_PUT_MONITOR
        DebugTrace("Check:\n");
#endif
        BOOL bEffect=FALSE;//���ı����ʾ�����Ӱ��ô?
        if(pStrOpt->HasQuestion()){//����?�����ܺ��С���ͨ��*��*?��
            bEffect=Helper_CheckQuestionSearch(g_bCase,wszFileName,dwNameLen);
        }else{//������?��
            if(pStrOpt->HasStar()){//������?��������*�����ܺ��С���ͨ��*?��
                bEffect=Helper_CheckStarSearch(g_bCase,wszFileName,dwNameLen);
            }else{//������?��*��*a
                if(pStrOpt->HasStarQuestion()){//������?��*��������?*�����ܺ��С���ͨ��
                    bEffect=Helper_CheckStarQuestionSearch(g_bCase,wszFileName,dwNameLen);
                }else{//������?��*��?*����������ͨ
                    //assert(pStrOpt->pNormal && "һ��������ͨ��");//��ͨ���Ѵ���
                    if(pStrOpt->pNormal){//��ȫ*ʱ������ͨ��
                        bEffect=Helper_CheckOnlyNormalSearch(g_bCase,wszFileName,dwNameLen);//����
                    }
                }
            }
        } 
        if(bEffect)//ֱ���޸Ľ��
        {
#ifdef OUT_PUT_MONITOR
            DebugTrace("SendMessage:\n");
#endif
            g_bMonitorChange=TRUE;
            KernelSearch();
            g_bMonitorChange=FALSE;
        }
    }
#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT ChangeDataBase:%s  bDir=%d\n",buf,bDir);
#endif
}


//���Ӹ����̷�
//���񣺸������ݿ�(��չ�� Ŀ¼ �ļ� ����)
DWORD WINAPI MonitorProc(PVOID pParam)
{
    const DWORD SEARCH_TITLE_REASON_FLAG=
        //֧���ļ�������
        USN_REASON_FILE_CREATE              
        |USN_REASON_FILE_DELETE
        |USN_REASON_RENAME_OLD_NAME
        |USN_REASON_RENAME_NEW_NAME
        
        //֧���ļ����Ը��� //���������ʱ��֧��
//         |USN_REASON_DATA_EXTEND
//         |USN_REASON_DATA_OVERWRITE
//         |USN_REASON_DATA_TRUNCATION
//         |USN_REASON_BASIC_INFO_CHANGE   //�ļ����Ըı�
//         |USN_REASON_OBJECT_ID_CHANGE    //LAST ACCESS
        ;

    DWORD dwDri=(DWORD)pParam;
    HANDLE hVol=g_hVols[dwDri];   
    READ_USN_JOURNAL_DATA rujd;
    rujd.BytesToWaitFor=0;//�˴���1�ĳ�0 0ʱ�ȴ���¼���������
    rujd.ReasonMask=SEARCH_TITLE_REASON_FLAG;
    rujd.ReturnOnlyOnClose=0;
    rujd.StartUsn=g_curNextUSN[dwDri];
    rujd.Timeout=0;
    rujd.UsnJournalID=g_curJournalID[dwDri];

    DWORD dwBytes;
    DWORD dwRetBytes;
    BYTE Buffer[USN_PAGE_SIZE];
    PUSN_RECORD pRecord; 

//     char buf[MAX_PATH];
//     sprintf(buf,"C:\\Documents and Settings\\Administrator\\����\\%c.TXT",dwDri+'A');
//     FILE *fp=fopen(buf,"w");

    for(;;)
    {
        Sleep(rand()%1000+1000);
//         fprintf(fp,"����DeviceIoControl\n");
//         fflush(fp);
        if(!DeviceIoControl(hVol, 
            FSCTL_READ_USN_JOURNAL, 
            &rujd,
            sizeof(rujd),
            Buffer,
            USN_PAGE_SIZE,
            &dwBytes,
            NULL) )
        {
            DWORD dwError=GetLastError();
            DebugStringA("%c �߳� FSCTL_READ_USN_JOURNAL ����%d",'A'+dwDri,dwError);
            switch(dwError)
            {
            case ERROR_INVALID_FUNCTION:
                DebugStringA("ERROR_INVALID_FUNCTION");               
                break;
            case ERROR_INVALID_PARAMETER:
                DebugStringA("ERROR_INVALID_PARAMETER");
                break;
            case ERROR_JOURNAL_DELETE_IN_PROGRESS:
                DebugStringA("ERROR_JOURNAL_DELETE_IN_PROGRESS");
                DWORD cb;
                DELETE_USN_JOURNAL_DATA del_ujd;
                del_ujd.UsnJournalID = rujd.UsnJournalID;
                del_ujd.DeleteFlags = USN_DELETE_FLAG_NOTIFY;
                if(!DeviceIoControl(hVol, FSCTL_DELETE_USN_JOURNAL, 
                    &del_ujd, sizeof(DELETE_USN_JOURNAL_DATA), 
                    NULL, 0, &cb, NULL
                    )) {
                }
                CreateUsnJournal(hVol,0x2000000,0x400000); 
                USN_JOURNAL_DATA ujd;
                QueryUsnJournal(hVol,&ujd);
                g_curJournalID[dwDri]=ujd.UsnJournalID;
                g_curNextUSN[dwDri]=ujd.NextUsn;
                break;
            case ERROR_JOURNAL_NOT_ACTIVE:
                DebugStringA("ERROR_JOURNAL_NOT_ACTIVE");
                break;
            case ERROR_JOURNAL_ENTRY_DELETED:
                DebugStringA("ERROR_JOURNAL_ENTRY_DELETED");

                break;
            default:
                DebugStringA("ERROR_UNKNOWN");
                break;
            }
            DebugBreak();
        }
//         fprintf(fp,"��������DeviceIoControl\n");
//         fflush(fp);
        if(dwBytes<=sizeof(USN)) {continue;}//����!

        dwRetBytes = dwBytes - sizeof(USN);//������1��USN����USN����һ�۲�ѯ���
        //������û����¼
        //�Ľ� �������еļ��Ӽ�¼��������
        pRecord = PUSN_RECORD((PBYTE)Buffer+sizeof(USN));  
        g_Lock.Lock();//������ر�ʱֹͣ���룬�����ظ��ļ�����
        while(dwRetBytes > 0 )//������1��USN�󣬻��ж����ֽڣ���������1����¼
        {
            ChangeDataBase(
                dwDri
                ,pRecord->FileAttributes
                ,pRecord->Reason
                ,pRecord->FileReferenceNumber
                ,pRecord->ParentFileReferenceNumber
                ,PWCHAR(pRecord->FileName)
                ,pRecord->FileNameLength>>1
                );  
            dwRetBytes -= pRecord->RecordLength;
            //����һ����¼
            pRecord = (PUSN_RECORD)(((PBYTE)pRecord) + pRecord->RecordLength); 
        }
        //������ʼUSN
        rujd.StartUsn = g_curNextUSN[dwDri]=*(USN*)Buffer; 
        g_Lock.UnLock();
    }
}


DWORD InitScanMftProc(PVOID pParam)
{
    HWND hMainWnd=(HWND)pParam;
    DWORD dwDri;   
    BYTE RecvBuffer[sizeof(DWORDLONG) + 0x80000];

    for(dwDri=0;dwDri<26;++dwDri)
    {
        if(g_hVols[dwDri])//��NTFS����Ϣ�ѻ�ȡ
        {
            Helper_SetCurrentState(-1,"����ɨ��(%c)�̣����Ժ�...",dwDri+'A');

            //ö��USN
            MFT_ENUM_DATA med;
            med.StartFileReferenceNumber = 0;
            med.LowUsn = 0;
            med.HighUsn = g_curNextUSN[dwDri];
            DWORD cbRet;
            PUSN_RECORD pRecord,pEnd;
            int codeNameLen;    //�ļ���ռ�ڴ��С
            DWORD dwMemRecord;//��¼��ռ�ռ�
            PWCHAR pFileName;
            PNORMALFILE_RECORD pFile;
            PDIRECTORY_RECORD pDir;
            BYTE szFileNameCode[768];//�����Ҫ255*3���ֽ�<768


            {//�ȰѸ��ӽ�ȥ
                //C:  D: ... ROOT=0x000000000005 Parent
                //��������ռ�
                dwMemRecord=GetDirectoryRecordLength(2);
                //����ռ�
                pDir=(PDIRECTORY_RECORD)g_MemDir.PushBack(dwMemRecord);
                //�������
                pDir->BasicInfo=GetBasicInfo(dwDri,5);
                pDir->ParentBasicInfo=0;
                pDir->NameLength=2;
                pDir->Name[0]=dwDri+'A';
                pDir->Name[1]=':';
                //��������
                g_vDirIndex.push_back((IndexElemType)pDir);
                g_DirMap.push_back(GetBasicInfo(dwDri,5),(IndexElemType)pDir);
            }

            DWORD dwLastBasic=0;
            HANDLE hVolume=g_hVols[dwDri];
            while (DeviceIoControl(hVolume, FSCTL_ENUM_USN_DATA, 
                &med, sizeof(med),
                RecvBuffer, sizeof(RecvBuffer), &cbRet, 
                NULL)
                ) 
            {
                for(pRecord = (PUSN_RECORD) &RecvBuffer[sizeof(USN)],pEnd=PUSN_RECORD(RecvBuffer + cbRet);
                    pRecord<pEnd;
                    pRecord = (PUSN_RECORD) ((PBYTE)pRecord + pRecord->RecordLength)
                    )
                {
                    pFileName=pRecord->FileName;
                    int i,iLen=pRecord->FileNameLength>>1;
                    if(pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY)
                    {   
                        codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,iLen);
                        dwMemRecord=GetDirectoryRecordLength(codeNameLen);
                        pDir=(PDIRECTORY_RECORD)g_MemDir.PushBack(dwMemRecord);
                        pDir->InitializeData(dwDri
                            ,pRecord->FileReferenceNumber
                            ,pRecord->ParentFileReferenceNumber
                            ,szFileNameCode,codeNameLen
                            ,codeNameLen>iLen
                            );
                        g_vDirIndex.push_back((IndexElemType)pDir);
                        g_DirMap.push_back(GetBasicInfo(dwDri,pRecord->FileReferenceNumber),(IndexElemType)pDir);
                    }
                    else
                    {
                        int idExt=0;//��չ��ID
                        DWORD dwExtIdLen;//��չ����ռ���ֽ���  
                        DWORD dwIconLen=0;
                        for(i=iLen-1;i>=0&&pFileName[i]!=L' '&&pFileName[i]!=L'.';--i);
                        if(i<=0 ||pFileName[i]==L' '){
                            dwExtIdLen=0;//����չ�� ע�⣬����ļ�����.��ͷ ��ӷ�. �ո��ַ�������Ϊ����չ�� ��ʱi==0
                        }else{//
                            idExt=g_ExtMgr.insert(pFileName+i+1,iLen-1-i);
                            if(idExt<CExtArray::s_dwOmitExt) dwIconLen=4;
                            dwExtIdLen=GetExtIdLength(idExt);
                            iLen=i;
                        }
                        
                        //iLenΪUCS2�ļ����� ȥ.��չ��
                        codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,iLen);                       
                        DWORD dwNameLenLength=GetNameLenLength(codeNameLen);
                        dwMemRecord=GetNormalFileRecordLength(codeNameLen,dwNameLenLength,dwExtIdLen,dwIconLen);
                        pFile=(PNORMALFILE_RECORD)g_MemFile.PushBack(dwMemRecord);
                        pFile->InitializeData(codeNameLen>iLen,dwNameLenLength,dwExtIdLen
                            ,pRecord->FileReferenceNumber
                            ,dwDri,pRecord->ParentFileReferenceNumber
                            ,szFileNameCode,codeNameLen
                            ,idExt
                            );
                        if(pFile->BasicInfo==0x18030d74)
                        {
                            int jjj=0;
                            ++jjj;
                        }
                        g_vFileIndex.push_back((IndexElemType)pFile);
                    }
                }
                med.StartFileReferenceNumber=*(DWORDLONG*)RecvBuffer;
            }
        }
    }

    Helper_SetCurrentState(-1,"���ڹ���Ŀ¼���ݿ⣬���Ժ�...");
    //����g_DirMap ���� g_vDirIndex�Կ�
    //������ͳһʹ��g_DirMap��ǿ����ɶ���
    {
        PDIRECTORY_RECORD pDir;
        PNORMALFILE_RECORD pFile;

        int i;
        PIndexElemType  pData, pDataEnd;
        PINDEX_BLOCK_NODE   *pIndex;
        PINDEX_BLOCK_NODE   pNode;
        int cBlock;

        //�ȹ���Ŀ¼����
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();

        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                if(pDir->ParentBasicInfo)
                {
                    PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(pDir->ParentBasicInfo);
                    if(ppDir) pDir->ParentPtr=*(ppDir+1);
                    else//�丸δ������������VISTA WIN7�� \$Extend\$RmMetadata
                    {
                        g_vDirIndex.DecreaseSize();
                        //��������ɾ������
                        --pDataEnd;
                        for(PIndexElemType p=pData;p<pDataEnd;++p)
                        {
                            *p=*(p+1);
                        }
                        pNode->CurrentEnd-=1;
                        --pData;                      
                    }                    
                }
                //Ϊ0ʱΪ��Ŀ¼��������
            }
        }

        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData){
                pFile=PNORMALFILE_RECORD(*pData);
                PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(pFile->ParentBasicInfo);
                if(ppDir) pFile->ParentPtr=*(ppDir+1);
                else//�丸δ������
                {
                    g_vFileIndex.DecreaseSize();
                    //��������ɾ������
                    --pDataEnd;
                    for(PIndexElemType p=pData;p<pDataEnd;++p)
                    {
                        *p=*(p+1);
                    }
                    pNode->CurrentEnd-=1;
                    --pData;                      
                }  
            }
        }
    }

    DWORD nDirSize=g_vDirIndex.size();
    DWORD nFileSize=g_vFileIndex.size();
    PIndexElemType pTempHead,pTempByte;;
    pTempHead=(PIndexElemType)g_MemoryMgr.GetMemory(sizeof(IndexElemType)*(nFileSize>nDirSize?nFileSize:nDirSize));  

    Helper_SetCurrentState(-1,"���������ļ��У����Ժ�...");
    {
        //����������pTempByte
        pTempByte=pTempHead;
        int i;
        IndexElemType *pData,*pDataEnd;
        PINDEX_BLOCK_NODE   *pIndex;
        PINDEX_BLOCK_NODE   pNode;
        int cBlock;
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();

        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                *pTempByte++=*pData;
            }
        }
        //��pTempHead[0,...,nDirSize-1]��������
#ifdef MS_QSORT
        qsort(pTempHead,nDirSize,4,pcomp_dir);
#else
        name_qsort(pTempHead,nDirSize,comp_dir);
#endif
        pTempByte=pTempHead;
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                *pData=*pTempByte++;
            }
        }
    }

    Helper_SetCurrentState(-1,"���������ļ������Ժ�...");
    {
        pTempByte=pTempHead;
        int i;
        PIndexElemType pData,pDataEnd;
        PINDEX_BLOCK_NODE   *pIndex;
        PINDEX_BLOCK_NODE   pNode;
        int cBlock;
        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData){
                *pTempByte++=*pData;
            }
        }
#ifdef MS_QSORT
        qsort(pTempHead,nFileSize,4,pcomp_file);
#else
        name_qsort(pTempHead,nFileSize,comp_file);
#endif
        pTempByte=pTempHead;
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                *pData=*pTempByte++;
            }
        }
    }
    g_MemoryMgr.FreeMemory((PBYTE)pTempHead);

//     QueryPerformanceCounter(&_end);
//     LARGE_INTEGER _freq;
//     QueryPerformanceFrequency(&_freq);
//     char buf[1000];
//     sprintf(buf,"��ʱ %f �룡",(_end.QuadPart-_beg.QuadPart)*1.0/_freq.QuadPart);
//     SetWindowTextA(hMainWnd,buf);
    return 0L;
}

//��������ȡ��չ��
//����������� ����TRUE
BOOL GetExtIdFromStr(PWCHAR pStr,int *pIdExt,const int nMax,int &nExt)
{
    BOOL bRet=FALSE;
    PWCHAR pWch;
    nExt=0;
    for(;;)
    {
        for(;*pStr && L' '==*pStr;++pStr);
        if(!*pStr) break;
        bRet=TRUE;
        for(pWch=pStr;*pWch && *pWch!=L' ';++pWch);

        int iExt=g_ExtMgr.find(pStr,pWch-pStr);
        if(iExt!=-1) {
            for(int i=0;i<nExt;++i) {
                if(iExt==pIdExt[i]){
                    iExt=-1;
                    break;
                }
            }
            if(iExt!=-1) {
                pIdExt[nExt++]=iExt;
                if(nExt==nMax) break;//�ﵽ�����չ����
            }
        }
        pStr=pWch;
    }
    return bRet;
}

void KernelSearch()
{
    if(g_bCanSearch)//���Բ�ѯ
    {
        //��ȡ��ǰĿ¼����
        DWORD *pBasicInfo=0;
        BOOL  *pbSubDir=0;
        int nCheck=g_listDirFilter.GetFileCheckState(pBasicInfo,pbSubDir);
        PDIRECTORY_RECORD* pDirs=NULL;
        
        if(nCheck>0){
            pDirs=new PDIRECTORY_RECORD[nCheck];
            //��ȡ��Ӧ��Ŀ¼��Ϣ
            g_vDirIndex.Lock();
            for(int i=0;i<nCheck;++i)
            {
                PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(pBasicInfo[i]);
                assert(ppDir && "Ŀ¼һ�����ڿ���");
                pDirs[i]=*(ppDir+1);
            }
            g_vDirIndex.UnLock();
            delete []pBasicInfo;
            pBasicInfo=NULL;
        }
              
        static WCHAR szSearchStr[1024];//��ǰҪ��ѯ�Ĵ�
        GetWindowTextW(g_hEdit,szSearchStr,1024);

        static WCHAR szExtSearch[512];
        GetWindowTextW(g_hExtEdit,szExtSearch,512);//��ȡ��չ����

        int IdExt[64];//���֧��64����չ��
        int nExt;       //������չ������
        BOOL bHasExt=GetExtIdFromStr(szExtSearch,IdExt,64,nExt);
        int *pIdExt;
        if(!bHasExt) pIdExt=NULL;
        else pIdExt=IdExt;

        BOOL bAvaiable=TRUE;
        if(g_bCase){//��Сд����
            if(!ArrangeSearchStrCase(szSearchStr)){
                bAvaiable=FALSE;
            }
        }else{//������
            if(!ArrangeSearchStrNoCase(szSearchStr)){
                bAvaiable=FALSE;
            }  
        }

        if(!bAvaiable ||(bHasExt && 0==nExt) ||0==nCheck){//����Ķ�����Ч
            g_vDirOutPtr.clear_all();
            g_vFileOutPtr.clear_all();
            g_LastStrOptCase.Reset();
            g_LastStrOptNoCase.Reset();
            Helper_SetCurrentState(0,NULL); 
            ListView_SetItemCount(g_hListCtrl,0);
            ListView_RedrawItems(g_hListCtrl,0,-1);
        }else{
            SearchStrOpt *pStrOpt;
                //�ȼ�����봮�Ƿ��֮ǰ�Ĵ���ͬ
            BOOL bNameChange=FALSE;
            if(g_bCase){
                if(!(g_StrOptCase==g_LastStrOptCase)) bNameChange=TRUE;
            }else{
                if(!(g_StrOptNoCase==g_LastStrOptNoCase)) bNameChange=TRUE;
            }

            if( g_bDirSetting           //Ŀ¼ѡ��ı�
                ||g_bOptChanged         //ѡ��ı�
                ||g_bMonitorChange      //���Ӹı�
                ||bNameChange           //���������ı�
                ||g_bExtChange          //������չ�����ı�                    
                )
            {
                if(g_bCase){
                    g_LastStrOptCase=g_StrOptCase;
                    pStrOpt=&g_StrOptCase;
                }else{
                    g_LastStrOptNoCase=g_StrOptNoCase;  
                    pStrOpt=&g_StrOptNoCase;
                }
            }
            else goto OVER;

            g_vDirOutPtr.clear();
            g_vFileOutPtr.clear();

            if(pStrOpt->HasQuestion()){//����?�����ܺ��С���ͨ��*��*?��
                Helper_QuestionSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);
            }else{//������?��
                if(pStrOpt->HasStar()){//������?��������*�����ܺ��С���ͨ��*?��
                    Helper_StarSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);
                }else{//������?��*��*a
                    if(pStrOpt->HasStarQuestion()){//������?��*��������?*�����ܺ��С���ͨ��
                        Helper_StarQuestionSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);
                    }else{//������?��*��?*����������ͨ
                        //assert(pStrOpt->pNormal && "һ��������ͨ��");//��ͨ���Ѵ���
                        if(pStrOpt->pNormal){//��ȫ*ʱ������ͨ��
                            if(pStrOpt->_bAllChar){
                                Helper_OnlyNormalSearchAscii(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);//AC�㷨��ģʽ������Ԥ�����������һ������ǰ���г��֡�
                            }
                            else{
                                Helper_OnlyNormalSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);//����
                            }
                        }
                    }
                }
            } 
            //���ļ����ļ���ѡ����һδָ��ʱ����size==0
            Helper_SetCurrentState(g_vFileOutPtr.size()+g_vDirOutPtr.size(),NULL);
            ListView_SetItemCount(g_hListCtrl,g_vFileOutPtr.size()+g_vDirOutPtr.size());
        }
        g_bOptChanged=FALSE;//����ʱ�Ѿ�����
        g_bDirSetting=FALSE;

OVER:
        delete []pbSubDir;
        delete []pDirs;
    }
}

void Dlg_OnCommand(HWND hwnd, int idCtl, HWND hwndCtl, UINT codeNotify) 
{
    switch (idCtl) 
    {
    case IDM_DIRECTOION:
        {
            //MessageBoxA(hwnd,"�����Ʒ�ĵ�","��������",MB_ICONINFORMATION);
            //ShellExcute
        }break;
    case IDM_ABOUT:
        {
        }break;
    case IDM_CASE://��Сд�����Ӳ˵�
        {
            HMENU hMenu=::GetMenu(hwnd);
            HMENU hSubMenu=::GetSubMenu(hMenu,0);
            if(g_bCase)
            {
                ::CheckMenuItem(hSubMenu,0,MF_UNCHECKED|MF_BYPOSITION);
            }else
            {
                ::CheckMenuItem(hSubMenu,0,MF_CHECKED|MF_BYPOSITION);
            }
            g_bCase=!g_bCase;
            g_bOptChanged=TRUE;
            KernelSearch();
            g_bOptChanged=FALSE;
        }
        break;
    case IDM_SEARCH_FILE:
        {
            HMENU hMenu=::GetMenu(hwnd);
            HMENU hSubMenu=::GetSubMenu(hMenu,0);
            if(g_bFileSearch)
            {
                if(!g_bDirSearch){
                    MessageBoxA(hwnd,"�ļ����ļ�������ָ��һ����","�������ѣ�",MB_ICONINFORMATION);
                    return;
                }
                ::CheckMenuItem(hSubMenu,2,MF_UNCHECKED|MF_BYPOSITION);
            }else
            {
                ::CheckMenuItem(hSubMenu,2,MF_CHECKED|MF_BYPOSITION);
            }
            g_bFileSearch=!g_bFileSearch;
            g_bOptChanged=TRUE;
            KernelSearch();
            g_bOptChanged=FALSE;
        }break;
    case IDM_SEARCH_DIR:
        {
            HMENU hMenu=::GetMenu(hwnd);
            HMENU hSubMenu=::GetSubMenu(hMenu,0);
            if(g_bDirSearch)
            {
                if(!g_bFileSearch){
                    MessageBoxA(hwnd,"�ļ����ļ�������ָ��һ����","�������ѣ�",MB_ICONINFORMATION);
                    return;
                }
                ::CheckMenuItem(hSubMenu,1,MF_UNCHECKED|MF_BYPOSITION);
            }else
            {
                ::CheckMenuItem(hSubMenu,1,MF_CHECKED|MF_BYPOSITION);
            }
            g_bDirSearch=!g_bDirSearch;
            g_bOptChanged=TRUE;
            KernelSearch();
            g_bOptChanged=FALSE;
        }break;
    case IDM_DIR_SETTING:
        {
            HMENU hMenu=::GetMenu(hwnd);
            if(g_bDirSetExpand){
                ModifyMenuW(hMenu,1,MF_BYPOSITION|MF_STRING,IDM_DIR_SETTING,L"չ��Ŀ¼����(&M)");
            }else{
                if(g_bFilterExpand){
                    //�Ĳ˵���Ϊ������Ŀ¼����
                    ModifyMenuW(hMenu,2,MF_BYPOSITION|MF_STRING,IDM_FILTER,L"չ������ѡ��(&G)");
                    g_bFilterExpand=!g_bFilterExpand;  
                }               
                ModifyMenuW(hMenu,1,MF_BYPOSITION|MF_STRING,IDM_DIR_SETTING,L"����Ŀ¼����(&M)");
            }
            g_bDirSetExpand=!g_bDirSetExpand;  
            DrawMenuBar(hwnd);
            UpdateLayout(hwnd);
        }
        break;
    case IDM_FILTER:
        {
            HMENU hMenu=::GetMenu(hwnd);
            if(g_bFilterExpand){
                //�Ĳ˵���Ϊ������Ŀ¼����
                ModifyMenuW(hMenu,2,MF_BYPOSITION|MF_STRING,IDM_FILTER,L"չ������ѡ��(&G)");
            }else{
                //�Ĳ˵���Ϊ��չ��Ŀ¼����
                ModifyMenuW(hMenu,2,MF_BYPOSITION|MF_STRING,IDM_FILTER,L"�������ѡ��(&G)");
                if(g_bDirSetExpand){
                    ModifyMenuW(hMenu,1,MF_BYPOSITION|MF_STRING,IDM_DIR_SETTING,L"չ��Ŀ¼����(&M)");
                    g_bDirSetExpand=!g_bDirSetExpand; 
                }
            }
            g_bFilterExpand=!g_bFilterExpand;
            DrawMenuBar(hwnd);
            UpdateLayout(hwnd);
        }
        break;
    case IDC_EDIT1://�ļ����༭��
        {
            if(EN_CHANGE==codeNotify){
                if(g_bJustFilter)
                {
                    g_bJustFilter=FALSE;
                    g_bCanSearch=TRUE;
                }
                KernelSearch();
            }           
        }break;
    case IDC_EDIT2:
        {
            if(EN_CHANGE==codeNotify){
                if(g_bJustFilter)
                {
                    g_bJustFilter=FALSE;
                    g_bCanSearch=TRUE;
                }
                g_bExtChange=TRUE;
                KernelSearch();
                g_bExtChange=FALSE;
            }   
        }break;
#pragma region Ŀ¼����ѡ��
    case IDC_BTN_ADDDIR://���һ��Ŀ¼
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.AddFilterDirectory()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break;      
    case IDC_BTN_DELDIR://ɾ��ָ����Ŀ¼
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.DeleteItemsSelected()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break; 
    case IDC_BTN_DELALLDIR://ɾ������Ŀ¼
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.DeleteAllDirItems()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break;
    case IDC_BTN_CHECKALLDRI://��ѡ�����̷�
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.CheckAllDriverItems()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break;
#pragma endregion

#pragma region �������˰�ť
    case IDC_BTN_FILTER:
        {
            if(BN_CLICKED==codeNotify) g_Filter.Go();
        }break;
    case IDC_CHECK_SIZE:
        {
            if(BN_CLICKED==codeNotify){
                g_Filter.SetSizeUse(SendMessage(hwndCtl,BM_GETCHECK,0,0)==BST_CHECKED);
            }
        }break;
    case IDC_CHECK_DATE:
        {
            if(BN_CLICKED==codeNotify){
                g_Filter.SetDateUse(SendMessage(hwndCtl,BM_GETCHECK,0,0)==BST_CHECKED);
            }
        }break;
    case IDC_CHECK_ATTR:
        {
            if(BN_CLICKED==codeNotify){
                g_Filter.SetAttrUse(SendMessage(hwndCtl,BM_GETCHECK,0,0)==BST_CHECKED);
            }
        }break;
#pragma endregion
    default:
        break;
    }
}


/**
 *	Function:
 *      ��ȡ�������ļ�����Ϣ
 *	Parameter(s):
 *      iItemIndex �ڼ���
 *      path ��Ŀ¼��L'\\' ����ǰĿ¼Ϊ������Ŀ¼Ϊ�� 
 *      fileName С�ļ���������չ��
 *      extName ��չ��
 *	Return:	
 *      path�ĳ��� Ϊ0��ʾ��Ŀ¼
 *	Commons:
 **/
int GetFileName(IN int iItemIndx,OUT PWCHAR path,OUT PWCHAR fileName,OUT PWCHAR extName=NULL)
{
    PBYTE pName;    //�ļ�����Ϣ����ָ��
    int fileLen;    //�����ļ�����
    int nameLen;    //�����ļ�����

    PDIRECTORY_RECORD pCurDir;      //�������������ֵ�ǰ��ʾ����Ŀ¼�����ļ���
    PNORMALFILE_RECORD pCurFile;    //���ұ��浱ǰ��¼�׵�ַ

    PDIRECTORY_RECORD pCurParent;
    if(iItemIndx<g_vDirOutPtr.size()){//Ŀ¼
        pCurDir=PDIRECTORY_RECORD(g_vDirOutPtr[iItemIndx]);
        pCurFile=NULL;
        pCurParent=pCurDir->ParentPtr;
        nameLen=pCurDir->GetCodeName(pName);
    }else{//�ļ�
        pCurDir=NULL;
        pCurFile=PNORMALFILE_RECORD(g_vFileOutPtr[iItemIndx-g_vDirOutPtr.size()]);
        pCurParent=pCurFile->ParentPtr;
        nameLen=pCurFile->GetCodeName(pName);
    }
    fileLen=Helper_CodeToUcs2Case(fileName,pName,nameLen);
    
    if(pCurFile){
        pName+=nameLen;//��չ��λ��
        int idExt=pCurFile->GetExtendID(pName);
        if(idExt!=-1){
            PWCHAR pWch=g_ExtMgr.GetExtName(idExt);
            if(extName) wcscpy(extName,pWch);
            fileName[fileLen++]=L'.';
            for(;;)
            {
                fileName[fileLen]=*pWch;
                if(L'\0'==*pWch) break;
                ++fileLen;
                ++pWch;
            }
        }
    }
    return Helper_GetPath(path,pCurParent);
}



BOOL Dlg_OnNotify(HWND hWnd,int idCtrl,LPNMHDR pNMHDR)
{
    switch(idCtrl)
    {
    case IDC_LIST1:
        {
            if(pNMHDR->code==LVN_GETDISPINFO){
                LV_ITEM *pItem=&(((NMLVDISPINFO*)pNMHDR)->item);
                int iItemIndx= pItem->iItem;
 
                static PBYTE pName;
                static WCHAR sFileName[MAX_PATH];
                static int fileLen;

                static WCHAR path[MAX_PATH];//Ŀ¼
                static int pathLen;

                static PDIRECTORY_RECORD pCurDir;//�������������ֵ�ǰ��ʾ����Ŀ¼�����ļ���
                static PNORMALFILE_RECORD pCurFile;//���ұ��浱ǰ��¼�׵�ַ

                static PWCHAR pExt;//��չ��
                static int iIcon;

                static ULONGLONG fileSize;
                static DWORDLONG llFileCreateTm,llFileLastAccessTm,llFileLastChangeTm;
                static DWORD FileAttri;

                if (pItem->mask & LVIF_TEXT)
                {
                    switch(pItem->iSubItem)
                    {
                    case 0:
                        {   
                            PDIRECTORY_RECORD pCurParent;
                            int nameLen;
                            pCurDir=NULL;
                            pCurFile=NULL;
                            if(iItemIndx<g_vDirOutPtr.size()){//Ŀ¼
                                pCurDir=PDIRECTORY_RECORD(g_vDirOutPtr[iItemIndx]);
                                Helper_GetBasicInformation(pCurDir->BasicInfo>>27,pCurDir->BasicInfo&0x7ffffff
                                    ,&llFileCreateTm,&llFileLastAccessTm,&llFileLastChangeTm
                                    ,NULL,&FileAttri);
                                pCurParent=pCurDir->ParentPtr;
                                nameLen=pCurDir->GetCodeName(pName);
                            }else{//�ļ�
                                pCurFile=PNORMALFILE_RECORD(g_vFileOutPtr[iItemIndx-g_vDirOutPtr.size()]);
                                Helper_GetBasicInformation(pCurFile->ParentPtr->BasicInfo>>27,pCurFile->BasicInfo&0x7ffffff
                                    ,&llFileCreateTm,&llFileLastAccessTm,&llFileLastChangeTm
                                    ,&fileSize,&FileAttri);
                                pCurParent=pCurFile->ParentPtr;
                                nameLen=pCurFile->GetCodeName(pName);
                            }
                            fileLen=Helper_CodeToUcs2Case(sFileName,pName,nameLen);
                            wcsncpy(pItem->pszText,sFileName,pItem->cchTextMax);

                            if(pCurDir){//��ǰitem��Ŀ¼     
                                int pathLen=Helper_GetPath(path,pCurParent);
                                if(0==pathLen) iIcon=g_iRootIcon;
                                else iIcon=g_iDirIcon;
                                pExt=NULL;                                    
                            }
                            else
                            {//�ڻ�ȡ��pFileName֮�������չ��
                                pName+=nameLen;//��չ��λ��
                                DWORD dwExtLen;
                                int idExt=pCurFile->GetExtendID(pName,&dwExtLen);
                                if(-1==idExt){//����չ��
                                    iIcon=3;
                                    pathLen=Helper_GetPath(path,pCurParent);
                                    pExt=NULL;
                                }else{//����չ��
                                    pExt=g_ExtMgr.GetExtName(idExt);

                                    //�����ļ���
                                    PWCHAR pWch=pExt;         
                                    sFileName[fileLen++]=L'.';
                                    for(;;)
                                    {
                                        sFileName[fileLen]=*pWch;
                                        if(L'\0'==*pWch) break;
                                        ++fileLen;
                                        ++pWch;
                                    }
                                    pathLen=Helper_GetPath(path,pCurParent);  

                                    if(idExt<CExtArray::s_dwOmitExt){//ICON���ڱ���
                                                                    //Ҳ��ͨ��BasicInfo��BITMASK�ж�
                                        DWORD *pIcon=(DWORD*)(pName+dwExtLen);
                                        if(0xFFFFFFFF==*pIcon){
                                            *pIcon=Helper_GetFileIconIndex(path,pathLen,sFileName,fileLen);
                                        }
                                        iIcon=*pIcon;   
                                    }else{//ͨ����չ�������ȡICON
                                        iIcon=g_ExtMgr.GetIconIndex(idExt);  
                                        if(iIcon==ICON_INDEX_UNINITIALIZED){
                                            iIcon=g_ExtMgr.SetIconIndex(idExt,path,pathLen,sFileName,fileLen);
                                        }
                                    }
                                }

                            }//!bDir
                        }
                        break;
                    case 1://��ʾ��չ��
                        { 
                            if(pExt!=NULL){
                                wcsncpy(pItem->pszText,pExt,pItem->cchTextMax);
                            }
                        }
                        break;
                    case 2: 
                        {
                            wcsncpy(pItem->pszText,path,pItem->cchTextMax);
                        }
                        break;
                    case 3: //��ʾ�ļ�(��Ŀ¼)��С
                        {
                            if(pCurFile)
                            {
                                char szInfo[62];
                                if(fileSize<1024){
                                    sprintf(szInfo,"%d B",fileSize);
                                }
                                else{
                                    double fFileSize=fileSize;
                                    if(fileSize<1048576){
                                        sprintf(szInfo,"%0.2f KB",fFileSize/1024);
                                    }
                                    else{
                                        if(fileSize<1073741824){
                                            sprintf(szInfo,"%0.2f MB",fFileSize/1048576);
                                        }else{
                                            sprintf(szInfo,"%0.2f GB",fFileSize/1073741824);
                                        }
                                    }   
                                }  
                                PWCHAR pWch=pItem->pszText;
                                for(char *pCh=szInfo;;pCh++)
                                {
                                    *pWch++=*pCh;
                                    if('\0'==*pCh) break;
                                }
                            }
                        }
                        break;
                    case 4://����ʱ��
                        {
                            SYSTEMTIME fileCreateTm;
                            FILETIME fileTime;
                            FileTimeToLocalFileTime((PFILETIME)&llFileCreateTm,&fileTime);
                            FileTimeToSystemTime(&fileTime,&fileCreateTm);
                            wsprintfW(pItem->pszText,L"%d��%d��%d�� %dʱ%d��"
                                ,fileCreateTm.wYear,fileCreateTm.wMonth,fileCreateTm.wDay
                                ,fileCreateTm.wHour,fileCreateTm.wMinute
                                );
                        }
                        break;
                    case 5://����޸�ʱ��
                        {
                            SYSTEMTIME fileLastChangeTm;
                            FILETIME fileTime;
                            FileTimeToLocalFileTime((PFILETIME)&llFileLastChangeTm,&fileTime);
                            FileTimeToSystemTime(&fileTime,&fileLastChangeTm);
                            wsprintfW(pItem->pszText,L"%d��%d��%d�� %dʱ%d��"
                                ,fileLastChangeTm.wYear,fileLastChangeTm.wMonth,fileLastChangeTm.wDay
                                ,fileLastChangeTm.wHour,fileLastChangeTm.wMinute
                                );
                        }
                        break;
                    case 6://�������ʱ��
                        {
                            SYSTEMTIME fileLastAccessTm;
                            FILETIME fileTime;
                            FileTimeToLocalFileTime((PFILETIME)&llFileLastAccessTm,&fileTime);
                            FileTimeToSystemTime(&fileTime,&fileLastAccessTm);
                            wsprintfW(pItem->pszText,L"%d��%d��%d�� %dʱ%d��"
                                ,fileLastAccessTm.wYear,fileLastAccessTm.wMonth,fileLastAccessTm.wDay
                                ,fileLastAccessTm.wHour,fileLastAccessTm.wMinute
                                );
                        }
                        break;
                    case 7:
                        {
                            PWCHAR pWch=pItem->pszText;
                            if(0==(FileAttri&(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))){
                                *pWch++=L'[';*pWch++=L'��';*pWch++=L'ͨ';*pWch++=L']';
                            }else{
                                *pWch++=L'[';
                                if(FileAttri&FILE_ATTRIBUTE_HIDDEN)
                                {
                                    *pWch++=L'��';*pWch++=L'��';*pWch++=L',';
                                }
                                if(FileAttri&FILE_ATTRIBUTE_SYSTEM)
                                {
                                    *pWch++=L'ϵ';*pWch++=L'ͳ';*pWch++=L',';
                                }   
                                *(pWch-1)=L']';
                            }
                            if(NULL==pCurFile){
                                wcscpy(pWch,L"�ļ���");
                            }else{
                                wcscpy(pWch,L"�ļ�");
                            }                     
                        }
                        break;
                    default:
                        MessageBoxA(0,"default error",0,0);
                        break;
                    }
                }
                if(pItem->mask&LVIF_IMAGE)
                {
                    pItem->iImage=iIcon;
                }
            } 
            else if(NM_RCLICK==pNMHDR->code){//�Ҽ����
                    NM_LISTVIEW*  pNMListView=(NM_LISTVIEW*)pNMHDR;
                    if(pNMListView->iItem<0) break;
                    WCHAR path[MAX_PATH],fileName[MAX_PATH];
                    int pathLen=GetFileName(pNMListView->iItem,path,fileName);
                    HRESULT hr;
                    IShellFolder *pDeskFolder,*pParentFolder;
                    hr=SHGetDesktopFolder(&pDeskFolder);
                    if(SUCCEEDED(hr)){
                            LPITEMIDLIST pidlParent;
                            if(0==pathLen){
                                hr=::SHGetSpecialFolderLocation(NULL,CSIDL_DRIVES,&pidlParent);//�ҵĵ���pidl
                                fileName[2]=L'\\';fileName[3]=0;
                            }else{
                                hr=pDeskFolder->ParseDisplayName(NULL,NULL,path,NULL,&pidlParent,NULL);
                            }              
                            if(SUCCEEDED(hr)){
                                hr=pDeskFolder->BindToObject(pidlParent,NULL,IID_IShellFolder,(PVOID*)&pParentFolder);
                                if(SUCCEEDED(hr)){
                                    LPITEMIDLIST pidl;
                                    hr=pParentFolder->ParseDisplayName(NULL,NULL,fileName,NULL,&pidl,NULL);
                                    if(SUCCEEDED(hr)){
                                        ShowContextMenu(pNMHDR->hwndFrom,pParentFolder,pidl);
                                        ::CoTaskMemFree(pidl);
                                    }
                                    SafeRelease(pParentFolder);
                                }
                                ::CoTaskMemFree(pidlParent);
                            }
                        SafeRelease(pDeskFolder);
                    }
            }
            else if(NM_DBLCLK==pNMHDR->code){
                NM_LISTVIEW*  pNMListView=(NM_LISTVIEW*)pNMHDR;
                int  iItem=pNMListView->iItem;
                if(iItem<0) break;
                WCHAR path[MAX_PATH],fileName[MAX_PATH];
                WCHAR extName[MAX_PATH];
                int pathLen=GetFileName(iItem,path,fileName,extName);
                if(pNMListView->iSubItem<3){
                    if(iItem<g_vDirOutPtr.size()){
                        if(0==pathLen){//��Ŀ¼
                            Help_OpenFile(fileName);     
                        }else{
                            if(pNMListView->iSubItem<2){
                                wcscat(path,fileName);
                            }
                            Help_OpenFile(path);  
                        } 
                    }else{
                            if(pNMListView->iSubItem<2){
                                wcscat(path,fileName);
                                if(*(PDWORDLONG)L"lnk"==*(PDWORDLONG)extName){
                                    Help_OpenShortCut(path);
                                }else{
                                    Help_OpenFile(path);  
                                }                              
                            }else{
                                Help_OpenFile(path);  
                            }        
                    }      
                }
            }
        }
        break;
#pragma region Ŀ¼����ѡ��
    case IDC_LIST_FILTER:
        {
            LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
            if(NM_CLICK==pNMHDR->code){
                if(g_listDirFilter.ClickList(pNMLV->iItem,pNMLV->iSubItem)){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                } 
            }else if(NM_RCLICK==pNMHDR->code){
                if(g_listDirFilter.ShowMenu()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                } 
            }
            else if(LVN_BEGINSCROLL==pNMHDR->code){
                g_listDirFilter.HideCombbox(); 
            }else if(LVN_ITEMCHANGED==pNMHDR->code){
                if(g_listDirFilter.OnItemChange(pNMLV->iItem)){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
            }
        }
        break;
#pragma endregion
    default:
        g_listDirFilter.HideCombbox();
//         if(HDN_TRACK==pNMHDR->code)
//         {
//              
//         }
        break;
    }
    return 0;
}

/**
 *	Function:
 *      �����˳�ʱд�����ݿ�
 *	Parameter(s):
 *
 *	Return:	
 *
 *	Commons:
 *      ��д�뷽ʽ���ڴ�ӳ���ļ���Ϊд�ڴ淽ʽ
 *      ����д�뵱ǰϵͳʱ��
 **/
void WriteToDatabase()
{
    //�ȴ�ÿ���߳��Զ�ֹͣ
    g_Lock.Lock();
    int nDirCount=g_vDirIndex.size();
    int nFileCount=g_vFileIndex.size();

    map<PDIRECTORY_RECORD,DWORD>  mapParentPtr2Offset;
    mapParentPtr2Offset[NULL]=0xFFFFFFFF;

    PDIRECTORY_RECORD pDir;
    PNORMALFILE_RECORD pFile;
    int i;
    PIndexElemType  pData, pDataEnd;
    PINDEX_BLOCK_NODE   *pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    {//���㸸��ƫ��ӳ��
        DWORD nBlockSize=CMemoryPool::GetBlockSize();//���ݴ�ֵ����ƫ��
        DWORD iBlock=0,iLastPos=0;//ƫ�ƿ飬����ƫ��

        DWORD dwRecordLen;

        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                if(pDir==pDir->ParentPtr)
                {
                    int jjj=0;
                    ++jjj;
                }
                dwRecordLen=pDir->GetLength();
                if(iLastPos+dwRecordLen>nBlockSize)
                {
                    iLastPos=0;
                    ++iBlock;
                }
                mapParentPtr2Offset[pDir]=((iBlock<<16)|(iLastPos&0xFFFF));
                iLastPos+=dwRecordLen;
            }
        }
    }

    CWriteMgr o_WriteMgr;

    //��ʼ���ļ���ƫ�Ʋ���
    PBYTE pByte;
    {
        //4b �ļ���С MAPƫ�� DIRƫ�� FILEƫ��   =16B        
        //4b tag_'����' tag_'����' tag_version dir_size file_size =20B
        //2b time_year time_month time_day time_hour  =8B
        BYTE Buffer[16+20+8+26*9];

        DWORD *pdwBuf=(DWORD*)(Buffer+16);
        *pdwBuf++='����';*pdwBuf++='����';*pdwBuf++=0x01000001;
        *pdwBuf++=g_vDirIndex.size();
        *pdwBuf++=g_vFileIndex.size();

        //д��ʱ��
        WORD* pWord=(WORD*)(Buffer+16+20);
        SYSTEMTIME curSysTm;
        GetSystemTime(&curSysTm);
        *pWord++=curSysTm.wYear;
        *pWord++=curSysTm.wMonth;
        *pWord++=curSysTm.wDay;
        *pWord++=curSysTm.wHour;


        pByte=Buffer+16+20+8;
        for(int i=0;i<26;++i)//���ÿ�����Ƿ���JournalID NextUsn
        {
            if(g_hVols[i]){
                *pByte++=1;
                *(DWORDLONG*)pByte=g_curJournalID[i];pByte+=8;
                *(USN*)pByte=g_curNextUSN[i];pByte+=8;
            }else *pByte++=0;
        }
        DWORD dwSize=pByte-Buffer;
        *(DWORD*)(Buffer+4)=dwSize;//ͷ���ֽڣ�ƫ�Ƶ�map����
        o_WriteMgr.Write(Buffer,dwSize);
    }


    {
        //PBYTE pTempBuf=g_MemoryMgr.GetMemory(nDirCount*sizeof(DWORD));
        //DWORD *pDwArr=(DWORD*)pTempBuf;

        //offsets����BasicInfo����
        DWORDLONG*  pData, *pDataEnd;
        PDirFrnIndexBlockNode   *pIndex;
        PDirFrnIndexBlockNode   pNode;
        DWORD *pBasicInfo;
        PDIRECTORY_RECORD* ppDir;
        cBlock=g_DirMap.GetBlockCount();
        pIndex=g_DirMap.GetBlockIndex();
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pBasicInfo=(DWORD*)pData;
                ppDir=(PDIRECTORY_RECORD*)(pBasicInfo+1);
                if(*ppDir){ //�����0��ʾ�Ѿ�ɾ��
                    o_WriteMgr.Write(&mapParentPtr2Offset[*ppDir],4);
                }
            }
        }
        o_WriteMgr.ReWriteFirstBlock(2,o_WriteMgr.GetTotalWirte());  
    }

    BYTE TempBuffer[1024];//�Ƚ�����д����ʱBUFFER

    PBYTE   pLastCodeName=NULL;//��һ���ļ�����ʼ��ַ
    DWORD   dwLastCodeNameLen=0;//��һ���ļ�����
    PBYTE   pCodeName;
    DWORD   dwCodeNameLen;
    DWORD   dwComLen;//������
    {//һ��д��Ŀ¼����(��ѹ��)
        //�������и�PtrΪ��Offset 
        //����ƫ��Ϊ0xFFFFFFFF,�����丸ָ��ΪNULL
        
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)//ֱ�Ӳ�����������
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);//��ָ��
                pDir->ParentOffset=mapParentPtr2Offset[pDir->ParentPtr];
                dwCodeNameLen=pDir->GetCodeName(pCodeName);
                for(dwComLen=0;
                    dwComLen<dwLastCodeNameLen 
                    && dwComLen<dwCodeNameLen 
                    && pLastCodeName[dwComLen]==pCodeName[dwComLen];
                ++dwComLen
                    );
                dwLastCodeNameLen=dwCodeNameLen;
                pLastCodeName=pCodeName;
                o_WriteMgr.Write(TempBuffer,pDir->WriteCompressData(dwComLen,TempBuffer));                    
            }
        }
        o_WriteMgr.ReWriteFirstBlock(3,o_WriteMgr.GetTotalWirte());  
    }

    pLastCodeName=NULL;//��һ���ļ�����ʼ��ַ
    dwLastCodeNameLen=0;//��һ���ļ�����
    {//һ��д���ļ�����(��ѹ��)
        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData){
                pFile=PNORMALFILE_RECORD(*pData);
                pFile->ParentOffset=mapParentPtr2Offset[pFile->ParentPtr];  
                //д�뵽�ļ�
                dwCodeNameLen=pFile->GetCodeName(pCodeName);
                for(dwComLen=0;
                    dwComLen<dwLastCodeNameLen 
                    && dwComLen<dwCodeNameLen 
                    && pLastCodeName[dwComLen]==pCodeName[dwComLen];
                ++dwComLen
                    );
                dwLastCodeNameLen=dwCodeNameLen;
                pLastCodeName=pCodeName;
                o_WriteMgr.Write(TempBuffer,pFile->WriteCompressData(dwComLen,TempBuffer));
            }
        }  
        o_WriteMgr.ReWriteFirstBlock(0,o_WriteMgr.GetTotalWirte());  
    }
    if(!o_WriteMgr.WriteOver(CURDIR L"ntfsqs.db"))
    {
        MessageBox(0,0,0,0);
    }
    g_ExtMgr.WriteToFile(CURDIR L"ext.db");//д����չ��

    //�ɵ�all�߳�
    for(int i=0;i<26;++i)
    {
        if(g_hThread[i]) TerminateThread(g_hThread[i],0);
    }
    g_Lock.UnLock();
}

/**
 *	Function:
 *      �򿪸�NTFS������ȡ�����Ϣ
 *	Parameter(s):
 *      
 *	Return:	
 *      
 *	Commons:
 **/
void OpenNtfsVolume()
{
    WCHAR tDrivers[26*4+1];
    GetLogicalDriveStringsW(26*4+1,tDrivers);
    WCHAR fileSysBuf[8];
    DWORD dwDri; //��� 0~25

    WCHAR szRootName[40];
    WCHAR szVolumeName[32];
    int iFilterRoot=0;
#ifdef TEST
    WCHAR *p=TEST_ROOT_DIR;
#else
    for(WCHAR *p=tDrivers;*p!='\0';p+=4)
#endif
    {       
        if(*p>=L'a') *p-=32;//���д
        dwDri=*p-L'A';
        if(DRIVE_FIXED==GetDriveTypeW(p))
        {
            DWORD dwMaxComLen,dwFileSysFlag;
            GetVolumeInformationW(p,szVolumeName,32,NULL,&dwMaxComLen,&dwFileSysFlag,fileSysBuf,8);
            if(fileSysBuf[0]==L'N' && fileSysBuf[1]==L'T' && fileSysBuf[2]==L'F' && fileSysBuf[3]==L'S')
            {
                swprintf(szRootName,L"%s (%c:)",szVolumeName,*p);
                g_listDirFilter.InsertItem(iFilterRoot,szRootName,TRUE);
                g_listDirFilter.SetItemText(iFilterRoot,1,L"��");
                ++iFilterRoot;

                WCHAR szVolumePath[8];
                swprintf(szVolumePath,L"\\\\.\\%c:",*p);
                HANDLE hVolume=CreateFileW(szVolumePath,
                    GENERIC_READ | GENERIC_WRITE, 
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
                if(INVALID_HANDLE_VALUE==hVolume){
                    DebugStringA("������%cʧ�� ����%d",*p,GetLastError());
#ifdef TEST
                    return;
#else
                    continue;
#endif
                }              
                g_hVols[dwDri]=hVolume;//������

                
                DWORD dwWritten;
                
//                 BOOT_BLOCK bb;
//                 ReadFile(hVolume,&bb,512,&dwWritten,NULL);
//                 assert(dwWritten==512);
//                 g_BytesPerCluster[dwDri]=bb.SectorsPerCluster*bb.BytesPerSector;
//                 g_FileRecSize[dwDri]=sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER)-1+bb.ClustersPerFileRecord;

                NTFS_VOLUME_DATA_BUFFER ntfsVolData;              
                DeviceIoControl(hVolume,
                    FSCTL_GET_NTFS_VOLUME_DATA, 
                    NULL, 0, 
                    &ntfsVolData, sizeof(ntfsVolData), 
                    &dwWritten, NULL);
                g_BytesPerCluster[dwDri]=ntfsVolData.BytesPerCluster;
                g_FileRecSize[dwDri]=sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER)-1+ntfsVolData.BytesPerFileRecordSegment;
                g_pOutBuffer[dwDri]=g_MemoryMgr.GetMemory(g_FileRecSize[dwDri]);

                USN_JOURNAL_DATA ujd;
                if(!QueryUsnJournal(hVolume,&ujd)){//��ѯʧ��
                    //DebugStringA("��ѯʧ��");
                    switch(GetLastError())
                    {
                    case ERROR_JOURNAL_NOT_ACTIVE:
                        {
                            CreateUsnJournal(hVolume,0x800000, 0x100000); 
                            QueryUsnJournal(hVolume,&ujd);
                        }
                        break;
                    case ERROR_JOURNAL_DELETE_IN_PROGRESS:
                        {
                            DWORD cb;
                            DELETE_USN_JOURNAL_DATA del_ujd;
                            del_ujd.UsnJournalID = ujd.UsnJournalID;
                            del_ujd.DeleteFlags = USN_DELETE_FLAG_NOTIFY;
                            if(!DeviceIoControl(hVolume, FSCTL_DELETE_USN_JOURNAL, 
                                &del_ujd, sizeof(DELETE_USN_JOURNAL_DATA), 
                                NULL, 0, &cb, NULL
                                )) {
                            }
                            CreateUsnJournal(hVolume,0x2000000,0x400000); 
                            QueryUsnJournal(hVolume,&ujd);
                        }
                        break;
                    default:
                        break;
                    }
                }
                g_curJournalID[dwDri]=ujd.UsnJournalID;
                g_curFirstUSN[dwDri]=ujd.FirstUsn;
                g_curNextUSN[dwDri]=ujd.NextUsn; 
            }
        }
    }
}

/**
 *	Function:
 *      �������ݿ�,�����������ݽṹ���������غ͸���
 *	Parameter(s):
 *      
 *	Return:	
 *      ���ɹ����أ�����TRUE;�����ʾ��Ҫ����ɨ�����ݿ�
 *	Commons:
 **/
BOOL LoadDatabase(HWND hMainWnd)
{
    Helper_SetCurrentState(-1,"���ڼ������ݿ�...");
    BOOL bRet=TRUE;
    HANDLE  hDB=CreateFileW(CURDIR L"ntfsqs.db"
        ,GENERIC_READ
        ,FILE_SHARE_READ
        ,NULL
        ,OPEN_EXISTING
        ,FILE_ATTRIBUTE_NORMAL
        ,NULL);
    if(INVALID_HANDLE_VALUE==hDB){
        bRet=FALSE;
        goto OPEN_FILE_CHECK;
    }

    DWORD dwFileSize=GetFileSize(hDB,NULL);
    if(INVALID_FILE_SIZE==dwFileSize ||dwFileSize<(16+20+8+26)){
        DebugStringA("��ȡ�ļ���Сʧ��:%d",GetLastError());
        bRet=FALSE;
        goto FILE_SIZE_CHECK;
    }

    PBYTE DB_Buffer=g_MemoryMgr.GetMemory(dwFileSize);

    DWORD dwRead;
    BOOL bReadOK=ReadFile(hDB,DB_Buffer,dwFileSize,&dwRead,NULL);

    if(!bReadOK || dwRead!=dwFileSize ){
        DebugStringA("���ļ�ʧ��:%d",GetLastError());
        bRet=FALSE;
        goto READ_FILE_CHECK;
    }

    //4b �ļ���С MAPƫ�� DIRƫ�� FILEƫ��   =16B
    //4b tag_'����' tag_'����' tag_version dir_size file_size =20B
    //2b time_year time_month time_day time_hour  =8B

    WORD wYear,wMonth,wDay,wHour;

    DWORD *pTag=(DWORD *)DB_Buffer;
    DWORD nDirCount=pTag[7];
    DWORD nFileCount=pTag[8];
    if(0==nDirCount||dwFileSize!=*pTag||pTag[1]<(16+20+8+26) || pTag[2]-pTag[1]!=(nDirCount<<2) || pTag[4]!='����' || pTag[5]!='����' || pTag[6]!=0x01000001){
        bRet=FALSE;
        goto FILE_TAG_CHECK;
    }

    PBYTE pByte=PBYTE(pTag+9);
    wYear=*(WORD*)pByte;pByte+=2;
    wMonth=*(WORD*)pByte;pByte+=2;
    wDay=*(WORD*)pByte;pByte+=2;
    wHour=*(WORD*)pByte;pByte+=2;

    assert(bRet);
    //���Journal ID
    USN usnLast[26]={0};
    {     
        DWORD dwDri=0;
        for(dwDri=0;dwDri<26;++dwDri)
        {
            if(*pByte++){
                if(*(DWORDLONG*)pByte!=g_curJournalID[dwDri]) {//Journal ID�Ѿ��ı�
                    bRet=FALSE;
                    break;
                }
                pByte+=8;
                usnLast[dwDri]=*(USN*)pByte;//�޸�Ҫ����USN���
                assert(usnLast[dwDri]<=g_curNextUSN[dwDri]);
                if(usnLast[dwDri]<g_curFirstUSN[dwDri]){
                    bRet=FALSE;
                    break;
                }
                pByte+=8;
            }else{
                if(g_curJournalID[dwDri]){
                    bRet=FALSE;
                    break;
                }
            }
        }
    }
    if(!bRet)
    {
        char buf[1024];
        sprintf(buf,
            "��ǰ���ݿ�洢ʱ�䣺 %d��%d��%d��%dʱ���Ѿ����ڣ�\r\n"
            "���Ƿ���Ҫ���£�\r\n\r\n"
            "���ѣ����ѡ���ǣ���Ҫ�����ĵȴ�...^_^\r\n"
            "      ���ѡ��񣬱����������������ļ�ϵͳ�ĸı䣡��ʱ������Ȼ\r\n"
            "      ������������ǰ������ļ���"
            ,wYear,wMonth,wDay,wHour
            );
        if(IDYES==::MessageBoxA(hMainWnd,buf,"�����������",MB_YESNO|MB_ICONINFORMATION))
        {
            goto JOURNAL_ID_CHECK;
        }
        else
        {
            bRet=TRUE;
            g_bJournalIdFailure=TRUE;
        }
    }

    PDIRECTORY_RECORD pDir;
    PNORMALFILE_RECORD pFile;
    DWORD dwMemRecord;
    DWORD i;

    //����Ŀ¼��������
    //��ԭĿ¼���ݣ�д��g_MemDir
    PBYTE pDirBuf=DB_Buffer+pTag[2],pDirEnd=DB_Buffer+pTag[3],pNextBeg;
    PBYTE pLastName=NULL;
    //�ṹΪ
    //0x0 4B BasicInfo
    //0x4 4B Offset
    //0x8 3B ��10bitΪdwCommLen �ε�10λΪdwNameLenLeft
    DWORD dwNameLen,dwCommLen,dwNameLenLeft;
    PBYTE pName;
    DWORD dwTemp;
    for(;pDirBuf<pDirEnd;)
    {
        dwTemp=*(DWORD*)(pDirBuf+8)&0xFFFFFF;
        dwCommLen=dwTemp&0x3FF;
        dwTemp>>=10;
        dwNameLenLeft=dwTemp&0x3FF;  
        dwTemp>>=10;
        dwNameLen=dwCommLen+dwNameLenLeft;
        dwMemRecord=GetDirectoryRecordLength(dwNameLen);
        pDir=(PDIRECTORY_RECORD)g_MemDir.PushBack(dwMemRecord);
        g_vDirIndex.push_back((IndexElemType)pDir);//����Ŀ¼����
        pDir->BasicInfo=*(DWORD*)(pDirBuf);pDirBuf+=4;
        pDir->ParentOffset=*(DWORD*)(pDirBuf);pDirBuf+=7; //ֱ������3�ֽڲ���      
        if(dwTemp){
            pDir->NameLength=dwNameLen|0x8000;
        }else{
            pDir->NameLength=dwNameLen;
        }
        pName=pDir->Name;
        for(i=0;i<dwCommLen;++i){//������������
            *pName++=pLastName[i];
        }
        pNextBeg=pDirBuf+dwNameLenLeft;
        for(;pDirBuf<pNextBeg;)
        {
            *pName++=*pDirBuf++;
        }
        pLastName=pDir->Name;//����pLastName
    }

    
    //����g_DirMap ��Ŀ¼Offset->Ptr
    //ע�����Offset->Ptr�������Ŀ¼����
    DWORD *pOffset=(DWORD*)(DB_Buffer+pTag[1]),*pOffsetEnd=pOffset+nDirCount;
    for(;pOffset<pOffsetEnd;++pOffset)
    {
        pDir=(PDIRECTORY_RECORD)g_MemDir.DB_FromOffsetToPtr(*pOffset);
        pDir->ParentPtr=(PDIRECTORY_RECORD)g_MemDir.DB_FromOffsetToPtr(pDir->ParentOffset);
        g_DirMap.push_back(pDir->BasicInfo,(IndexElemType)pDir);
    }

    //�ļ��ṹΪ
    //0x0 4B BasicInfo
    //0x4 4B ParentOffset
    //0x
    PBYTE pFileBuf=pDirEnd,pFileEnd=DB_Buffer+dwFileSize;
    pLastName=NULL;
    DWORD dwIconLen,dwExtLength,dwNameLenLength;
    DWORD dwBasicInfo;
    PBYTE pTempLastName;
    for(;pFileBuf<pFileEnd;)
    {
        dwBasicInfo=*(DWORD*)(pFileBuf);pFileBuf+=4;
        if(dwBasicInfo&NormalFileRecord::BITMASK_TWOBYTE_NAMELEN){
            dwNameLenLength=2;
        }else{
            dwNameLenLength=1;
        }
        if(dwBasicInfo&NormalFileRecord::BITMASK_ICON){
            dwIconLen=4;
        }else{
            dwIconLen=0;
        }
        dwTemp=*(DWORD*)(pFileBuf+4)&0xFFFFFF;
        dwCommLen=dwTemp&0x3FF;
        dwTemp>>=10;
        dwNameLenLeft=dwTemp&0x3FF;//��Ŀ¼��ͬ ��0x7F��־
        dwExtLength=(dwTemp>>10);//��չ����
        dwNameLen=dwCommLen+dwNameLenLeft;
        dwMemRecord=GetNormalFileRecordLength(dwNameLen,dwNameLenLength,dwExtLength,dwIconLen);
        pFile=(PNORMALFILE_RECORD)g_MemFile.PushBack(dwMemRecord);
        g_vFileIndex.push_back((IndexElemType)pFile);//����Ŀ¼����
        pFile->BasicInfo=dwBasicInfo;

        pFile->ParentPtr=(PDIRECTORY_RECORD)g_MemDir.DB_FromOffsetToPtr(*(DWORD*)(pFileBuf));pFileBuf+=7; 
        pName=pFile->NameInformation;
        if(1==dwNameLenLength){
            *pName=dwNameLen;
            ++pName;
        }else{
            *(WORD*)pName=dwNameLen;
            pName+=2;
        } 
        pTempLastName=pName;
        for(i=0;i<dwCommLen;++i){//������������
            *pName++=pLastName[i];
        }
        pNextBeg=pFileBuf+dwNameLenLeft;
        for(;pFileBuf<pNextBeg;)
        {
            *pName++=*pFileBuf++;
        }
        pLastName=pTempLastName;//����pLastName

        //������չ��
        if(dwExtLength>0)
        {
            if(1==dwExtLength){
                *pName=*pFileBuf++;               
            }else if(2==dwExtLength){
                *(WORD*)pName=*(WORD*)pFileBuf;
                pFileBuf+=2;
            }else{
                *(DWORD*)pName=*(DWORD*)pFileBuf;
                pFileBuf+=4;
            }
            if(dwIconLen) *(DWORD*)(pName+dwExtLength)=0xFFFFFFFF;
        }
    }
    if(bRet && !g_bJournalIdFailure)//�����������ݿ�ɹ�ʱ���� NextUSNΪ�ļ��еĴ洢ֵ
    {
        for(int i=0;i<26;++i) g_curNextUSN[i]=usnLast[i];
    }//���ݿ����ʧ�ܣ���Ӧ�ı�g_curNextUSNֵ
JOURNAL_ID_CHECK:
FILE_TAG_CHECK:
READ_FILE_CHECK:
    g_MemoryMgr.FreeMemory(DB_Buffer);
FILE_SIZE_CHECK:
    CloseHandle(hDB);
OPEN_FILE_CHECK:
    return bRet;
}

DWORD WINAPI UpdateDatabase(PVOID pParam)
{
    HWND hMainWnd=(HWND)pParam;
    Helper_SetCurrentState(-1,"���ڸ������ݿ�...");
    const DWORD SEARCH_TITLE_REASON_FLAG=
        USN_REASON_FILE_CREATE              
        |USN_REASON_FILE_DELETE
        |USN_REASON_RENAME_OLD_NAME
        |USN_REASON_RENAME_NEW_NAME
        ;
    READ_USN_JOURNAL_DATA rujd;
    rujd.BytesToWaitFor=0;//û�����ݾͽ���
    rujd.ReasonMask=SEARCH_TITLE_REASON_FLAG;
    rujd.ReturnOnlyOnClose=0;
    rujd.Timeout=0;

    DWORD dwBytes;
    DWORD dwRetBytes;
    BYTE Buffer[USN_PAGE_SIZE];
    PUSN_RECORD pRecord; 
    DWORD dwDri;
    for(dwDri=0;dwDri<26;++dwDri)
    {
        if(g_hVols[dwDri])
        {
            DebugTrace("����%c��\n",dwDri+'A');
            rujd.StartUsn=g_curNextUSN[dwDri];
            rujd.UsnJournalID=g_curJournalID[dwDri];
            for(;;)
            {
                if(!DeviceIoControl(g_hVols[dwDri], 
                    FSCTL_READ_USN_JOURNAL, 
                    &rujd,
                    sizeof(rujd),
                    &Buffer,
                    USN_PAGE_SIZE,
                    &dwBytes,
                    NULL) )
                {
                    DWORD dwError=GetLastError();
                    DebugStringA("%c �߳� FSCTL_READ_USN_JOURNAL ����%d",'A'+dwDri,dwError);
                    switch(dwError)
                    {
                    case ERROR_INVALID_FUNCTION:
                        DebugStringA("ERROR_INVALID_FUNCTION");               
                        break;
                    case ERROR_INVALID_PARAMETER:
                        DebugStringA("ERROR_INVALID_PARAMETER");
                        break;
                    case ERROR_JOURNAL_DELETE_IN_PROGRESS:
                        DebugStringA("ERROR_JOURNAL_DELETE_IN_PROGRESS");
                        break;
                    case ERROR_JOURNAL_NOT_ACTIVE:
                        DebugStringA("ERROR_JOURNAL_NOT_ACTIVE");
                        break;
                    case ERROR_JOURNAL_ENTRY_DELETED:
                        DebugStringA("ERROR_JOURNAL_ENTRY_DELETED");
                        break;
                    default:
                        DebugStringA("ERROR_UNKNOWN");
                        break;
                    }
                    DebugBreak();
                }
                if(dwBytes<=sizeof(USN)) break;//����!

                dwRetBytes = dwBytes - sizeof(USN);//������1��USN����USN����һ�۲�ѯ���
                //������û����¼

                pRecord = PUSN_RECORD((PBYTE)Buffer+sizeof(USN));  
                while(dwRetBytes > 0 )//������1��USN�󣬻��ж����ֽڣ���������1����¼
                {
                    ChangeDataBase(
                        dwDri
                        ,pRecord->FileAttributes
                        ,pRecord->Reason
                        ,pRecord->FileReferenceNumber
                        ,pRecord->ParentFileReferenceNumber
                        ,PWCHAR(pRecord->FileName)
                        ,pRecord->FileNameLength>>1
                        );  
                    dwRetBytes -= pRecord->RecordLength;
                    //����һ����¼
                    pRecord = (PUSN_RECORD)(((PBYTE)pRecord) + pRecord->RecordLength); 
                }
                //������ʼUSN
                rujd.StartUsn = g_curNextUSN[dwDri]=*(USN*)Buffer; 
            }   
            DebugTrace("����%c�� ����\n",dwDri+'A');
        }
    }
    return 0;
}


//��ʼ�����ؼ�
void InitCtrl(HWND hMainWnd)
{
    g_hStateWnd=CreateStatusWindowW(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        L"׼��",hMainWnd,12345);
    g_hListCtrl=GetDlgItem(hMainWnd,IDC_LIST1);
    g_hEdit=GetDlgItem(hMainWnd,IDC_EDIT1);
    g_hExtEdit=GetDlgItem(hMainWnd,IDC_EDIT2);
    UpdateLayout(hMainWnd);
    SendMessage(g_hEdit,EM_LIMITTEXT,1024,0);//�ļ����������û����������ַ�����
    SendMessage(g_hExtEdit,EM_LIMITTEXT,512,0);//��չ���������û����������ַ�����

    DWORD dwStyle=GetWindowLong(g_hListCtrl,GWL_STYLE);
    SetWindowLong(g_hListCtrl,GWL_STYLE
        ,dwStyle
        |WS_CLIPSIBLINGS
        |WS_CLIPCHILDREN
        |LVS_ICON
        |LVS_SHOWSELALWAYS
        /*                |LVS_SORTASCENDING|LVS_SORTDESCENDING*/
        |LVS_SHAREIMAGELISTS //imagelist auto ���� ��ctrl����
        /*                |LVS_EDITLABELS*/ //�ɱ༭
        );
    dwStyle=ListView_GetExtendedListViewStyle(g_hListCtrl);
    dwStyle |=WS_EX_LEFT|
        WS_EX_LTRREADING|
        WS_EX_RIGHTSCROLLBAR|
        //LVS_EX_HEADERDRAGDROP|  //�϶���ͷ����������
        LVS_EX_FULLROWSELECT |
        LVS_EX_INFOTIP|
        LVS_EX_UNDERLINEHOT
        ; 
    ListView_SetExtendedListViewStyle(g_hListCtrl,dwStyle);

    SHFILEINFO shfi;
    ZeroMemory(&shfi,sizeof(SHFILEINFO));
    TCHAR dir[MAX_PATH];
    GetSystemDirectory(dir,MAX_PATH);
    dir[3]=0;
    HIMAGELIST hImageList=(HIMAGELIST)SHGetFileInfo(dir,0,&shfi,sizeof(SHFILEINFO),SHGFI_ICON |SHGFI_SYSICONINDEX|/*SHGFI_LARGEICON*/SHGFI_SMALLICON);
    ListView_SetImageList(g_hListCtrl,hImageList,LVSIL_SMALL);
    g_iRootIcon=Helper_GetDirectoryIconIndex(L"c:",0);
    g_iDirIcon=Helper_GetDirectoryIconIndex(L"c:\\",3);

    int iCol=0;
    LV_COLUMN lvCol={LVCF_TEXT|LVCF_WIDTH|LVCF_FMT,LVCFMT_LEFT,0};

    lvCol.pszText=_T("�ļ���");
    lvCol.cx=200;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("��չ��");
    lvCol.cx=60;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("����Ŀ¼");
    lvCol.cx=500;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.fmt=LVCFMT_RIGHT;
    lvCol.pszText=_T("�ļ���С");
    lvCol.cx=80;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.fmt=LVCFMT_CENTER;
    lvCol.pszText=_T("����ʱ��");
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("����޸�ʱ��");
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("�������ʱ��");
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.fmt=LVCFMT_LEFT;
    lvCol.pszText=_T("����");//ֻ�� ���� ϵͳ
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    //��ʼ�����˿ؼ�
    g_listDirFilter.FromHandle(::GetDlgItem(hMainWnd,IDC_LIST_FILTER));
    HWND hBtnAdd=::GetDlgItem(hMainWnd,IDC_BTN_ADDDIR)
        ,hBtnDel=::GetDlgItem(hMainWnd,IDC_BTN_DELDIR)
        ,hBtnDelAll=::GetDlgItem(hMainWnd,IDC_BTN_DELALLDIR)
        ,hBtnCheckAllDri=::GetDlgItem(hMainWnd,IDC_BTN_CHECKALLDRI);
    g_listDirFilter.CreateContext(hBtnAdd,hBtnDel,hBtnDelAll,hBtnCheckAllDri,hMainWnd);//����
    g_listDirFilter.SetImageList(hImageList,g_iRootIcon,g_iDirIcon);

    iCol=0;
    lvCol.fmt=LVCFMT_LEFT;
    lvCol.pszText=_T("����Ŀ¼");
    lvCol.cx=300;
    g_listDirFilter.InsertColumn(iCol++,&lvCol);

    lvCol.pszText=_T("����Ŀ¼��");
    lvCol.cx=100;
    g_listDirFilter.InsertColumn(iCol++,&lvCol);

    g_listDirFilter.Hide();


    //��ʼ�����˿ؼ�
    HWND hSizeCheck=GetDlgItem(hMainWnd,IDC_CHECK_SIZE)
        ,hSizeLeftComb=GetDlgItem(hMainWnd,IDC_COMBO_SIZELEFT)
        ,hSizeEdit=GetDlgItem(hMainWnd,IDC_EDIT_SIZE)
        ,hSizeRightComb=GetDlgItem(hMainWnd,IDC_COMBO_SIZERIGHT)
        ,hDateCheck=GetDlgItem(hMainWnd,IDC_CHECK_DATE)
        ,hDateTypeComb=GetDlgItem(hMainWnd,IDC_COMBO_DATATYPE)
        ,hPreRadio=GetDlgItem(hMainWnd,IDC_RADIO_PRE) 
        ,hPreTime=GetDlgItem(hMainWnd,IDC_TIME_PRE) 
        ,hPosRadio=GetDlgItem(hMainWnd,IDC_RADIO_POS) 
        ,hPosTime=GetDlgItem(hMainWnd,IDC_TIME_POS)  
        ,hMidRadio=GetDlgItem(hMainWnd,IDC_RADIO_MID) 
        ,hMidTime1=GetDlgItem(hMainWnd,IDC_TIME_MID1) 
        ,hMidTime2=GetDlgItem(hMainWnd,IDC_TIME_MID2) 
        ,hAttrCheck=GetDlgItem(hMainWnd,IDC_CHECK_ATTR)
        ,hAttrNormalCheck=GetDlgItem(hMainWnd,IDC_CHECK_ATTR_NORMAL)
        ,hAttrHiddenCheck=GetDlgItem(hMainWnd,IDC_CHECK_ATTR_HIDDEN)
        ,hAttrSysCheck=GetDlgItem(hMainWnd,IDC_CHECK_ATTR_SYSTEM)
        ,hBtnFilter=GetDlgItem(hMainWnd,IDC_BTN_FILTER)
        ,hParentWnd=hMainWnd;
    g_Filter.CreateContext(hSizeCheck,hSizeLeftComb,hSizeEdit,hSizeRightComb
        ,hDateCheck,hDateTypeComb,hPreRadio,hPreTime,hPosRadio,hPosTime,hMidRadio,hMidTime1,hMidTime2
        ,hAttrCheck,hAttrNormalCheck,hAttrHiddenCheck,hAttrSysCheck
        ,hBtnFilter,hParentWnd
        );
    g_Filter.Hide();
}


DWORD WINAPI InitDatabase(PVOID pParam)
{
    HWND hMainWnd=(HWND)pParam;
    Helper_SetCurrentState(-1,"��ʼ��...");
    OpenNtfsVolume();//�򿪸�NTFS��
    BOOL bLoad=LoadDatabase(hMainWnd);
    if(!bLoad || !g_ExtMgr.LoadFromFile(CURDIR L"ext.db"))
    {
        //�����߳���InitScanMftProc������
        g_ExtMgr.InitRealTimeCallExt();
        InitScanMftProc(hMainWnd);
    }
    else
    {
        if(!g_bJournalIdFailure) UpdateDatabase(hMainWnd);
    }
    g_bCanSearch=TRUE;
    KernelSearch();

    //���������߳�  
    if(!g_bJournalIdFailure)
    {
        for(int i=0;i<26;++i)
        {
            if(g_hVols[i]) {
                g_hThread[i]=CreateThread(NULL,0,MonitorProc,(PVOID)(i),0,0);
            }
        }
    }
    
    return 0;
}


INT_PTR CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_INITDIALOG:
        {      
            //��ʼ����ؼ�
            InitCtrl(hWnd);

            //��ʼ��no_case��
            Helper_InitNoCaseTable();

            //��ʼ�����������
            Help_InitCompare();

            //���̰߳��� �������ݿ� �����ز��ɹ� ��ɨ��MFT
            HANDLE hThread=CreateThread(NULL,0,InitDatabase,hWnd,0,0);
            CloseHandle(hThread); 
        }
        break;
    case WM_GETMINMAXINFO:
        {
            PMINMAXINFO pMMInfo=(PMINMAXINFO)lParam;
            pMMInfo->ptMinTrackSize.x=800;
            pMMInfo->ptMinTrackSize.y=600;    
        }
        break;
    case WM_SIZE:
        {
            UpdateLayout(hWnd);
        }
        break;
    case WM_COMMAND:
        return SetDlgMsgResult(hWnd,msg,
            HANDLE_WM_COMMAND(hWnd,wParam,lParam,Dlg_OnCommand)
            );
        break;
    case WM_NOTIFY:
        return SetDlgMsgResult(hWnd,msg,
            HANDLE_WM_NOTIFY(hWnd,wParam,lParam,Dlg_OnNotify)
            );
    case WM_CLOSE:
        {
            ShowWindow(hWnd,SW_HIDE);
            if(g_bCanSearch) //�������ݿⶼδ����
            {
                if(!g_bJournalIdFailure) WriteToDatabase();//������д�����ݿ���
            }
            for(DWORD dwDri=0;dwDri<26;++dwDri){
                if(g_hVols[dwDri]) {
                    CloseHandle(g_hVols[dwDri]);
                    g_MemoryMgr.FreeMemory(g_pOutBuffer[dwDri]);
                }
            }
            DestroyWindow(hWnd);
            return TRUE;
        }
        break;
    case WM_DESTROY:
        {
            PostQuitMessage(0);
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}







int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
    g_hMainWnd=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,(DLGPROC)DialogProc);
    RECT rcWnd;
    GetWindowRect(g_hMainWnd,&rcWnd);
    int X=(GetSystemMetrics(SM_CXSCREEN)-rcWnd.right+rcWnd.left)>>1,
        Y=(GetSystemMetrics(SM_CYSCREEN)-rcWnd.bottom+rcWnd.top)>>1;
    MoveWindow(g_hMainWnd,X,Y,rcWnd.right-rcWnd.left,rcWnd.bottom-rcWnd.top,FALSE);
    ShowWindow(g_hMainWnd,SW_SHOW);

    BOOL bRet;
    MSG msg;
    while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
    { 
        if (bRet == -1)
        {
            MessageBox(NULL,_T("GetMessage error with -1 returned��"),_T("error"),MB_ICONHAND);
            break;
        }
        else if (!IsWindow(g_hMainWnd) || !IsDialogMessage(g_hMainWnd, &msg)) 
        {
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        }
    }
    return 0;
}


