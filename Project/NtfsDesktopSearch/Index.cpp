// Index.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "Index.h"

CIndex g_vDirIndex,g_vFileIndex;
static const DWORD INDEX_COUNT_DELT=32; //用于扩展索引数组空间，当结点较小时，它应较大

CIndex::CIndex(int _N){
    m_dwMaxCount=_N;
    m_pIndex=(PINDEX_BLOCK_NODE*)g_MemoryMgr.malloc(m_dwMaxCount*sizeof(PINDEX_BLOCK_NODE));
    m_dwBlockCount=0;
    m_pLastNode=m_pIndex[m_dwBlockCount++]=(PINDEX_BLOCK_NODE)g_MemoryMgr.GetMemory(sizeof(INDEX_BLOCK_NODE));
    m_pLastNode->Init();
    m_size=0;
    InitializeCriticalSection(&m_cs);
}

CIndex::~CIndex(){
    DeleteCriticalSection(&m_cs);
    for(int i=0;i<m_dwBlockCount;++i){
        g_MemoryMgr.FreeMemory((PBYTE)m_pIndex[i]);
    }
    g_MemoryMgr.free(m_pIndex);        
}
void CIndex::Lock(){
    EnterCriticalSection(&m_cs);
}
void CIndex::UnLock(){
    LeaveCriticalSection(&m_cs);
}

void  CIndex::DecreaseSize(){--m_size;}
DWORD CIndex::size()const{return m_size;}
void CIndex::push_back(IndexElemType RecordOffset){
    ++m_size;
    if(m_pLastNode->IsEndFull()){
        AddLastBlock();
    }
    m_pLastNode->AddEnd(RecordOffset);
}

