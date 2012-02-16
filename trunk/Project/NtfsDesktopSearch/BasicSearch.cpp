// BasicSearch.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "StrMatch.h"

const int ALPHABETA_SIZE=0x80;
const short FAIL_STATE=0x8000; //-32768 用于AC算法
BYTE g_NoCaseTable[ALPHABETA_SIZE]; //搜索时已经初始化

void Helper_InitNoCaseTable()
{
    for(int i=0;i<ALPHABETA_SIZE;++i){
        if(i>='A' && i<='Z') g_NoCaseTable[i]=i+32;
        else g_NoCaseTable[i]=i;
    }
}

//pParentDir是待考察文件的直接父指针
__forceinline BOOL IsExtnameSat(PDIRECTORY_RECORD pParentDir,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck)//扩展名满足么
{
    PDIRECTORY_RECORD p;
    for(int i=0;i<nCheck;++i)
    {
        p=pParentDir;
        if(pbSubDir[i]){//含子目录
            for(;p;p=p->ParentPtr)
            {
                if(ppDir[i]==p) return TRUE;
            }
        }else{//不包含子目录
            if(ppDir[i]==p) return TRUE;
        }
    }
    return FALSE;
}


//遍历文件，使用AC算法判断<0x80转换表
//空间优化9~0x31不存在
const DWORDLONG MAX_DWORDLONG=0xffffffffffffffff;
void Helper_OnlyNormalSearchAscii(BOOL bCase
                                  ,BOOL bDirSearch,BOOL bFileSearch
                                  ,int *pIdExt,int nExt
                                  ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                                  )
{
    if(pIdExt) bDirSearch=FALSE;//如果存在有效扩展名 不搜索目录

    SearchStrOpt *pStrOpt;
    if(bCase) pStrOpt=&g_StrOptCase;
    else pStrOpt=&g_StrOptNoCase;

    ListNormal *pNormal;
    int _nMaxStates=1;//最大状态数，至少一个初态
    DWORDLONG nTerminalTag=MAX_DWORDLONG;
    short iFinalState=-1;
    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){
        _nMaxStates+=pNormal->_len;
        nTerminalTag&=~(0x1<<(-iFinalState--));
    }

    //终止状态标志<0 [-1 -2 -3]
    short *pStateTable=new short[_nMaxStates*ALPHABETA_SIZE];
    short **pStateBase =new short*[_nMaxStates];
    //pStateBase[i][0]表示是否为终态标识 pStateBase[i][1]表示fail函数
    for(int i=0;i<_nMaxStates;++i) pStateBase[i]=pStateTable+i*ALPHABETA_SIZE;

    //初始化所有TO状态为失效状态
    for(int i=0;i<_nMaxStates;++i){
        *pStateBase[i]=0;//<0时表示终止状态
        for(int j=32;j<ALPHABETA_SIZE;++j) pStateBase[i][j]=FAIL_STATE;
    }
    PWCHAR pWch,pWchEnd;  
    short state,nextState;//状态变量
    short nTotalState=0;//总状态数
    iFinalState=-1;
    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){//增加模式到AC图
        state=0;
        pWch=pNormal->_wchs;pWchEnd=pNormal->_wchs+pNormal->_len;
        for(;pWch<pWchEnd;pWch++){//检查前缀匹配
            nextState=pStateBase[state][*pWch];
            if(FAIL_STATE==nextState) break;
            state=nextState;
        }
        //继续添加
        for(;pWch<pWchEnd;pWch++){//检查前缀匹配
            pStateBase[state][*pWch]=++nTotalState;
            state=nTotalState;
        }
        *pStateBase[state]=iFinalState--;//设置终态
    }


    for(int i=32;i<ALPHABETA_SIZE;++i){//设置初态
        if(FAIL_STATE==pStateBase[0][i]) pStateBase[0][i]=0;
    }
    //for all i ,pStateBase[0][i]>=0

    //构建fail函数，层次遍历
    Queue<short> queue;
    for(int i=32;i<ALPHABETA_SIZE;++i){
        state=pStateBase[0][i];
        if(state!=0){
            pStateBase[state][1]=0;//第一个元素表示失效TO
            queue.Add(state);            
        }
    }
    while(!queue.IsEmpty()){
        queue.Delete(state);//state的失效函数已设置
        short s,fail;
        for(int i=32;i<ALPHABETA_SIZE;++i){
            s=pStateBase[state][i];
            if(s!=FAIL_STATE){
                fail=pStateBase[state][1];
                //assert(pStateBase[0][i]!=FAIL_STATE);
                while(FAIL_STATE==pStateBase[fail][i]){
                    fail=pStateBase[fail][1];
                }
                pStateBase[s][1]=pStateBase[fail][i];
                queue.Add(s);
            }else{
                pStateBase[state][i]=pStateBase[pStateBase[state][1]][i];//高级AC 搜索时不考虑失效 顺链访问
            }
        }
    }

    ///搜索开始
    int _nMaxLen=pStrOpt->pNormal->_len;//最大长度，小于此长度的文件跳过 
    short curState;
    DWORDLONG terminalTag;
    PIndexElemType pData,pDataEnd;
    PBYTE pName,pNameEnd;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //如过存在目录过滤 需要全部锁

    if(bDirSearch)
    {
        if(!ppDir) g_vDirIndex.Lock();
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();   
        PDIRECTORY_RECORD pDir;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                nameLen=pDir->GetCodeName(pName);
                if(nameLen>=_nMaxLen){
                    pNameEnd=pName+nameLen;
                    curState=0;
                    terminalTag=nTerminalTag;
                    if(pDir->HasNonAscii()){//文件名中存在非ASCII
                        while(pName<pNameEnd){
                            if(*pName<0x80){
                                if(bCase)
                                    curState=pStateBase[curState][*pName];
                                else
                                    curState=pStateBase[curState][g_NoCaseTable[*pName]];
                                if(*pStateBase[curState]<0) terminalTag|=DWORDLONG(0x1<<(-*pStateBase[curState]));
                                ++pName;
                            }else{
                                curState=0;pName+=3;
                            }
                        }//while
                    }else{
                        for(;pName<pNameEnd;++pName){//无须判断是否为汉字 加速搜索
                            if(bCase)
                                curState=pStateBase[curState][*pName];
                            else
                                curState=pStateBase[curState][g_NoCaseTable[*pName]];
                            if(*pStateBase[curState]<0) terminalTag|=DWORDLONG(0x1<<(-*pStateBase[curState]));
                        }//while
                    }
                    if(MAX_DWORDLONG==terminalTag){
                        if(NULL==ppDir || IsExtnameSat(pDir->ParentPtr,ppDir,pbSubDir,nCheck)) 
                            g_vDirOutPtr.push_back(*pData); 
                    }
                }//else 长度不够格                        
            }
        }
        if(!ppDir) g_vDirIndex.UnLock();
    }


    if(bFileSearch)
    {
        g_vFileIndex.Lock();
        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        PNORMALFILE_RECORD pFile;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pFile=PNORMALFILE_RECORD(*pData);
                nameLen=pFile->GetCodeName(pName);
                if(nameLen>=_nMaxLen){
                    pNameEnd=pName+nameLen;
                    curState=0;
                    terminalTag=nTerminalTag;
                    if(pFile->HasNonAscii()){//文件名中存在非ASCII
                        while(pName<pNameEnd){
                            if(*pName<0x80){
                                if(bCase)
                                    curState=pStateBase[curState][*pName];
                                else
                                    curState=pStateBase[curState][g_NoCaseTable[*pName]];
                                if(*pStateBase[curState]<0) terminalTag|=DWORDLONG(0x1<<(-*pStateBase[curState]));
                                ++pName;
                            }else{
                                curState=0;pName+=3;
                            }
                        }//while
                    }else{
                        for(;pName<pNameEnd;++pName){//无须判断是否为汉字 加速搜索
                            if(bCase)
                                curState=pStateBase[curState][*pName];
                            else
                                curState=pStateBase[curState][g_NoCaseTable[*pName]];
                            if(*pStateBase[curState]<0) terminalTag|=DWORDLONG(0x1<<(-*pStateBase[curState]));
                        }//while
                    }
                    if(MAX_DWORDLONG==terminalTag){
                        if(NULL==pIdExt) {
                            if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck)) 
                                g_vFileOutPtr.push_back(*pData); 
                        }
                        else{
                            int iExt=pFile->GetExtendID(pNameEnd);
                            if(iExt!=-1){
                                for(int j=0;j<nExt;++j){
                                    if(pIdExt[j]==iExt){
                                        if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                                            g_vFileOutPtr.push_back(*pData); 
                                        break;
                                    } 
                                }
                            }
                        }
                    }
                }//else 长度不够格                
            }
        } 
        g_vFileIndex.UnLock();  
    }

    if(ppDir) g_vDirIndex.UnLock(); //如过存在目录过滤 需要全部锁


    delete []pStateBase;
    delete []pStateTable;  
}



