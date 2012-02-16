// BasicSearch.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "StrMatch.h"

const int ALPHABETA_SIZE=0x80;
const short FAIL_STATE=0x8000; //-32768 ����AC�㷨
BYTE g_NoCaseTable[ALPHABETA_SIZE]; //����ʱ�Ѿ���ʼ��

void Helper_InitNoCaseTable()
{
    for(int i=0;i<ALPHABETA_SIZE;++i){
        if(i>='A' && i<='Z') g_NoCaseTable[i]=i+32;
        else g_NoCaseTable[i]=i;
    }
}

//pParentDir�Ǵ������ļ���ֱ�Ӹ�ָ��
__forceinline BOOL IsExtnameSat(PDIRECTORY_RECORD pParentDir,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck)//��չ������ô
{
    PDIRECTORY_RECORD p;
    for(int i=0;i<nCheck;++i)
    {
        p=pParentDir;
        if(pbSubDir[i]){//����Ŀ¼
            for(;p;p=p->ParentPtr)
            {
                if(ppDir[i]==p) return TRUE;
            }
        }else{//��������Ŀ¼
            if(ppDir[i]==p) return TRUE;
        }
    }
    return FALSE;
}


//�����ļ���ʹ��AC�㷨�ж�<0x80ת����
//�ռ��Ż�9~0x31������
const DWORDLONG MAX_DWORDLONG=0xffffffffffffffff;
void Helper_OnlyNormalSearchAscii(BOOL bCase
                                  ,BOOL bDirSearch,BOOL bFileSearch
                                  ,int *pIdExt,int nExt
                                  ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                                  )
{
    if(pIdExt) bDirSearch=FALSE;//���������Ч��չ�� ������Ŀ¼

    SearchStrOpt *pStrOpt;
    if(bCase) pStrOpt=&g_StrOptCase;
    else pStrOpt=&g_StrOptNoCase;

    ListNormal *pNormal;
    int _nMaxStates=1;//���״̬��������һ����̬
    DWORDLONG nTerminalTag=MAX_DWORDLONG;
    short iFinalState=-1;
    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){
        _nMaxStates+=pNormal->_len;
        nTerminalTag&=~(0x1<<(-iFinalState--));
    }

    //��ֹ״̬��־<0 [-1 -2 -3]
    short *pStateTable=new short[_nMaxStates*ALPHABETA_SIZE];
    short **pStateBase =new short*[_nMaxStates];
    //pStateBase[i][0]��ʾ�Ƿ�Ϊ��̬��ʶ pStateBase[i][1]��ʾfail����
    for(int i=0;i<_nMaxStates;++i) pStateBase[i]=pStateTable+i*ALPHABETA_SIZE;

    //��ʼ������TO״̬ΪʧЧ״̬
    for(int i=0;i<_nMaxStates;++i){
        *pStateBase[i]=0;//<0ʱ��ʾ��ֹ״̬
        for(int j=32;j<ALPHABETA_SIZE;++j) pStateBase[i][j]=FAIL_STATE;
    }
    PWCHAR pWch,pWchEnd;  
    short state,nextState;//״̬����
    short nTotalState=0;//��״̬��
    iFinalState=-1;
    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){//����ģʽ��ACͼ
        state=0;
        pWch=pNormal->_wchs;pWchEnd=pNormal->_wchs+pNormal->_len;
        for(;pWch<pWchEnd;pWch++){//���ǰ׺ƥ��
            nextState=pStateBase[state][*pWch];
            if(FAIL_STATE==nextState) break;
            state=nextState;
        }
        //�������
        for(;pWch<pWchEnd;pWch++){//���ǰ׺ƥ��
            pStateBase[state][*pWch]=++nTotalState;
            state=nTotalState;
        }
        *pStateBase[state]=iFinalState--;//������̬
    }


    for(int i=32;i<ALPHABETA_SIZE;++i){//���ó�̬
        if(FAIL_STATE==pStateBase[0][i]) pStateBase[0][i]=0;
    }
    //for all i ,pStateBase[0][i]>=0

    //����fail��������α���
    Queue<short> queue;
    for(int i=32;i<ALPHABETA_SIZE;++i){
        state=pStateBase[0][i];
        if(state!=0){
            pStateBase[state][1]=0;//��һ��Ԫ�ر�ʾʧЧTO
            queue.Add(state);            
        }
    }
    while(!queue.IsEmpty()){
        queue.Delete(state);//state��ʧЧ����������
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
                pStateBase[state][i]=pStateBase[pStateBase[state][1]][i];//�߼�AC ����ʱ������ʧЧ ˳������
            }
        }
    }

    ///������ʼ
    int _nMaxLen=pStrOpt->pNormal->_len;//��󳤶ȣ�С�ڴ˳��ȵ��ļ����� 
    short curState;
    DWORDLONG terminalTag;
    PIndexElemType pData,pDataEnd;
    PBYTE pName,pNameEnd;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //�������Ŀ¼���� ��Ҫȫ����

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
                    if(pDir->HasNonAscii()){//�ļ����д��ڷ�ASCII
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
                        for(;pName<pNameEnd;++pName){//�����ж��Ƿ�Ϊ���� ��������
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
                }//else ���Ȳ�����                        
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
                    if(pFile->HasNonAscii()){//�ļ����д��ڷ�ASCII
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
                        for(;pName<pNameEnd;++pName){//�����ж��Ƿ�Ϊ���� ��������
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
                }//else ���Ȳ�����                
            }
        } 
        g_vFileIndex.UnLock();  
    }

    if(ppDir) g_vDirIndex.UnLock(); //�������Ŀ¼���� ��Ҫȫ����


    delete []pStateBase;
    delete []pStateTable;  
}



