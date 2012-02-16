// DesktopSearch.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
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

COutVector g_vDirOutPtr//目录的结果较少
        ,g_vFileOutPtr;//文件的结果较多

CLock g_Lock;//结束锁

USN         g_curFirstUSN[26]={0};
USN         g_curNextUSN[26]={0};
DWORDLONG   g_curJournalID[26]={0};
HANDLE      g_hVols[26]={0};//保存A~Z的对应的卷句柄 -'A'获取
HANDLE      g_hThread[26]={0};//每个盘一个监视线程
DWORD       g_BytesPerCluster[26];//每簇字节数
DWORD       g_FileRecSize[26];//MFT文件记录大小
PBYTE       g_pOutBuffer[26]={0};//每个盘的文件块记录地址 初始时分配 结束时消


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
BOOL g_bJustFilter=FALSE;//刚才按了过滤的

int g_iRootIcon;
int g_iDirIcon;

//搜索选项
BOOL g_bOptChanged=FALSE;//搜索选项改变
BOOL g_bCase=FALSE;
BOOL g_bDirSearch=TRUE;
BOOL g_bFileSearch=TRUE;
BOOL g_bDirSetExpand=FALSE; //目录设置展开
CDirFilterList g_listDirFilter;
BOOL g_bDirSetting=FALSE;//当有消息触发时改变为TRUE 调用KernelSearch


//过滤选项
CFilterCtrl g_Filter;
BOOL g_bFilterExpand=FALSE; //目录设置展开
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
    int static_widht=rcText.right-rcText.left;//文本长
    int editWidth=rectWnd.right-rectWnd.left-2*static_widht;
    int fnameEditWidth=editWidth*0.7;//文件名编辑框长度   

    int editTop;//此值需要根据过滤选项 动态调整
    if(g_bDirSetExpand){
        g_Filter.Hide();
        editTop=170;
        g_listDirFilter.Show(rectWnd.right-rectWnd.left,editTop);//显示所在目录过滤       
    }else{
        g_listDirFilter.Hide();
        if(g_bFilterExpand){//显示过滤扩展
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
        //文件名:
        left=3;
        width=static_widht;
        ::SetWindowPos(hStaticText2,NULL,left,
            editTop+3,width,editHeight-4,SWP_NOZORDER | SWP_NOACTIVATE); 

        //文件名编辑框
        left+=width;
        width=fnameEditWidth;
        ::SetWindowPos(g_hEdit, NULL,left,editTop,
            width, editHeight,SWP_NOZORDER | SWP_NOACTIVATE);  

        //扩展名：
        left+=width;
        width=static_widht;
        ::SetWindowPos(hStaticText,NULL,left,
            editTop+3,width,editHeight-4,SWP_NOZORDER | SWP_NOACTIVATE);  

        //扩展名编辑框
        left+=width;
        width=rectWnd.right-left;
        ::SetWindowPos(g_hExtEdit, NULL,left,editTop,
            width,editHeight,SWP_NOZORDER | SWP_NOACTIVATE); 

        //计算状态栏
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

    BYTE szFileNameCode[768];//最多需要255*3个字节<768

    //将目录增加到目录库中  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,nameLen);
    g_vDirIndex.Lock();/****************锁锁**********************/
    NewParentPtr=(PDIRECTORY_RECORD)g_MemDir.Alloc(GetDirectoryRecordLength(codeNameLen));

    DebugTrace("in map-insert\n");
    g_DirMap.insert(GetBasicInfo(dwDri,frn),(IndexElemType)NewParentPtr);//插入本目录 进目录FRN库
    DebugTrace("out map-insert\n");

    DebugTrace("in map-find\n");
    PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(GetBasicInfo(dwDri,parent_frn)); ++ppDir;
    DebugTrace("out map-find\n");

    NewParentPtr->SetData(dwDri,frn,*ppDir,szFileNameCode,codeNameLen,codeNameLen>nameLen);

    //将记录插入目录索引区，目录索引区按文件名有序的
    //有序规则较复杂
    //锁保证外界不能访问目录索引区
    g_vDirIndex.insert((IndexElemType)NewParentPtr,TRUE);

    //将所有Index中父是OldParentPtr的元素 其父改为NewParentPtr
    //注意，此部一定要在该目录数据写好之后在做，
    //因为此步会改变数据区，而列表控件可能此时正在访问数据区
    //如果此步先做，父目录链会中断
    {
        PDIRECTORY_RECORD pDir;
        PNORMALFILE_RECORD pFile;

        int i;
        PIndexElemType  pData, pDataEnd;
        PINDEX_BLOCK_NODE   *pIndex;
        PINDEX_BLOCK_NODE   pNode;
        int cBlock;
        //*****遍历目录索引
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
        //*****遍历文件索引
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
    g_vDirIndex.UnLock();/****************解锁**********************/
    ListView_RedrawItems(g_hListCtrl,0,100);//重绘结果

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
    BYTE szFileNameCode[768];//最多需要255*3个字节<768

    //将目录增加到目录库中  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,fileName,nameLen);
    pDir=(PDIRECTORY_RECORD)g_MemoryMgr.GetMemory(GetDirectoryRecordLength(codeNameLen));
    pDir->SimpleSetData(dwDri,frn,szFileNameCode,codeNameLen,codeNameLen>nameLen);
    DWORD dwBasicInfo=GetBasicInfo(dwDri,frn);
    g_vDirIndex.Lock();/****************锁锁**********************/
    {
        OldParentPtr=(PDIRECTORY_RECORD)g_DirMap.erase(dwBasicInfo);
        g_vDirIndex.erase((IndexElemType)pDir,TRUE);
        g_MemDir.Free(OldParentPtr,OldParentPtr->GetLength());//在此释放内存(未实现)
    }
    g_vDirIndex.UnLock();/****************解锁锁**********************/
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
    BYTE szFileNameCode[768];//最多需要255*3个字节<768

    //将目录增加到目录库中  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,nameLen);
    g_vDirIndex.Lock();/****************锁锁**********************/

#ifdef OUT_PUT_MONITOR
    DebugTrace("in map-find\n");
#endif
    PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(GetBasicInfo(dwDri,parent_frn)); 
#ifdef OUT_PUT_MONITOR
    DebugTrace("out map-find\n");
#endif

    if(ppDir)//若父目录存在才插入
    {
        ++ppDir;

        pDir=(PDIRECTORY_RECORD)(
            g_MemDir.Alloc(GetDirectoryRecordLength(codeNameLen))
            );   

#ifdef OUT_PUT_MONITOR
        DebugTrace("in map-insert\n");
#endif
        g_DirMap.insert(GetBasicInfo(dwDri,frn),(IndexElemType)pDir);//插入本目录 进目录FRN库
#ifdef OUT_PUT_MONITOR
        DebugTrace("out map-insert\n");
#endif

        pDir->SetData(dwDri,frn,*ppDir,szFileNameCode,codeNameLen,codeNameLen>nameLen);

        //将记录插入目录索引区，目录索引区按文件名有序的
        //有序规则较复杂
        //锁保证外界不能访问目录索引区
        g_vDirIndex.insert((IndexElemType)pDir,TRUE);
    }

    g_vDirIndex.UnLock();/****************解锁**********************/

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
    BYTE szFileNameCode[768];//最多需要255*3个字节<768

    //将目录增加到目录库中  
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,fileName,nameLen);
    pDir=(PDIRECTORY_RECORD)g_MemoryMgr.GetMemory(GetDirectoryRecordLength(codeNameLen));
    pDir->SimpleSetData(dwDri,frn,szFileNameCode,codeNameLen,codeNameLen>nameLen);
    DWORD dwBasicInfo=GetBasicInfo(dwDri,frn);
    g_vDirIndex.Lock();/****************锁锁**********************/
    {
        g_DirMap.erase(dwBasicInfo);
        g_vDirIndex.erase((IndexElemType)pDir,TRUE);
    }
    g_vDirIndex.UnLock();/****************解锁锁**********************/
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
    BYTE szFileNameCode[768];//最多需要255*3个字节<768
    int idExt=0;//扩展名ID
    DWORD dwExtIdLen;//扩展名所占用字节数 
    DWORD dwIconLen=0;
    int i;
    for(i=nameLen-1;i>=0&&pFileName[i]!=L' '&&pFileName[i]!=L'.';--i);
    if(i<=0 ||pFileName[i]==L' '){
        dwExtIdLen=0;//无扩展名
    }else{//
        idExt=g_ExtMgr.insert(pFileName+i+1,nameLen-1-i);
        if(idExt<CExtArray::s_dwOmitExt) dwIconLen=4;
        dwExtIdLen=GetExtIdLength(idExt);
        nameLen=i;
    }
    int codeNameLen=Helper_Ucs2ToCode(szFileNameCode,pFileName,nameLen);
    DWORD dwNameLenLength=GetNameLenLength(codeNameLen);
    DWORD dwMemRecord=GetNormalFileRecordLength(codeNameLen,dwNameLenLength,dwExtIdLen,dwIconLen);

    g_vFileIndex.Lock();/****************锁锁**********************/
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
            //文件名 扩展名有序插入
            g_vFileIndex.insert((IndexElemType)pFile,FALSE);
        }
    }
    g_vFileIndex.UnLock();/****************解锁锁**********************/