//使用暴力匹配，仅含普通模式串 含>0x80字符 大小写敏感
void Helper_OnlyNormalSearch(BOOL bCase
                             ,BOOL bDirSearch,BOOL bFileSearch
                             ,int *pIdExt,int nExt
                             ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                             )
{
    if(pIdExt) bDirSearch=FALSE;
    int (*FunCodeToUcs2)(PWCHAR,PBYTE,int);
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;
        FunCodeToUcs2=Helper_CodeToUcs2Case;
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        FunCodeToUcs2=Helper_CodeToUcs2NoCase;
    }


    WCHAR ucs2Name[MAX_PATH];
    int ucs2Len;

    ///搜索开始
    int _nMaxLen=pStrOpt->pNormal->_len;//最大长度，小于此长度的文件跳过 
    ListNormal* pNormal;
    BOOL bSearch;
    PIndexElemType pData,pDataEnd;
    PBYTE pName;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //如过存在目录过滤 需要全部锁

    if(bDirSearch)
    {
        if(!ppDir) g_vDirIndex.Lock();
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();   
        PDIRECTORY_RECORD pDir;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                nameLen=pDir->GetCodeName(pName);
                if(nameLen>=_nMaxLen && pDir->HasNonAscii()){//含非英文 且 编码文件名要比最大的大
                    bSearch=TRUE;
                    ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext)
                    {
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }
                    if(bSearch){
                        if(NULL==ppDir || IsExtnameSat(pDir->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                            g_vDirOutPtr.push_back(*pData); 
                    }
                }                       
            }
        }
        if(!ppDir) g_vDirIndex.UnLock();
    }

    PBYTE pNameEnd;
    if(bFileSearch)
    {
            g_vFileIndex.Lock();
            cBlock=g_vFileIndex.GetBlockCount();
            pIndex=g_vFileIndex.GetBlockIndex();
            PNORMALFILE_RECORD pFile;
            for(int i=0;i<cBlock;++i)
            {
                pNode=pIndex[i];
                pData=pNode->PtrData+pNode->CurrentBegin;  
                pDataEnd=pNode->PtrData+pNode->CurrentEnd;
                for(;pData<pDataEnd;++pData)
                {
                    pFile=PNORMALFILE_RECORD(*pData);
                    nameLen=pFile->GetCodeName(pName);
                    pNameEnd=pName+nameLen;
                    if(nameLen>=_nMaxLen && pFile->HasNonAscii()){
                        bSearch=TRUE;
                        ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                        for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext)
                        {
                            if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                                bSearch=FALSE;break;
                            }
                        }
                        if(bSearch){
                            if(NULL==pIdExt) {
                                if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck)) 
                                    g_vFileOutPtr.push_back(*pData); 
                            }
                            else{
                                int iExt=pFile->GetExtendID(pNameEnd);
                                if(iExt!=-1){
                                    for(int j=0;j<nExt;++j){
                                        if(pIdExt[j]==iExt){
                                            if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                                                g_vFileOutPtr.push_back(*pData); 
                                            break;
                                        } 
                                    }
                                }
                            }
                        }
                    }          
                }
            } 
            g_vFileIndex.UnLock();  
    }
    if(ppDir) g_vDirIndex.UnLock();
}