//ʹ�ñ���ƥ�䣬������ͨģʽ�� ��>0x80�ַ� ��Сд����
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

    ///������ʼ
    int _nMaxLen=pStrOpt->pNormal->_len;//��󳤶ȣ�С�ڴ˳��ȵ��ļ����� 
    ListNormal* pNormal;
    BOOL bSearch;
    PIndexElemType pData,pDataEnd;
    PBYTE pName;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //�������Ŀ¼���� ��Ҫȫ����

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
                if(nameLen>=_nMaxLen && pDir->HasNonAscii()){//����Ӣ�� �� �����ļ���Ҫ�����Ĵ�
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
*      ���������д�?ʱ���ã����ܺ�����ͨ������?��*?���
*	Parameter(s):
*      bCase,��Сд����ô?
*	Return:	
*      ��
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

    ///������ʼ   
    ListQuestion *pQuestion=pStrOpt->pQuestion;
    PWCHAR pQueWch=pQuestion->_wchs;
    int _nLen=pQuestion->_len;//�ļ��������� 

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

    if(ppDir) g_vDirIndex.Lock(); //�������Ŀ¼���� ��Ҫȫ����

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
                if(ucs2Len==_nLen)//������ȷ����ʸ�
                {
                    //��ƥ���ʺŴ�
                    bSearch=TRUE;
                    for(int i=0;i<_nLen;++i){
                        if(L'?'!=pQueWch[i] && pQueWch[i]!=ucs2Name[i]){
                            bSearch=FALSE;break;
                        }
                    }
                    if(bSearch){//�ɹ�ƥ���ʺŴ�
                        //ƥ����ͨ��
                        for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                            if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//�ɹ�ƥ����ͨ��
                            //ƥ��*�Ŵ�
                            for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                                if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                                    bSearch=FALSE;break;
                                }
                            }  
                            if(bSearch){//�ɹ�ƥ��*�Ŵ�
                                //ƥ��?*
                                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                        bSearch=FALSE;break;
                                    }
                                }  
                                if(bSearch){//�ɹ�ƥ��?*
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
                if(ucs2Len==_nLen)//������ȷ����ʸ�
                {
                    //��ƥ���ʺŴ�
                    bSearch=TRUE;
                    for(int i=0;i<_nLen;++i){
                        if(L'?'!=pQueWch[i] && pQueWch[i]!=ucs2Name[i]){
                            bSearch=FALSE;break;
                        }
                    }
                    if(bSearch){//�ɹ�ƥ���ʺŴ�
                        //ƥ����ͨ��
                        for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                            if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//�ɹ�ƥ����ͨ��
                            //ƥ��*�Ŵ�
                            for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                                if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                                    bSearch=FALSE;break;
                                }
                            }  
                            if(bSearch){//�ɹ�ƥ��*�Ŵ�
                                //ƥ��?*
                                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                        bSearch=FALSE;break;
                                    }
                                }  
                                if(bSearch){//�ɹ�ƥ��?*
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
*      ���������д�*����������?ʱ���ã����ܺ�����ͨ����*?���
*	Parameter(s):
*      bCase,��Сд����ô?
*	Return:	
*      ��
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

    ///������ʼ   
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

    if(ppDir) g_vDirIndex.Lock(); //�������Ŀ¼���� ��Ҫȫ����

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
                //��ƥ��*�Ŵ�
                bSearch=TRUE;
                for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                    if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//�ɹ�ƥ��*�Ŵ�
                    //ƥ����ͨ��
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//�ɹ�ƥ����ͨ��
                        //ƥ��?*
                        for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                            if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//�ɹ�ƥ��?*
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
                //��ƥ��*�Ŵ�
                bSearch=TRUE;
                for(pStar=pStrOpt->pStar;pStar;pStar=pStar->_pNext){ 
                    if(!pStar->StarSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//�ɹ�ƥ��*�Ŵ�
                    //ƥ����ͨ��
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//�ɹ�ƥ����ͨ��
                        //ƥ��?*
                        for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                            if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                                bSearch=FALSE;break;
                            }
                        }  
                        if(bSearch){//�ɹ�ƥ��?*
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
*      ����������*?��ϣ���������?����*ʱ���ã����ܺ�����ͨ��
*	Parameter(s):
*      bCase,��Сд����ô?
*	Return:	
*      ��
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

    ///������ʼ   
    ListNormal          *pNormal;   
    ListStarQuestion    *pStarQuestion;

    BOOL bSearch;

    PIndexElemType pData,pDataEnd;
    PBYTE pName;
    int nameLen;

    PINDEX_BLOCK_NODE* pIndex;
    PINDEX_BLOCK_NODE   pNode;
    int cBlock;

    if(ppDir) g_vDirIndex.Lock(); //�������Ŀ¼���� ��Ҫȫ����

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
                //��ƥ��?*�Ŵ�
                bSearch=TRUE;
                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//�ɹ�ƥ��?*�Ŵ�
                    //ƥ����ͨ��
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//�ɹ�ƥ����ͨ��
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
                //��ƥ��?*�Ŵ�
                bSearch=TRUE;
                for(pStarQuestion=pStrOpt->pStarQuestion;pStarQuestion;pStarQuestion=pStarQuestion->_pNext){ 
                    if(!pStarQuestion->StarQuestionSubStringMatch(ucs2Name,ucs2Len)){
                        bSearch=FALSE;break;
                    }
                }  
                if(bSearch){//�ɹ�ƥ��?*�Ŵ�
                    //ƥ����ͨ��
                    for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext){ 
                        if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                            bSearch=FALSE;break;
                        }
                    }  
                    if(bSearch){//�ɹ�ƥ����ͨ��
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