#ifdef OUT_PUT_MONITOR
    DebugTrace("OUT AddFileBase!!!!!!!\n");
#endif   
}

VOID DeleteFileBase(DWORD dwDri,DWORDLONG frn,PWCHAR pFileName,int nameLen)
{
    PNORMALFILE_RECORD pFile;
    BYTE szFileNameCode[768];//最多需要255*3个字节<768
    int idExt=0;//扩展名ID
    DWORD dwExtIdLen;//扩展名所占用字节数 
    DWORD dwIconLen=0;
    int i;
    for(i=nameLen-1;i>=0&&pFileName[i]!=L' '&&pFileName[i]!=L'.';--i);
    if(i<=0 ||pFileName[i]==L' '){
        dwExtIdLen=0;//无扩展名
    }else{//
        idExt=g_ExtMgr.find(pFileName+i+1,nameLen-1-i);
        if(idExt<CExtArray::s_dwOmitExt) dwIconLen=4;
        assert(idExt>=0 && "扩展名必须要存在");
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

    g_vFileIndex.Lock();/****************锁锁**********************/
    {
        g_vFileIndex.erase((IndexElemType)pFile,FALSE);
    }
    g_vFileIndex.UnLock();/****************解锁锁**********************/
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
    DebugTrace("原因：%s\n",buf2);
#endif
    if((USN_REASON_FILE_CREATE&dwReason)&&(USN_REASON_CLOSE&dwReason)){//增
        if(bDir)AddDirectoryBase(dwDri,frn,parent_frn,wszFileName,dwNameLen);
        else AddFileBase(dwDri,frn,parent_frn,wszFileName,dwNameLen); 
        bFileNameChange=TRUE;
    }else if((USN_REASON_RENAME_NEW_NAME&dwReason) && (dwReason&USN_REASON_CLOSE)){//增
        if(bDir)//重命名目录 仅根目录变化
            RenameNewDirectory(dwDri,frn,parent_frn,wszFileName,dwNameLen);
        else AddFileBase(dwDri,frn,parent_frn,wszFileName,dwNameLen); 
        bFileNameChange=TRUE;      
    }
    else if(USN_REASON_RENAME_OLD_NAME&dwReason){//删
        if(USN_REASON_FILE_CREATE&dwReason){
            //old 里面含有创建
        }else{
            if(bDir) //重命名目录 仅根目录变化
                RenameOldDirectory(dwDri,frn,wszFileName,dwNameLen);
            else DeleteFileBase(dwDri,frn,wszFileName,dwNameLen);
            bFileNameChange=TRUE;
        }
    }else if((dwReason&USN_REASON_FILE_DELETE)&&(USN_REASON_CLOSE&dwReason)){//删
        if(USN_REASON_FILE_CREATE&dwReason){
            //new close 里面含创建，仅在创建中处理
        }else{
            if(bDir) DeleteDirectoryBase(dwDri,frn,wszFileName,dwNameLen);
            else DeleteFileBase(dwDri,frn,wszFileName,dwNameLen);
            bFileNameChange=TRUE;
        }
    }else {
#ifdef OUT_PUT_MONITOR
        DebugTrace("进来了其他原因\n");
#endif
        //可能是：
        //[RenameNewName|Close] 防止重复处理
        //[FileCreate|Close]    防止重复处理
    }


    if(bFileNameChange) {//当库文件名改变，且是当前输入串的结构时，向用户更新
        if(bDir && !g_bDirSearch) return;//改变的是目录 但当前不搜索目录
        if(!bDir && !g_bFileSearch) return;//改变的是文件 但当前不搜索文件

        //检查本文件是否对当前查询串有影响
        SearchStrOpt *pStrOpt;
        if(g_bCase){//大小写敏感
            pStrOpt=&g_StrOptCase;
        }else{//不敏感                         
            pStrOpt=&g_StrOptNoCase;
        }
#ifdef OUT_PUT_MONITOR
        DebugTrace("Check:\n");
#endif
        BOOL bEffect=FALSE;//本改变对显示结果有影响么?
        if(pStrOpt->HasQuestion()){//含有?，可能含有【普通，*，*?】
            bEffect=Helper_CheckQuestionSearch(g_bCase,wszFileName,dwNameLen);
        }else{//不含【?】
            if(pStrOpt->HasStar()){//不含【?】，含有*，可能含有【普通，*?】
                bEffect=Helper_CheckStarSearch(g_bCase,wszFileName,dwNameLen);
            }else{//不含【?，*】*a
                if(pStrOpt->HasStarQuestion()){//不含【?，*】，含有?*，可能含有【普通】
                    bEffect=Helper_CheckStarQuestionSearch(g_bCase,wszFileName,dwNameLen);
                }else{//不含【?，*，?*】，含有普通
                    //assert(pStrOpt->pNormal && "一定含有普通串");//普通串已处理
                    if(pStrOpt->pNormal){//当全*时不含普通串
                        bEffect=Helper_CheckOnlyNormalSearch(g_bCase,wszFileName,dwNameLen);//暴力
                    }
                }
            }
        } 
        if(bEffect)//直接修改结果
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


//监视各个盘符
//任务：更改数据库(扩展名 目录 文件 索引)
DWORD WINAPI MonitorProc(PVOID pParam)
{
    const DWORD SEARCH_TITLE_REASON_FLAG=
        //支持文件名更改
        USN_REASON_FILE_CREATE              
        |USN_REASON_FILE_DELETE
        |USN_REASON_RENAME_OLD_NAME
        |USN_REASON_RENAME_NEW_NAME
        
        //支持文件属性更改 //无最近访问时间支持
//         |USN_REASON_DATA_EXTEND
//         |USN_REASON_DATA_OVERWRITE
//         |USN_REASON_DATA_TRUNCATION
//         |USN_REASON_BASIC_INFO_CHANGE   //文件属性改变
//         |USN_REASON_OBJECT_ID_CHANGE    //LAST ACCESS
        ;

    DWORD dwDri=(DWORD)pParam;
    HANDLE hVol=g_hVols[dwDri];   
    READ_USN_JOURNAL_DATA rujd;
    rujd.BytesToWaitFor=0;//此处由1改成0 0时等待记录卷访问阻塞
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
//     sprintf(buf,"C:\\Documents and Settings\\Administrator\\桌面\\%c.TXT",dwDri+'A');
//     FILE *fp=fopen(buf,"w");

    for(;;)
    {
        Sleep(rand()%1000+1000);
//         fprintf(fp,"调用DeviceIoControl\n");
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
            DebugStringA("%c 线程 FSCTL_READ_USN_JOURNAL 错误%d",'A'+dwDri,dwError);
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
//         fprintf(fp,"结束调用DeviceIoControl\n");
//         fflush(fp);
        if(dwBytes<=sizeof(USN)) {continue;}//结束!

        dwRetBytes = dwBytes - sizeof(USN);//跳过了1个USN，此USN是下一论查询起点
        //即本轮没有收录
        //改进 ：将所有的监视记录先整理下
        pRecord = PUSN_RECORD((PBYTE)Buffer+sizeof(USN));  
        g_Lock.Lock();//当程序关闭时停止进入，避免重复文件操作
        while(dwRetBytes > 0 )//若跳过1个USN后，还有多余字节，则至少有1个记录
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
            //找下一个记录
            pRecord = (PUSN_RECORD)(((PBYTE)pRecord) + pRecord->RecordLength); 
        }
        //更新起始USN
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
        if(g_hVols[dwDri])//是NTFS且信息已获取
        {
            Helper_SetCurrentState(-1,"正在扫描(%c)盘，请稍候...",dwDri+'A');

            //枚举USN
            MFT_ENUM_DATA med;
            med.StartFileReferenceNumber = 0;
            med.LowUsn = 0;
            med.HighUsn = g_curNextUSN[dwDri];
            DWORD cbRet;
            PUSN_RECORD pRecord,pEnd;
            int codeNameLen;    //文件名占内存大小
            DWORD dwMemRecord;//记录所占空间
            PWCHAR pFileName;
            PNORMALFILE_RECORD pFile;
            PDIRECTORY_RECORD pDir;
            BYTE szFileNameCode[768];//最多需要255*3个字节<768


            {//先把根加进去
                //C:  D: ... ROOT=0x000000000005 Parent
                //计算所需空间
                dwMemRecord=GetDirectoryRecordLength(2);
                //分配空间
                pDir=(PDIRECTORY_RECORD)g_MemDir.PushBack(dwMemRecord);
                //填充数据
                pDir->BasicInfo=GetBasicInfo(dwDri,5);
                pDir->ParentBasicInfo=0;
                pDir->NameLength=2;
                pDir->Name[0]=dwDri+'A';
                pDir->Name[1]=':';
                //设置索引
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
                        int idExt=0;//扩展名ID
                        DWORD dwExtIdLen;//扩展名所占用字节数  
                        DWORD dwIconLen=0;
                        for(i=iLen-1;i>=0&&pFileName[i]!=L' '&&pFileName[i]!=L'.';--i);
                        if(i<=0 ||pFileName[i]==L' '){
                            dwExtIdLen=0;//无扩展名 注意，如果文件名以.开头 后接非. 空格字符，不认为是扩展名 此时i==0
                        }else{//
                            idExt=g_ExtMgr.insert(pFileName+i+1,iLen-1-i);
                            if(idExt<CExtArray::s_dwOmitExt) dwIconLen=4;
                            dwExtIdLen=GetExtIdLength(idExt);
                            iLen=i;
                        }
                        
                        //iLen为UCS2文件名长 去.扩展名
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

    Helper_SetCurrentState(-1,"正在构建目录数据库，请稍候...");
    //利用g_DirMap 或者 g_vDirIndex皆可
    //程序中统一使用g_DirMap增强代码可读性
    {
        PDIRECTORY_RECORD pDir;
        PNORMALFILE_RECORD pFile;

        int i;
        PIndexElemType  pData, pDataEnd;
        PINDEX_BLOCK_NODE   *pIndex;
        PINDEX_BLOCK_NODE   pNode;
        int cBlock;

        //先构建目录部分
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
                    else//其父未被索引，例如VISTA WIN7下 \$Extend\$RmMetadata
                    {
                        g_vDirIndex.DecreaseSize();
                        //在索引中删除该项
                        --pDataEnd;
                        for(PIndexElemType p=pData;p<pDataEnd;++p)
                        {
                            *p=*(p+1);
                        }
                        pNode->CurrentEnd-=1;
                        --pData;                      
                    }                    
                }
                //为0时为根目录，不处理
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
                else//其父未被索引
                {
                    g_vFileIndex.DecreaseSize();
                    //在索引中删除该项
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

    Helper_SetCurrentState(-1,"正在排序文件夹，请稍候...");
    {
        //拷贝索引到pTempByte
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
        //对pTempHead[0,...,nDirSize-1]进行排序
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

    Helper_SetCurrentState(-1,"正在排序文件，请稍候...");
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
//     sprintf(buf,"耗时 %f 秒！",(_end.QuadPart-_beg.QuadPart)*1.0/_freq.QuadPart);
//     SetWindowTextA(hMainWnd,buf);
    return 0L;
}

//分析，获取扩展名
//如果存在输入 返回TRUE
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
                if(nExt==nMax) break;//达到最大扩展名数
            }
        }
        pStr=pWch;
    }
    return bRet;
}

void KernelSearch()
{
    if(g_bCanSearch)//可以查询
    {
        //获取当前目录设置
        DWORD *pBasicInfo=0;
        BOOL  *pbSubDir=0;
        int nCheck=g_listDirFilter.GetFileCheckState(pBasicInfo,pbSubDir);
        PDIRECTORY_RECORD* pDirs=NULL;
        
        if(nCheck>0){
            pDirs=new PDIRECTORY_RECORD[nCheck];
            //获取对应的目录信息
            g_vDirIndex.Lock();
            for(int i=0;i<nCheck;++i)
            {
                PDIRECTORY_RECORD* ppDir=(PDIRECTORY_RECORD*)g_DirMap.find(pBasicInfo[i]);
                assert(ppDir && "目录一定存在库中");
                pDirs[i]=*(ppDir+1);
            }
            g_vDirIndex.UnLock();
            delete []pBasicInfo;
            pBasicInfo=NULL;
        }
              
        static WCHAR szSearchStr[1024];//当前要查询的串
        GetWindowTextW(g_hEdit,szSearchStr,1024);

        static WCHAR szExtSearch[512];
        GetWindowTextW(g_hExtEdit,szExtSearch,512);//获取扩展名串

        int IdExt[64];//最大支持64个扩展名
        int nExt;       //可用扩展名数量
        BOOL bHasExt=GetExtIdFromStr(szExtSearch,IdExt,64,nExt);
        int *pIdExt;
        if(!bHasExt) pIdExt=NULL;
        else pIdExt=IdExt;

        BOOL bAvaiable=TRUE;
        if(g_bCase){//大小写敏感
            if(!ArrangeSearchStrCase(szSearchStr)){
                bAvaiable=FALSE;
            }
        }else{//不敏感
            if(!ArrangeSearchStrNoCase(szSearchStr)){
                bAvaiable=FALSE;
            }  
        }

        if(!bAvaiable ||(bHasExt && 0==nExt) ||0==nCheck){//输入的都是无效
            g_vDirOutPtr.clear_all();
            g_vFileOutPtr.clear_all();
            g_LastStrOptCase.Reset();
            g_LastStrOptNoCase.Reset();
            Helper_SetCurrentState(0,NULL); 
            ListView_SetItemCount(g_hListCtrl,0);
            ListView_RedrawItems(g_hListCtrl,0,-1);
        }else{
            SearchStrOpt *pStrOpt;
                //先检查输入串是否和之前的串相同
            BOOL bNameChange=FALSE;
            if(g_bCase){
                if(!(g_StrOptCase==g_LastStrOptCase)) bNameChange=TRUE;
            }else{
                if(!(g_StrOptNoCase==g_LastStrOptNoCase)) bNameChange=TRUE;
            }

            if( g_bDirSetting           //目录选项改变
                ||g_bOptChanged         //选项改变
                ||g_bMonitorChange      //监视改变
                ||bNameChange           //输入名串改变
                ||g_bExtChange          //输入扩展名串改变                    
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

            if(pStrOpt->HasQuestion()){//含有?，可能含有【普通，*，*?】
                Helper_QuestionSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);
            }else{//不含【?】
                if(pStrOpt->HasStar()){//不含【?】，含有*，可能含有【普通，*?】
                    Helper_StarSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);
                }else{//不含【?，*】*a
                    if(pStrOpt->HasStarQuestion()){//不含【?，*】，含有?*，可能含有【普通】
                        Helper_StarQuestionSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);
                    }else{//不含【?，*，?*】，含有普通
                        //assert(pStrOpt->pNormal && "一定含有普通串");//普通串已处理
                        if(pStrOpt->pNormal){//当全*时不含普通串
                            if(pStrOpt->_bAllChar){
                                Helper_OnlyNormalSearchAscii(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);//AC算法【模式串经过预处理，不会出现一后者在前者中出现】
                            }
                            else{
                                Helper_OnlyNormalSearch(g_bCase,g_bDirSearch,g_bFileSearch,pIdExt,nExt,pDirs,pbSubDir,nCheck);//暴力
                            }
                        }
                    }
                }
            } 
            //当文件或文件夹选项其一未指定时，其size==0
            Helper_SetCurrentState(g_vFileOutPtr.size()+g_vDirOutPtr.size(),NULL);
            ListView_SetItemCount(g_hListCtrl,g_vFileOutPtr.size()+g_vDirOutPtr.size());
        }
        g_bOptChanged=FALSE;//调用时已经重置
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
            //MessageBoxA(hwnd,"请见产品文档","友情提醒",MB_ICONINFORMATION);
            //ShellExcute
        }break;
    case IDM_ABOUT:
        {
        }break;
    case IDM_CASE://大小写敏感子菜单
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
                    MessageBoxA(hwnd,"文件和文件夹至少指定一个！","友情提醒！",MB_ICONINFORMATION);
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
                    MessageBoxA(hwnd,"文件和文件夹至少指定一个！","友情提醒！",MB_ICONINFORMATION);
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
                ModifyMenuW(hMenu,1,MF_BYPOSITION|MF_STRING,IDM_DIR_SETTING,L"展开目录设置(&M)");
            }else{
                if(g_bFilterExpand){
                    //改菜单名为：收起目录设置
                    ModifyMenuW(hMenu,2,MF_BYPOSITION|MF_STRING,IDM_FILTER,L"展开过滤选项(&G)");
                    g_bFilterExpand=!g_bFilterExpand;  
                }               
                ModifyMenuW(hMenu,1,MF_BYPOSITION|MF_STRING,IDM_DIR_SETTING,L"收起目录设置(&M)");
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
                //改菜单名为：收起目录设置
                ModifyMenuW(hMenu,2,MF_BYPOSITION|MF_STRING,IDM_FILTER,L"展开过滤选项(&G)");
            }else{
                //改菜单名为：展开目录设置
                ModifyMenuW(hMenu,2,MF_BYPOSITION|MF_STRING,IDM_FILTER,L"收起过滤选项(&G)");
                if(g_bDirSetExpand){
                    ModifyMenuW(hMenu,1,MF_BYPOSITION|MF_STRING,IDM_DIR_SETTING,L"展开目录设置(&M)");
                    g_bDirSetExpand=!g_bDirSetExpand; 
                }
            }
            g_bFilterExpand=!g_bFilterExpand;
            DrawMenuBar(hwnd);
            UpdateLayout(hwnd);
        }
        break;
    case IDC_EDIT1://文件名编辑框
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
#pragma region 目录过滤选项
    case IDC_BTN_ADDDIR://添加一个目录
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.AddFilterDirectory()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break;      
    case IDC_BTN_DELDIR://删除指定的目录
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.DeleteItemsSelected()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break; 
    case IDC_BTN_DELALLDIR://删除所有目录
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.DeleteAllDirItems()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break;
    case IDC_BTN_CHECKALLDRI://勾选所有盘符
        {
            if(BN_CLICKED==codeNotify) 
                if(g_listDirFilter.CheckAllDriverItems()){
                    g_bDirSetting=TRUE;
                    KernelSearch();
                    g_bDirSetting=FALSE;
                }
        }break;