/**
*	Function:
*      搜索串含有纯?时调用，可能含有普通串、纯?、*?混合
*	Parameter(s):
*      bCase,大小写敏感么?
*	Return:	
*      无
*	Commons:
*      
**/
void Helper_QuestionSearch(BOOL bCase
                           ,BOOL bDirSearch,BOOL bFileSearch
                           ,int *pIdExt,int nExt
                           ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                           )
{
    if(pIdExt) bDirSearch=FALSE;
    int (*FunCodeToUcs2)(PWCHAR,PBYTE,int);
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;
        FunCodeToUcs2=Helper_CodeToUcs2Case;
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        FunCodeToUcs2=Helper_CodeToUcs2NoCase;
    }

    WCHAR ucs2Name[MAX_PATH];
    int ucs2Len;

    ///搜索开始   
    ListQuestion *pQuestion=pStrOpt->pQuestion;
    PWCHAR pQueWch=pQuestion->_wchs;
    int _nLen=pQuestion->_len;//文件长度限制 

    ListNormal          *pNormal;
    ListStar            *pStar;
    ListStarQuestion    *pStarQuestion;

    BOOL bSearch;

    PIndexElemType pData,pDataEnd;
    PBYTE pName;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //如过存在目录过滤 需要全部锁

    if(bDirSearch)
    {
        if(!ppDir) g_vDirIndex.Lock();
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();   
        PDIRECTORY_RECORD pDir;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                nameLen=pDir->GetCodeName(pName);
                ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                if(ucs2Len==_nLen)//长度相等符合资格
                {
                    //先匹配问号串
                    bSearch=TRUE;
                    for(int i=0;i<_nLen;++i){
                        if(L'?'!=pQueWch[i] && pQueWch[i]!=ucs2Name[i]){
                            bSearch=FALSE;break;
                        }
                    }
                    if(bSearch){//成功匹配问号串
                        //匹配普通串
                        for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                            if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//成功匹配普通串
                            //匹配*号串
                            for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                                if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                                    bSearch=FALSE;break;
                                }
                            }  
                            if(bSearch){//成功匹配*号串
                                //匹配?*
                                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                        bSearch=FALSE;break;
                                    }
                                }  
                                if(bSearch){//成功匹配?*
                                    if(NULL==ppDir || IsExtnameSat(pDir->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                                        g_vDirOutPtr.push_back(*pData); 
                                }
                            }    
                        }
                    }               
                }                      
            }
        }
        if(!ppDir) g_vDirIndex.UnLock();
    }


    PBYTE pNameEnd;
    if(bFileSearch)
    {
        g_vFileIndex.Lock();
        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        PNORMALFILE_RECORD pFile;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pFile=PNORMALFILE_RECORD(*pData);
                nameLen=pFile->GetCodeName(pName);
                pNameEnd=pName+nameLen;
                ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                if(ucs2Len==_nLen)//长度相等符合资格
                {
                    //先匹配问号串
                    bSearch=TRUE;
                    for(int i=0;i<_nLen;++i){
                        if(L'?'!=pQueWch[i] && pQueWch[i]!=ucs2Name[i]){
                            bSearch=FALSE;break;
                        }
                    }
                    if(bSearch){//成功匹配问号串
                        //匹配普通串
                        for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                            if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//成功匹配普通串
                            //匹配*号串
                            for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                                if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                                    bSearch=FALSE;break;
                                }
                            }  
                            if(bSearch){//成功匹配*号串
                                //匹配?*
                                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                        bSearch=FALSE;break;
                                    }
                                }  
                                if(bSearch){//成功匹配?*
                                    if(NULL==pIdExt) {
                                        if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck)) 
                                            g_vFileOutPtr.push_back(*pData); 
                                    }
                                    else{
                                        int iExt=pFile->GetExtendID(pNameEnd);
                                        if(iExt!=-1){
                                            for(int j=0;j<nExt;++j){
                                                if(pIdExt[j]==iExt){
                                                    if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                                                        g_vFileOutPtr.push_back(*pData); 
                                                    break;
                                                } 
                                            }
                                        }
                                    }
                                }
                            }    
                        }
                    }               
                }        
            }
        } 
        g_vFileIndex.UnLock();
    }
    if(ppDir) g_vDirIndex.UnLock();
}