//按文件名顺序插入
void CIndex::insert(IndexElemType Record,BOOL bDir)
{
    DebugTrace("**********插入记录\n");
    int (*FunCmpare)(IndexElemType,IndexElemType);
    if(bDir) FunCmpare=comp_dir;
    else FunCmpare=comp_file;
    ++m_size;
    int cmp;
    PINDEX_BLOCK_NODE pNode;

    int i;
    for(i=0;i<m_dwBlockCount;++i)
    {
        pNode=m_pIndex[i];
        cmp=FunCmpare(Record,pNode->PtrData[pNode->CurrentEnd-1]);
        if(cmp<=0) break;
    }
    if(i==m_dwBlockCount){//
        DebugTrace("当前插入的是最大文件目录名\n");
        //看最后节点是否有空间插入，如果没有，则新增加节点
        //满时：m_pLastNode->CurrentBegin==0 m_pLastNode->CurrentEnd==DirFrnIndexBlockNode::CountPerIndex
        //[CurrentBegin,CurrentEnd)
        if(m_pLastNode->IsEndFull()){//右满
            DebugTrace("右满\n");
            if(m_pLastNode->IsBeginFull()){//左满
                DebugTrace("左满\n");
                //先向左借空间
                if(m_dwBlockCount>=2 && !m_pIndex[m_dwBlockCount-2]->IsFull()){//左有借
                    DebugTrace("左有借\n");
                    pNode=m_pIndex[m_dwBlockCount-2];
                    if(pNode->IsEndFull()){//右满
                        DebugTrace("左右端满，左移前Beg=%d,End=%d\n",pNode->CurrentBegin,pNode->CurrentEnd);
                        pNode->LeftMove();
                        DebugTrace("左右端满，左移后Beg=%d,End=%d\n",pNode->CurrentBegin,pNode->CurrentEnd);
                    }
                    DebugTrace("最后右移动前，【Beg1=%d,End1=%d】【Beg2=%d,End2=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    pNode->AddEnd(m_pLastNode->RemoveBegin());//最后BEG右移
                    DebugTrace("最后右移动后，【Beg1=%d,End1=%d】【Beg2=%d,End2=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    m_pLastNode->LeftMove();//右移动一格
                    DebugTrace("左移动一格，【Beg=%d,End=%d】\n",m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                }else{//左无借，将最后一个节点分裂保留3/4 1/4进新节点
                    DebugTrace("左无借【Beg=%d,End=%d】\n",m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode已指向新块
                    DebugTrace("增加新块，【Beg1=%d,End1=%d】【Beg2=%d,End2=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    //分裂pNode
                    m_pLastNode->CutFromLeft(pNode);
                    DebugTrace("左剪切，【Beg1=%d,End1=%d】【Beg2=%d,End2=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);         
                }
                DebugTrace("AddEnd【Beg=%d,End=%d】\n",m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                m_pLastNode->AddEnd(Record);
            }
            else{//左未满
                DebugTrace("左未满\n");
                //整体左移动一格
                m_pLastNode->LeftMove();
                m_pLastNode->AddEnd(Record);
            }
        }else{//右未满
            DebugTrace("右未满\n");
            m_pLastNode->AddEnd(Record);
        }           
    }else{//插入的值非最大值
        //pNode指向插入节点
        //寻找插入位置先
        
        int low=pNode->CurrentBegin,high=pNode->CurrentEnd-1;
        int mid;  
        DWORD dwInsert;
        while(low<=high)
        {
            mid=low+((high-low)>>1);
            cmp=FunCmpare(Record,pNode->PtrData[mid]);
            if(cmp<0) high=mid-1;
            else if(cmp>0) low=mid+1;
            else{//找到同名
                dwInsert=mid;
                break;
            }
        }
        if(low>high) dwInsert=low;   
        DebugTrace("寻找插入位置先【Beg=%d,dwInsert=%d,End=%d】\n",pNode->CurrentBegin,dwInsert,pNode->CurrentEnd);
        assert(dwInsert>=pNode->CurrentBegin && dwInsert<pNode->CurrentEnd);
        //插入位置 注意[dwInsert ,CurEnd) 应向右移动
        if(pNode->IsFull()){
            DebugTrace("插入节点满\n");
            if(pNode==m_pLastNode){//在最后一个节点中插入
                DebugTrace("是在最后一个节点中插入\n");
                if(i>=1 && !m_pIndex[i-1]->IsFull()){//左有借
                    DebugTrace("左有借\n");
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//右满
                        DebugTrace("左节点 右满【Beg=%d,End=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd);
                        pNode->LeftMove();
                        DebugTrace("左移动 右满【Beg=%d,End=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd);
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode的所有值
                        DebugTrace("直接插入左最后\n");
                        pNode->AddEnd(Record);
                        DebugTrace("【Beg=%d,End=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd);
                        return;
                    }else pNode->AddEnd(m_pLastNode->RemoveBegin());//最后BEG右移
                    m_pLastNode->Insert(dwInsert,Record);
                    DebugTrace("最后右移动后，【Beg1=%d,End1=%d】【Beg2=%d,End2=%d】\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                }else{//左无借，将最后一个节点分裂保留3/4 1/4进新节点
                    DebugTrace("左无借，分增加最后节点\n");
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode已指向新块
                    //分裂pNode
                    m_pLastNode->CutFromLeft(pNode,&dwInsert);
                    pNode->AddEnd(Record);
                }
            }else if(pNode==*m_pIndex){//在第一个节点中插入
                DebugTrace("在第一个节点中插入\n");
                if(m_dwBlockCount>1 && !m_pIndex[1]->IsFull()){//右有借
                    DebugTrace("右有借\n");
                    pNode=m_pIndex[1];
                    if(pNode->IsBeginFull()){//左满
                        DebugTrace("左满则右移动\n");
                        pNode->RightMove();
                    }
                    //*m_pIndex的最后一个值一定大于插入指
                    pNode->AddBegin((*m_pIndex)->RemoveEnd());
                    (*m_pIndex)->Insert(dwInsert,Record);
                }else{//右无借
                    DebugTrace("右无借\n");
                    InsertBlock(1);
                    pNode=m_pIndex[1];
                    //分裂pNode
                    pNode->CutFromLeft(*m_pIndex,&dwInsert);
                    (*m_pIndex)->AddEnd(Record);
                }              
            }else{//即不是第一个节点 又不是最后节点插入 可能要向两边借
                DebugTrace("向两边借\n");
                if(!m_pIndex[i-1]->IsFull()){//试图向左借
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//右满
                        pNode->LeftMove();
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode的所有值
                        pNode->AddEnd(Record);
                        return;
                    }else pNode->AddEnd(m_pIndex[i]->RemoveBegin());//最后BEG右移
                    m_pIndex[i]->Insert(dwInsert,Record);
                }else if(!m_pIndex[i+1]->IsFull()){//试图向右借
                    pNode=m_pIndex[i+1];
                    if(pNode->IsBeginFull()){//左满
                        pNode->RightMove();
                    }
                    //*m_pIndex的最后一个值一定大于插入指
                    pNode->AddBegin(m_pIndex[i]->RemoveEnd());
                    m_pIndex[i]->Insert(dwInsert,Record);
                }else{//左右均无借的
                    //插入新节点，然后向两边借，本处仅实现的拷贝一边的
                    //优化：可以拷贝两边的数据，使得连续3个节点都未满
                    if(dwInsert<(INDEX_BLOCK_NODE::CountPerIndex>>1)){ 
                        //插入位置在前部 拷贝大块到令一块中央
                        InsertBlock(i+1);
                        m_pIndex[i+1]->CutFromLeft(m_pIndex[i],&dwInsert);
                        m_pIndex[i]->AddEnd(Record);  
                    }else{
                        InsertBlock(i);
                        m_pIndex[i]->CutFromRight(m_pIndex[i+1],&dwInsert);
                        m_pIndex[i+1]->AddBegin(Record);          
                    }
                }
            }
        }else{//本节点有位置可插
            DebugTrace("插入节点未满\n");
            pNode->Insert(dwInsert,Record);
            DebugTrace("寻找插入位置先【Beg=%d,dwInsert=%d,End=%d】\n",pNode->CurrentBegin,dwInsert,pNode->CurrentEnd);
        }
    }   
    DebugTrace("**********插入完成===============\n");
}

//删除的时候由于会出现重名现象，必须提供文件BasicInfo
//当节点为空时，释放空节点块，这样表中没有空节点块
void CIndex::erase(IndexElemType Record,BOOL bDir)
{
    DebugTrace("erase1\n");
    int (*FunCmpare)(IndexElemType,IndexElemType);
    if(bDir) FunCmpare=comp_dir;
    else FunCmpare=comp_file;

    --m_size;
    int cmp;
    PINDEX_BLOCK_NODE pNode;
    DWORD BasicInfo=*(DWORD*)Record;

    int i;
    for(i=0;i<m_dwBlockCount;++i)
    {
        pNode=m_pIndex[i];
        cmp=FunCmpare(Record,pNode->PtrData[pNode->CurrentEnd-1]);
        if(cmp<=0) break;
    }
    assert(i<m_dwBlockCount && "文件名一定在库中存在");
    //if(i==m_dwBlockCount) return;//文件名不在库中

    //pNode指向插入节点
    //寻找插入位置先
    int low=pNode->CurrentBegin,high=pNode->CurrentEnd-1;
    int mid;  
    while(low<=high)
    {
        mid=low+((high-low)>>1);
        cmp=FunCmpare(Record,pNode->PtrData[mid]);
        if(cmp<0) high=mid-1;
        else if(cmp>0) low=mid+1;
        else{//找到同名
            break;
        }
    }
    assert(low<=high && "文件名一定在库中存在");
    //if(low>high) return;//文件名不在库中

    DebugTrace("erase2\n");
    //mid处为文件名处，但可能有重名，
    //应进一步向两边检查BasicInfo 向两边访问的过程中可能需要跨块
    if(*(DWORD*)(pNode->PtrData[mid])==BasicInfo){//多数情况下无重复文件名情况
        DebugTrace("erase3 i=%d beg=%d mid=%d end=%d\n",i,pNode->CurrentBegin,mid,pNode->CurrentEnd);
        assert(m_pIndex[i]==pNode);
        pNode->Erase(mid);//删除第mid个值
        DebugTrace("erase3.5 i=%d beg=%d mid=%d end=%d\n",i,pNode->CurrentBegin,mid,pNode->CurrentEnd);
        if(pNode->IsEmpty()) EraseBlock(i);//删除当前块
        DebugTrace("erase4\n");
    }
    else{//文件名相同，但当前位置mid不是所在文件
        DebugTrace("erase5\n");
        int j=i;
        PIndexElemType p
            ,pBeg//[
            ,pEnd;//)
        BOOL bFail=FALSE;
        assert(pNode==m_pIndex[i]);
        int left=pNode->CurrentBegin,right=mid-1;
        DebugTrace("erase 向左找\n");
        for(j=i;;)
        {
            p=pNode->PtrData+right;
            pBeg=pNode->PtrData+left;
            for(;p>=pBeg;--p)
            {
                cmp=FunCmpare(Record,*p);
                if(0==cmp){
                    if(*(DWORD*)*p==BasicInfo){
                        pNode->Erase(pNode->CurrentBegin+(p-pBeg));//删除
                        if(pNode->IsEmpty()) EraseBlock(j);//删除当前块 
                        return;
                    }
                }else{//没有找到
                    bFail=TRUE;
                    break;
                }                  
            }
            if(bFail) break;
            else{
                --j;
                if(j>=0){
                    pNode=m_pIndex[j];
                    left=pNode->CurrentBegin;
                    right=pNode->CurrentEnd-1;
                }else break;
            }
        }

        //左边没有找到，右边看看
        DebugTrace("erase 向右找\n");
        pNode=m_pIndex[i];
        left=mid+1,right=pNode->CurrentEnd;
        for(j=i;;)
        {
            p=pNode->PtrData+left;
            pEnd=pNode->PtrData+right;
            for(;p<pEnd;++p)
            {
                cmp=FunCmpare(Record,*p);
                if(0==cmp){
                    if(*(DWORD*)*p==BasicInfo){
                        pNode->Erase(pNode->CurrentEnd-(pEnd-p));//删除
                        if(pNode->IsEmpty()) EraseBlock(j);//删除当前块 
                        return;
                    }
                }else{//没有找到
                    assert(0 && "遇到某不等，竟然找不到?");
                    return;
                }                  
            }
            ++j;
            if(j<m_dwBlockCount){
                pNode=m_pIndex[j];
                left=pNode->CurrentBegin;
                right=pNode->CurrentEnd;               
            }else{
                assert(0 && "找完了，竟然找不到?");
                return;
            }
        }
    }
}


//用于顺序遍历，让用户管理提高速度
PINDEX_BLOCK_NODE *CIndex::GetBlockIndex()const{return m_pIndex;}
int CIndex::GetBlockCount()const{return m_dwBlockCount;}

void CIndex::AddLastBlock()
{
    m_pLastNode=m_pIndex[m_dwBlockCount]=(PINDEX_BLOCK_NODE)g_MemoryMgr.GetMemory(sizeof(INDEX_BLOCK_NODE));
    m_pLastNode->Init();
    ++m_dwBlockCount;
    if(m_dwBlockCount==m_dwMaxCount){//索引BUF已满，增加之
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PINDEX_BLOCK_NODE*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PINDEX_BLOCK_NODE));                
    }
}
void CIndex::InsertBlock(int i)//在第i处插入新块
{
    for(int j=m_dwBlockCount++;j>i;--j) m_pIndex[j]=m_pIndex[j-1];
    m_pIndex[i]=(PINDEX_BLOCK_NODE)g_MemoryMgr.GetMemory(sizeof(INDEX_BLOCK_NODE));
    m_pIndex[i]->Init();
    if(m_dwBlockCount==m_dwMaxCount){//索引BUF已满，增加之
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PINDEX_BLOCK_NODE*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PINDEX_BLOCK_NODE));                
    }
}

void CIndex::EraseBlock(int i)//考虑到空节点影响访问效率 删除第i块空节点
{
    assert(i>=0 && i<m_dwBlockCount);
    PINDEX_BLOCK_NODE pNode=m_pIndex[i];
    //[i ... m_dwBlockCount-1]
    --m_dwBlockCount;
    for(;i<m_dwBlockCount;++i) m_pIndex[i]=m_pIndex[i+1];
    g_MemoryMgr.FreeMemory((PBYTE)pNode);
}