#pragma endregion

#pragma region 搜索过滤按钮
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
 *      获取结果项的文件名信息
 *	Parameter(s):
 *      iItemIndex 第几项
 *      path 父目录带L'\\' 若当前目录为根，父目录为空 
 *      fileName 小文件名，带扩展名
 *      extName 扩展名
 *	Return:	
 *      path的长度 为0表示根目录
 *	Commons:
 **/
int GetFileName(IN int iItemIndx,OUT PWCHAR path,OUT PWCHAR fileName,OUT PWCHAR extName=NULL)
{
    PBYTE pName;    //文件名信息访问指针
    int fileLen;    //解码文件名长
    int nameLen;    //编码文件名长

    PDIRECTORY_RECORD pCurDir;      //此两者用以区分当前显示的是目录还是文件，
    PNORMALFILE_RECORD pCurFile;    //并且保存当前记录首地址

    PDIRECTORY_RECORD pCurParent;
    if(iItemIndx<g_vDirOutPtr.size()){//目录
        pCurDir=PDIRECTORY_RECORD(g_vDirOutPtr[iItemIndx]);
        pCurFile=NULL;
        pCurParent=pCurDir->ParentPtr;
        nameLen=pCurDir->GetCodeName(pName);
    }else{//文件
        pCurDir=NULL;
        pCurFile=PNORMALFILE_RECORD(g_vFileOutPtr[iItemIndx-g_vDirOutPtr.size()]);
        pCurParent=pCurFile->ParentPtr;
        nameLen=pCurFile->GetCodeName(pName);
    }
    fileLen=Helper_CodeToUcs2Case(fileName,pName,nameLen);
    
    if(pCurFile){
        pName+=nameLen;//扩展名位置
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

                static WCHAR path[MAX_PATH];//目录
                static int pathLen;

                static PDIRECTORY_RECORD pCurDir;//此两者用以区分当前显示的是目录还是文件，
                static PNORMALFILE_RECORD pCurFile;//并且保存当前记录首地址

                static PWCHAR pExt;//扩展名
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
                            if(iItemIndx<g_vDirOutPtr.size()){//目录
                                pCurDir=PDIRECTORY_RECORD(g_vDirOutPtr[iItemIndx]);
                                Helper_GetBasicInformation(pCurDir->BasicInfo>>27,pCurDir->BasicInfo&0x7ffffff
                                    ,&llFileCreateTm,&llFileLastAccessTm,&llFileLastChangeTm
                                    ,NULL,&FileAttri);
                                pCurParent=pCurDir->ParentPtr;
                                nameLen=pCurDir->GetCodeName(pName);
                            }else{//文件
                                pCurFile=PNORMALFILE_RECORD(g_vFileOutPtr[iItemIndx-g_vDirOutPtr.size()]);
                                Helper_GetBasicInformation(pCurFile->ParentPtr->BasicInfo>>27,pCurFile->BasicInfo&0x7ffffff
                                    ,&llFileCreateTm,&llFileLastAccessTm,&llFileLastChangeTm
                                    ,&fileSize,&FileAttri);
                                pCurParent=pCurFile->ParentPtr;
                                nameLen=pCurFile->GetCodeName(pName);
                            }
                            fileLen=Helper_CodeToUcs2Case(sFileName,pName,nameLen);
                            wcsncpy(pItem->pszText,sFileName,pItem->cchTextMax);

                            if(pCurDir){//当前item是目录     
                                int pathLen=Helper_GetPath(path,pCurParent);
                                if(0==pathLen) iIcon=g_iRootIcon;
                                else iIcon=g_iDirIcon;
                                pExt=NULL;                                    
                            }
                            else
                            {//在获取的pFileName之后加上扩展名
                                pName+=nameLen;//扩展名位置
                                DWORD dwExtLen;
                                int idExt=pCurFile->GetExtendID(pName,&dwExtLen);
                                if(-1==idExt){//无扩展名
                                    iIcon=3;
                                    pathLen=Helper_GetPath(path,pCurParent);
                                    pExt=NULL;
                                }else{//有扩展名
                                    pExt=g_ExtMgr.GetExtName(idExt);

                                    //生成文件名
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

                                    if(idExt<CExtArray::s_dwOmitExt){//ICON存在本地
                                                                    //也可通过BasicInfo中BITMASK判断
                                        DWORD *pIcon=(DWORD*)(pName+dwExtLen);
                                        if(0xFFFFFFFF==*pIcon){
                                            *pIcon=Helper_GetFileIconIndex(path,pathLen,sFileName,fileLen);
                                        }
                                        iIcon=*pIcon;   
                                    }else{//通过扩展名管理获取ICON
                                        iIcon=g_ExtMgr.GetIconIndex(idExt);  
                                        if(iIcon==ICON_INDEX_UNINITIALIZED){
                                            iIcon=g_ExtMgr.SetIconIndex(idExt,path,pathLen,sFileName,fileLen);
                                        }
                                    }
                                }

                            }//!bDir
                        }
                        break;
                    case 1://显示扩展名
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
                    case 3: //显示文件(非目录)大小
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
                    case 4://创建时间
                        {
                            SYSTEMTIME fileCreateTm;
                            FILETIME fileTime;
                            FileTimeToLocalFileTime((PFILETIME)&llFileCreateTm,&fileTime);
                            FileTimeToSystemTime(&fileTime,&fileCreateTm);
                            wsprintfW(pItem->pszText,L"%d年%d月%d日 %d时%d分"
                                ,fileCreateTm.wYear,fileCreateTm.wMonth,fileCreateTm.wDay
                                ,fileCreateTm.wHour,fileCreateTm.wMinute
                                );
                        }
                        break;
                    case 5://最近修改时间
                        {
                            SYSTEMTIME fileLastChangeTm;
                            FILETIME fileTime;
                            FileTimeToLocalFileTime((PFILETIME)&llFileLastChangeTm,&fileTime);
                            FileTimeToSystemTime(&fileTime,&fileLastChangeTm);
                            wsprintfW(pItem->pszText,L"%d年%d月%d日 %d时%d分"
                                ,fileLastChangeTm.wYear,fileLastChangeTm.wMonth,fileLastChangeTm.wDay
                                ,fileLastChangeTm.wHour,fileLastChangeTm.wMinute
                                );
                        }
                        break;
                    case 6://最近访问时间
                        {
                            SYSTEMTIME fileLastAccessTm;
                            FILETIME fileTime;
                            FileTimeToLocalFileTime((PFILETIME)&llFileLastAccessTm,&fileTime);
                            FileTimeToSystemTime(&fileTime,&fileLastAccessTm);
                            wsprintfW(pItem->pszText,L"%d年%d月%d日 %d时%d分"
                                ,fileLastAccessTm.wYear,fileLastAccessTm.wMonth,fileLastAccessTm.wDay
                                ,fileLastAccessTm.wHour,fileLastAccessTm.wMinute
                                );
                        }
                        break;
                    case 7:
                        {
                            PWCHAR pWch=pItem->pszText;
                            if(0==(FileAttri&(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))){
                                *pWch++=L'[';*pWch++=L'普';*pWch++=L'通';*pWch++=L']';
                            }else{
                                *pWch++=L'[';
                                if(FileAttri&FILE_ATTRIBUTE_HIDDEN)
                                {
                                    *pWch++=L'隐';*pWch++=L'藏';*pWch++=L',';
                                }
                                if(FileAttri&FILE_ATTRIBUTE_SYSTEM)
                                {
                                    *pWch++=L'系';*pWch++=L'统';*pWch++=L',';
                                }   
                                *(pWch-1)=L']';
                            }
                            if(NULL==pCurFile){
                                wcscpy(pWch,L"文件夹");
                            }else{
                                wcscpy(pWch,L"文件");
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
            else if(NM_RCLICK==pNMHDR->code){//右键点击
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
                                hr=::SHGetSpecialFolderLocation(NULL,CSIDL_DRIVES,&pidlParent);//我的电脑pidl
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
                        if(0==pathLen){//根目录
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
#pragma region 目录过滤选项
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
 *      程序退出时写入数据库
 *	Parameter(s):
 *
 *	Return:	
 *
 *	Commons:
 *      将写入方式从内存映射文件改为写内存方式
 *      增加写入当前系统时间
 **/
void WriteToDatabase()
{
    //等待每个线程自动停止
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

    {//计算父到偏移映射
        DWORD nBlockSize=CMemoryPool::GetBlockSize();//根据此值计算偏移
        DWORD iBlock=0,iLastPos=0;//偏移块，块内偏移

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

    //初始化文件夹偏移部分
    PBYTE pByte;
    {
        //4b 文件大小 MAP偏移 DIR偏移 FILE偏移   =16B        
        //4b tag_'雄青' tag_'青雄' tag_version dir_size file_size =20B
        //2b time_year time_month time_day time_hour  =8B
        BYTE Buffer[16+20+8+26*9];

        DWORD *pdwBuf=(DWORD*)(Buffer+16);
        *pdwBuf++='雄青';*pdwBuf++='青雄';*pdwBuf++=0x01000001;
        *pdwBuf++=g_vDirIndex.size();
        *pdwBuf++=g_vFileIndex.size();

        //写入时间
        WORD* pWord=(WORD*)(Buffer+16+20);
        SYSTEMTIME curSysTm;
        GetSystemTime(&curSysTm);
        *pWord++=curSysTm.wYear;
        *pWord++=curSysTm.wMonth;
        *pWord++=curSysTm.wDay;
        *pWord++=curSysTm.wHour;


        pByte=Buffer+16+20+8;
        for(int i=0;i<26;++i)//检查每个盘是否有JournalID NextUsn
        {
            if(g_hVols[i]){
                *pByte++=1;
                *(DWORDLONG*)pByte=g_curJournalID[i];pByte+=8;
                *(USN*)pByte=g_curNextUSN[i];pByte+=8;
            }else *pByte++=0;
        }
        DWORD dwSize=pByte-Buffer;
        *(DWORD*)(Buffer+4)=dwSize;//头四字节，偏移到map数组
        o_WriteMgr.Write(Buffer,dwSize);
    }


    {
        //PBYTE pTempBuf=g_MemoryMgr.GetMemory(nDirCount*sizeof(DWORD));
        //DWORD *pDwArr=(DWORD*)pTempBuf;

        //offsets表按照BasicInfo升序
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
                if(*ppDir){ //如果是0表示已经删除
                    o_WriteMgr.Write(&mapParentPtr2Offset[*ppDir],4);
                }
            }
        }
        o_WriteMgr.ReWriteFirstBlock(2,o_WriteMgr.GetTotalWirte());  
    }

    BYTE TempBuffer[1024];//先将数据写入临时BUFFER

    PBYTE   pLastCodeName=NULL;//上一个文件名起始地址
    DWORD   dwLastCodeNameLen=0;//上一个文件名长
    PBYTE   pCodeName;
    DWORD   dwCodeNameLen;
    DWORD   dwComLen;//公共长
    {//一次写入目录数据(带压缩)
        //重置所有父Ptr为父Offset 
        //若负偏移为0xFFFFFFFF,表明其父指针为NULL
        
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();
        for(i=0;i<cBlock;++i)//直接操作新数据区
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);//旧指针
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

    pLastCodeName=NULL;//上一个文件名起始地址
    dwLastCodeNameLen=0;//上一个文件名长
    {//一次写入文件数据(带压缩)
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
                //写入到文件
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
    g_ExtMgr.WriteToFile(CURDIR L"ext.db");//写如扩展名

    //干掉all线程
    for(int i=0;i<26;++i)
    {
        if(g_hThread[i]) TerminateThread(g_hThread[i],0);
    }
    g_Lock.UnLock();
}

/**
 *	Function:
 *      打开各NTFS卷，并获取相关信息
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
    DWORD dwDri; //卷号 0~25

    WCHAR szRootName[40];
    WCHAR szVolumeName[32];
    int iFilterRoot=0;
#ifdef TEST
    WCHAR *p=TEST_ROOT_DIR;
#else
    for(WCHAR *p=tDrivers;*p!='\0';p+=4)
#endif
    {       
        if(*p>=L'a') *p-=32;//变大写
        dwDri=*p-L'A';
        if(DRIVE_FIXED==GetDriveTypeW(p))
        {
            DWORD dwMaxComLen,dwFileSysFlag;
            GetVolumeInformationW(p,szVolumeName,32,NULL,&dwMaxComLen,&dwFileSysFlag,fileSysBuf,8);
            if(fileSysBuf[0]==L'N' && fileSysBuf[1]==L'T' && fileSysBuf[2]==L'F' && fileSysBuf[3]==L'S')
            {
                swprintf(szRootName,L"%s (%c:)",szVolumeName,*p);
                g_listDirFilter.InsertItem(iFilterRoot,szRootName,TRUE);
                g_listDirFilter.SetItemText(iFilterRoot,1,L"是");
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
                    DebugStringA("创建卷%c失败 代码%d",*p,GetLastError());
#ifdef TEST
                    return;
#else
                    continue;
#endif
                }              
                g_hVols[dwDri]=hVolume;//保存句柄

                
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
                if(!QueryUsnJournal(hVolume,&ujd)){//查询失败
                    //DebugStringA("查询失败");
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
 *      加载数据库,建立基本数据结构，包括加载和更新
 *	Parameter(s):
 *      
 *	Return:	
 *      若成功加载，返回TRUE;否则表示需要重新扫面数据库
 *	Commons:
 **/
BOOL LoadDatabase(HWND hMainWnd)
{
    Helper_SetCurrentState(-1,"正在加载数据库...");
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
        DebugStringA("获取文件大小失败:%d",GetLastError());
        bRet=FALSE;
        goto FILE_SIZE_CHECK;
    }

    PBYTE DB_Buffer=g_MemoryMgr.GetMemory(dwFileSize);

    DWORD dwRead;
    BOOL bReadOK=ReadFile(hDB,DB_Buffer,dwFileSize,&dwRead,NULL);

    if(!bReadOK || dwRead!=dwFileSize ){
        DebugStringA("读文件失败:%d",GetLastError());
        bRet=FALSE;
        goto READ_FILE_CHECK;
    }

    //4b 文件大小 MAP偏移 DIR偏移 FILE偏移   =16B
    //4b tag_'雄青' tag_'青雄' tag_version dir_size file_size =20B
    //2b time_year time_month time_day time_hour  =8B

    WORD wYear,wMonth,wDay,wHour;

    DWORD *pTag=(DWORD *)DB_Buffer;
    DWORD nDirCount=pTag[7];
    DWORD nFileCount=pTag[8];
    if(0==nDirCount||dwFileSize!=*pTag||pTag[1]<(16+20+8+26) || pTag[2]-pTag[1]!=(nDirCount<<2) || pTag[4]!='雄青' || pTag[5]!='青雄' || pTag[6]!=0x01000001){
        bRet=FALSE;
        goto FILE_TAG_CHECK;
    }

    PBYTE pByte=PBYTE(pTag+9);
    wYear=*(WORD*)pByte;pByte+=2;
    wMonth=*(WORD*)pByte;pByte+=2;
    wDay=*(WORD*)pByte;pByte+=2;
    wHour=*(WORD*)pByte;pByte+=2;

    assert(bRet);
    //检查Journal ID
    USN usnLast[26]={0};
    {     
        DWORD dwDri=0;
        for(dwDri=0;dwDri<26;++dwDri)
        {
            if(*pByte++){
                if(*(DWORDLONG*)pByte!=g_curJournalID[dwDri]) {//Journal ID已经改变
                    bRet=FALSE;
                    break;
                }
                pByte+=8;
                usnLast[dwDri]=*(USN*)pByte;//修改要读的USN起点
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
            "当前数据库存储时间： %d年%d月%d日%d时。已经过期！\r\n"
            "您是否需要更新？\r\n\r\n"
            "提醒：如果选择是，需要您耐心等待...^_^\r\n"
            "      如果选择否，本次软件将不会监视文件系统的改变！此时，您仍然\r\n"
            "      可以搜索到以前保存的文件！"
            ,wYear,wMonth,wDay,wHour
            );
        if(IDYES==::MessageBoxA(hMainWnd,buf,"友情更新提醒",MB_YESNO|MB_ICONINFORMATION))
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

    //构建目录及其索引
    //还原目录数据，写入g_MemDir
    PBYTE pDirBuf=DB_Buffer+pTag[2],pDirEnd=DB_Buffer+pTag[3],pNextBeg;
    PBYTE pLastName=NULL;
    //结构为
    //0x0 4B BasicInfo
    //0x4 4B Offset
    //0x8 3B 低10bit为dwCommLen 次低10位为dwNameLenLeft
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
        g_vDirIndex.push_back((IndexElemType)pDir);//增加目录索引
        pDir->BasicInfo=*(DWORD*)(pDirBuf);pDirBuf+=4;
        pDir->ParentOffset=*(DWORD*)(pDirBuf);pDirBuf+=7; //直接跳过3字节部分      
        if(dwTemp){
            pDir->NameLength=dwNameLen|0x8000;
        }else{
            pDir->NameLength=dwNameLen;
        }
        pName=pDir->Name;
        for(i=0;i<dwCommLen;++i){//拷贝公共部分
            *pName++=pLastName[i];
        }
        pNextBeg=pDirBuf+dwNameLenLeft;
        for(;pDirBuf<pNextBeg;)
        {
            *pName++=*pDirBuf++;
        }
        pLastName=pDir->Name;//更新pLastName
    }

    
    //产生g_DirMap 及目录Offset->Ptr
    //注意产生Offset->Ptr无须遍历目录索引
    DWORD *pOffset=(DWORD*)(DB_Buffer+pTag[1]),*pOffsetEnd=pOffset+nDirCount;
    for(;pOffset<pOffsetEnd;++pOffset)
    {
        pDir=(PDIRECTORY_RECORD)g_MemDir.DB_FromOffsetToPtr(*pOffset);
        pDir->ParentPtr=(PDIRECTORY_RECORD)g_MemDir.DB_FromOffsetToPtr(pDir->ParentOffset);
        g_DirMap.push_back(pDir->BasicInfo,(IndexElemType)pDir);
    }

    //文件结构为
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
        dwNameLenLeft=dwTemp&0x3FF;//和目录不同 无0x7F标志
        dwExtLength=(dwTemp>>10);//扩展名长
        dwNameLen=dwCommLen+dwNameLenLeft;
        dwMemRecord=GetNormalFileRecordLength(dwNameLen,dwNameLenLength,dwExtLength,dwIconLen);
        pFile=(PNORMALFILE_RECORD)g_MemFile.PushBack(dwMemRecord);
        g_vFileIndex.push_back((IndexElemType)pFile);//增加目录索引
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
        for(i=0;i<dwCommLen;++i){//拷贝公共部分
            *pName++=pLastName[i];
        }
        pNextBeg=pFileBuf+dwNameLenLeft;
        for(;pFileBuf<pNextBeg;)
        {
            *pName++=*pFileBuf++;
        }
        pLastName=pTempLastName;//更新pLastName

        //拷贝扩展名
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
    if(bRet && !g_bJournalIdFailure)//仅当加载数据库成功时更新 NextUSN为文件中的存储值
    {
        for(int i=0;i<26;++i) g_curNextUSN[i]=usnLast[i];
    }//数据库加载失败，不应改变g_curNextUSN值
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
    Helper_SetCurrentState(-1,"正在更新数据库...");
    const DWORD SEARCH_TITLE_REASON_FLAG=
        USN_REASON_FILE_CREATE              
        |USN_REASON_FILE_DELETE
        |USN_REASON_RENAME_OLD_NAME
        |USN_REASON_RENAME_NEW_NAME
        ;
    READ_USN_JOURNAL_DATA rujd;
    rujd.BytesToWaitFor=0;//没有数据就结束
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
            DebugTrace("更新%c盘\n",dwDri+'A');
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
                    DebugStringA("%c 线程 FSCTL_READ_USN_JOURNAL 错误%d",'A'+dwDri,dwError);
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
                if(dwBytes<=sizeof(USN)) break;//结束!

                dwRetBytes = dwBytes - sizeof(USN);//跳过了1个USN，此USN是下一论查询起点
                //即本轮没有收录

                pRecord = PUSN_RECORD((PBYTE)Buffer+sizeof(USN));  
                while(dwRetBytes > 0 )//若跳过1个USN后，还有多余字节，则至少有1个记录
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
                    //找下一个记录
                    pRecord = (PUSN_RECORD)(((PBYTE)pRecord) + pRecord->RecordLength); 
                }
                //更新起始USN
                rujd.StartUsn = g_curNextUSN[dwDri]=*(USN*)Buffer; 
            }   
            DebugTrace("更新%c盘 结束\n",dwDri+'A');
        }
    }
    return 0;
}


//初始化各控件
void InitCtrl(HWND hMainWnd)
{
    g_hStateWnd=CreateStatusWindowW(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        L"准备",hMainWnd,12345);
    g_hListCtrl=GetDlgItem(hMainWnd,IDC_LIST1);
    g_hEdit=GetDlgItem(hMainWnd,IDC_EDIT1);
    g_hExtEdit=GetDlgItem(hMainWnd,IDC_EDIT2);
    UpdateLayout(hMainWnd);
    SendMessage(g_hEdit,EM_LIMITTEXT,1024,0);//文件名框，限制用户输入的最大字符个数
    SendMessage(g_hExtEdit,EM_LIMITTEXT,512,0);//扩展名框，限制用户输入的最大字符个数

    DWORD dwStyle=GetWindowLong(g_hListCtrl,GWL_STYLE);
    SetWindowLong(g_hListCtrl,GWL_STYLE
        ,dwStyle
        |WS_CLIPSIBLINGS
        |WS_CLIPCHILDREN
        |LVS_ICON
        |LVS_SHOWSELALWAYS
        /*                |LVS_SORTASCENDING|LVS_SORTDESCENDING*/
        |LVS_SHAREIMAGELISTS //imagelist auto 销毁 当ctrl销毁
        /*                |LVS_EDITLABELS*/ //可编辑
        );
    dwStyle=ListView_GetExtendedListViewStyle(g_hListCtrl);
    dwStyle |=WS_EX_LEFT|
        WS_EX_LTRREADING|
        WS_EX_RIGHTSCROLLBAR|
        //LVS_EX_HEADERDRAGDROP|  //拖动列头，交换两列
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

    lvCol.pszText=_T("文件名");
    lvCol.cx=200;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("扩展名");
    lvCol.cx=60;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("所在目录");
    lvCol.cx=500;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.fmt=LVCFMT_RIGHT;
    lvCol.pszText=_T("文件大小");
    lvCol.cx=80;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.fmt=LVCFMT_CENTER;
    lvCol.pszText=_T("创建时间");
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("最近修改时间");
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.pszText=_T("最近访问时间");
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    lvCol.fmt=LVCFMT_LEFT;
    lvCol.pszText=_T("属性");//只读 隐藏 系统
    lvCol.cx=150;
    ListView_InsertColumn(g_hListCtrl,iCol++,&lvCol);

    //初始化过滤控件
    g_listDirFilter.FromHandle(::GetDlgItem(hMainWnd,IDC_LIST_FILTER));
    HWND hBtnAdd=::GetDlgItem(hMainWnd,IDC_BTN_ADDDIR)
        ,hBtnDel=::GetDlgItem(hMainWnd,IDC_BTN_DELDIR)
        ,hBtnDelAll=::GetDlgItem(hMainWnd,IDC_BTN_DELALLDIR)
        ,hBtnCheckAllDri=::GetDlgItem(hMainWnd,IDC_BTN_CHECKALLDRI);
    g_listDirFilter.CreateContext(hBtnAdd,hBtnDel,hBtnDelAll,hBtnCheckAllDri,hMainWnd);//创建
    g_listDirFilter.SetImageList(hImageList,g_iRootIcon,g_iDirIcon);

    iCol=0;
    lvCol.fmt=LVCFMT_LEFT;
    lvCol.pszText=_T("所在目录");
    lvCol.cx=300;
    g_listDirFilter.InsertColumn(iCol++,&lvCol);

    lvCol.pszText=_T("搜子目录吗？");
    lvCol.cx=100;
    g_listDirFilter.InsertColumn(iCol++,&lvCol);

    g_listDirFilter.Hide();


    //初始化过滤控件
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
    Helper_SetCurrentState(-1,"初始化...");
    OpenNtfsVolume();//打开各NTFS卷
    BOOL bLoad=LoadDatabase(hMainWnd);
    if(!bLoad || !g_ExtMgr.LoadFromFile(CURDIR L"ext.db"))
    {
        //监视线程在InitScanMftProc中启动
        g_ExtMgr.InitRealTimeCallExt();
        InitScanMftProc(hMainWnd);
    }
    else
    {
        if(!g_bJournalIdFailure) UpdateDatabase(hMainWnd);
    }
    g_bCanSearch=TRUE;
    KernelSearch();

    //启动监视线程  
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
            //初始化格控件
            InitCtrl(hWnd);

            //初始化no_case表
            Helper_InitNoCaseTable();

            //初始化汉字排序表
            Help_InitCompare();

            //该线程包括 加载数据库 若加载不成功 则扫描MFT
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
            if(g_bCanSearch) //否则，数据库都未成型
            {
                if(!g_bJournalIdFailure) WriteToDatabase();//将数据写入数据库中
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
            MessageBox(NULL,_T("GetMessage error with -1 returned！"),_T("error"),MB_ICONHAND);
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