/**
*	Function:
*      搜索串含有纯*，但不含有?时调用，可能含有普通串、*?混合
*	Parameter(s):
*      bCase,大小写敏感么?
*	Return:	
*      无
*	Commons:
*      
**/
void Helper_StarSearch(BOOL bCase
                       ,BOOL bDirSearch,BOOL bFileSearch
                       ,int *pIdExt,int nExt
                       ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                       )
{
    if(pIdExt) bDirSearch=FALSE;
    int (*FunCodeToUcs2)(PWCHAR,PBYTE,int);
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;
        FunCodeToUcs2=Helper_CodeToUcs2Case;
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        FunCodeToUcs2=Helper_CodeToUcs2NoCase;
    }

    WCHAR ucs2Name[MAX_PATH];
    int ucs2Len;

    ///搜索开始   
    ListStar            *pStar;
    ListNormal          *pNormal;   
    ListStarQuestion    *pStarQuestion;

    BOOL bSearch;

    PIndexElemType pData,pDataEnd;
    PBYTE pName;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //如过存在目录过滤 需要全部锁

    if(bDirSearch)
    {
        if(!ppDir) g_vDirIndex.Lock();
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();   
        PDIRECTORY_RECORD pDir;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                nameLen=pDir->GetCodeName(pName);
                ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                //先匹配*号串
                bSearch=TRUE;
                for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                    if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//成功匹配*号串
                    //匹配普通串
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//成功匹配普通串
                        //匹配?*
                        for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                            if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//成功匹配?*
                            if(NULL==ppDir || IsExtnameSat(pDir->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                                g_vDirOutPtr.push_back(*pData); 
                        }
                    }    
                }                    
            }
        }
        if(!ppDir) g_vDirIndex.UnLock();
    }


    PBYTE pNameEnd;
    if(bFileSearch)
    {
        g_vFileIndex.Lock();
        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        PNORMALFILE_RECORD pFile;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pFile=PNORMALFILE_RECORD(*pData);
                nameLen=pFile->GetCodeName(pName);
                pNameEnd=pName+nameLen;
                ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                //先匹配*号串
                bSearch=TRUE;
                for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                    if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//成功匹配*号串
                    //匹配普通串
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//成功匹配普通串
                        //匹配?*
                        for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                            if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//成功匹配?*
                            if(NULL==pIdExt) {
                                if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck)) 
                                    g_vFileOutPtr.push_back(*pData); 
                            }
                            else{
                                int iExt=pFile->GetExtendID(pNameEnd);
                                if(iExt!=-1){
                                    for(int j=0;j<nExt;++j){
                                        if(pIdExt[j]==iExt){
                                            if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                                                g_vFileOutPtr.push_back(*pData); 
                                            break;
                                        } 
                                    }
                                }
                            }
                        }
                    }    
                }        
            }
        } 
        g_vFileIndex.UnLock();  
    }
    if(ppDir) g_vDirIndex.UnLock();

}



/**
*	Function:
*      搜索串含有*?混合，但不含有?、纯*时调用，可能含有普通串
*	Parameter(s):
*      bCase,大小写敏感么?
*	Return:	
*      无
*	Commons:
*      
**/
void Helper_StarQuestionSearch(BOOL bCase
                               ,BOOL bDirSearch,BOOL bFileSearch
                               ,int *pIdExt,int nExt
                               ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                               )
{
    if(pIdExt) bDirSearch=FALSE;
    int (*FunCodeToUcs2)(PWCHAR,PBYTE,int);
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;
        FunCodeToUcs2=Helper_CodeToUcs2Case;
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        FunCodeToUcs2=Helper_CodeToUcs2NoCase;
    }

    WCHAR ucs2Name[MAX_PATH];
    int ucs2Len;

    ///搜索开始   
    ListNormal          *pNormal;   
    ListStarQuestion    *pStarQuestion;

    BOOL bSearch;

    PIndexElemType pData,pDataEnd;
    PBYTE pName;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //如过存在目录过滤 需要全部锁

    if(bDirSearch)
    {
        if(!ppDir) g_vDirIndex.Lock();
        cBlock=g_vDirIndex.GetBlockCount();
        pIndex=g_vDirIndex.GetBlockIndex();   
        PDIRECTORY_RECORD pDir;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pDir=PDIRECTORY_RECORD(*pData);
                nameLen=pDir->GetCodeName(pName);
                ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                //先匹配?*号串
                bSearch=TRUE;
                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//成功匹配?*号串
                    //匹配普通串
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//成功匹配普通串
                        if(NULL==ppDir || IsExtnameSat(pDir->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                            g_vDirOutPtr.push_back(*pData); 
                    }    
                }                  
            }
        }
        if(!ppDir) g_vDirIndex.UnLock();
    }

    PBYTE pNameEnd;
    if(bFileSearch)
    {
        g_vFileIndex.Lock();
        cBlock=g_vFileIndex.GetBlockCount();
        pIndex=g_vFileIndex.GetBlockIndex();
        PNORMALFILE_RECORD pFile;
        for(int i=0;i<cBlock;++i)
        {
            pNode=pIndex[i];
            pData=pNode->PtrData+pNode->CurrentBegin;  
            pDataEnd=pNode->PtrData+pNode->CurrentEnd;
            for(;pData<pDataEnd;++pData)
            {
                pFile=PNORMALFILE_RECORD(*pData);
                nameLen=pFile->GetCodeName(pName);
                pNameEnd=pName+nameLen;
                ucs2Len=FunCodeToUcs2(ucs2Name,pName,nameLen);
                //先匹配?*号串
                bSearch=TRUE;
                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//成功匹配?*号串
                    //匹配普通串
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//成功匹配普通串
                        if(NULL==pIdExt) {
                            if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck)) 
                                g_vFileOutPtr.push_back(*pData); 
                        }
                        else{
                            int iExt=pFile->GetExtendID(pNameEnd);
                            if(iExt!=-1){
                                for(int j=0;j<nExt;++j){
                                    if(pIdExt[j]==iExt){
                                        if(NULL==ppDir || IsExtnameSat(pFile->ParentPtr,ppDir,pbSubDir,nCheck) ) 
                                            g_vFileOutPtr.push_back(*pData); 
                                        break;
                                    } 
                                }
                            }
                        }
                    }    
                }        
            }
        } 
        g_vFileIndex.UnLock();  
    }
    if(ppDir) g_vDirIndex.UnLock();
}