// DirBasicInfoMap.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "DirBasicInfoMap.h"

CDirBasicInfoMap g_DirMap;

#define make_map(BasicInfo,DirPtr) (DWORDLONG((BasicInfo)|(DWORDLONG(DirPtr)<<32)))

static const DWORD INDEX_COUNT_DELT=32; //用于扩展索引数组空间，当结点较小时，它应较大

CDirBasicInfoMap::CDirBasicInfoMap(int _N){
    m_dwMaxCount=_N;
    m_pIndex=(PDirFrnIndexBlockNode*)g_MemoryMgr.malloc(m_dwMaxCount*sizeof(PDirFrnIndexBlockNode));
    m_dwBlockCount=0;
    m_pLastNode=m_pIndex[m_dwBlockCount++]=(PDirFrnIndexBlockNode)g_MemoryMgr.GetMemory(sizeof(DirFrnIndexBlockNode));
    m_pLastNode->Init();

    m_size=0;
}
CDirBasicInfoMap::~CDirBasicInfoMap(){
    for(int i=0;i<m_dwBlockCount;++i){
        g_MemoryMgr.FreeMemory((PBYTE)m_pIndex[i]);
    }
    g_MemoryMgr.free(m_pIndex);        
}

//仅在初始化时使用，以遍提高效率
void CDirBasicInfoMap::push_back(DWORD BasicInfo,IndexElemType DirPtr)
{       
    ++m_size;
    DWORDLONG _val=make_map(BasicInfo,DirPtr);
    if(m_pLastNode->IsEndFull()){
        AddLastBlock();
    }
    m_pLastNode->AddEnd(_val);
} 

//可以不提供删除节点功能
//没有删除的信息也不会需要被查找
//若新增的利用的先前的FRN，则
//因为插入是如果遇到重复值遍对DirOffset进行了修改 

//返回被删除的指针
IndexElemType CDirBasicInfoMap::erase(DWORD BasicInfo)//删除节点
{
    --m_size;
    PIndexElemType pMapElem=(PIndexElemType)find(BasicInfo);
    assert(pMapElem && "一定存在");
    ++pMapElem;
    IndexElemType tmp=*pMapElem;
    *pMapElem=NULL;//直接将值设为NULL
    return tmp;
}

void CDirBasicInfoMap::insert(DWORD BasicInfo,IndexElemType DirPtr)
{
    ++m_size;
    assert(DirPtr);
    DWORDLONG dd_map_val=make_map(BasicInfo,DirPtr);
    //找到节点所在的位置，可能需要分裂节点
    PDirFrnIndexBlockNode pNode;
    DWORD info;
    //找到所在块
    int i;
    for(i=0;i<m_dwBlockCount;++i)
    {
        pNode=m_pIndex[i];
        info=pNode->PtrData[pNode->CurrentEnd-1];
        if(BasicInfo<info){
            break;
        }else if(BasicInfo==info){
            pNode->PtrData[pNode->CurrentEnd-1]=dd_map_val;
            return;
        }
        //BasicInfo>info  continue;
    }
    if(i==m_dwBlockCount){//当前插入的是最大FRN
        //看最后节点是否有空间插入，如果没有，则新增加节点
        //满时：m_pLastNode->CurrentBegin==0 m_pLastNode->CurrentEnd==DirFrnIndexBlockNode::CountPerIndex
        //[CurrentBegin,CurrentEnd)
        if(m_pLastNode->IsEndFull()){//右满
            if(m_pLastNode->IsBeginFull()){//左满
                //先向左借空间
                if(m_dwBlockCount>=2 && !m_pIndex[m_dwBlockCount-2]->IsFull()){//左有借
                    pNode=m_pIndex[m_dwBlockCount-2];
                    if(pNode->IsEndFull()){//右满
                        pNode->LeftMove();
                    }
                    pNode->AddEnd(m_pLastNode->RemoveBegin());//最后BEG右移
                    m_pLastNode->LeftMove();//右移动一格
                    m_pLastNode->AddEnd(dd_map_val);
                }else{//左无借，将最后一个节点分裂保留3/4 1/4进新节点
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode已指向新块
                    //分裂pNode
                    m_pLastNode->CutFromLeft(pNode);
                    m_pLastNode->AddEnd(dd_map_val);
                }
            }
            else{//左未满
                //整体左移动一格
                m_pLastNode->LeftMove();
                m_pLastNode->AddEnd(dd_map_val);
            }
        }else{//右未满
            m_pLastNode->AddEnd(dd_map_val);
        }           
    }else{//插入的值非最大值
        //pNode指向插入节点
        //寻找插入位置先
        int low=pNode->CurrentBegin,high=pNode->CurrentEnd-1;
        int mid;         
        while(low<=high)
        {
            mid=low+((high-low)>>1);
            info=pNode->PtrData[mid];
            if(BasicInfo<info){
                high=mid-1;
            }else if(BasicInfo>info){
                low=mid+1;
            }else{
                pNode->PtrData[mid]=dd_map_val;
                return;
            }
        }
        assert(low>=pNode->CurrentBegin);
        DWORD dwInsert=low;//插入位置 注意[low ,CurEnd) 应向右移动
        if(pNode->IsFull()){
            if(pNode==m_pLastNode){//在最后一个节点中插入
                if(i>=1 && !m_pIndex[i-1]->IsFull()){//左有借
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//右满
                        pNode->LeftMove();
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode的所有值
                        pNode->AddEnd(dd_map_val);
                        return;
                    }else pNode->AddEnd(m_pLastNode->RemoveBegin());//最后BEG右移
                    m_pLastNode->Insert(dwInsert,dd_map_val);
                }else{//左无借，将最后一个节点分裂保留3/4 1/4进新节点
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode已指向新块
                    //分裂pNode
                    m_pLastNode->CutFromLeft(pNode,&dwInsert);
                    pNode->AddEnd(dd_map_val);
                }
            }else if(pNode==*m_pIndex){//在第一个节点中插入
                if(m_dwBlockCount>1 && !m_pIndex[1]->IsFull()){//右有借
                    pNode=m_pIndex[1];
                    if(pNode->IsBeginFull()){//左满
                        pNode->RightMove();
                    }
                    //*m_pIndex的最后一个值一定大于插入指
                    pNode->AddBegin((*m_pIndex)->RemoveEnd());
                    (*m_pIndex)->Insert(dwInsert,dd_map_val);
                }else{//右无借
                    InsertBlock(1);
                    pNode=m_pIndex[1];
                    //分裂pNode
                    pNode->CutFromLeft(*m_pIndex,&dwInsert);
                    (*m_pIndex)->AddEnd(dd_map_val);
                }              
            }else{//即不是第一个节点 又不是最后节点插入 可能要向两边借
                if(!m_pIndex[i-1]->IsFull()){//试图向左借
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//右满
                        pNode->LeftMove();
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode的所有值
                        pNode->AddEnd(dd_map_val);
                        return;
                    }else pNode->AddEnd(m_pIndex[i]->RemoveBegin());//最后BEG右移
                    m_pIndex[i]->Insert(dwInsert,dd_map_val);
                }else if(!m_pIndex[i+1]->IsFull()){//试图向右借
                    pNode=m_pIndex[i+1];
                    if(pNode->IsBeginFull()){//左满
                        pNode->RightMove();
                    }
                    //*m_pIndex的最后一个值一定大于插入指
                    pNode->AddBegin(m_pIndex[i]->RemoveEnd());
                    m_pIndex[i]->Insert(dwInsert,dd_map_val);
                }else{//左右均无借的
                    if(dwInsert<(DirFrnIndexBlockNode::CountPerIndex>>1)){
                        //插入位置在前部 拷贝大块到令一块中央
                        InsertBlock(i+1);
                        m_pIndex[i+1]->CutFromLeft(m_pIndex[i],&dwInsert);
                        m_pIndex[i]->AddEnd(dd_map_val);  
                    }else{
                        InsertBlock(i);
                        m_pIndex[i]->CutFromRight(m_pIndex[i+1],&dwInsert);
                        m_pIndex[i+1]->AddBegin(dd_map_val);          
                    }
                }
            }
        }else{//本节点有位置可插
            pNode->Insert(dwInsert,dd_map_val);
        }
    }    
}

//返回偏移值
DWORDLONG* CDirBasicInfoMap::find(DWORD BasicInfo)
{
    PDirFrnIndexBlockNode pNode;
    DWORD info;
    for(int i=0;i<m_dwBlockCount;++i)
    {
        pNode=m_pIndex[i];

        //当前块最大值
        info=pNode->PtrData[pNode->CurrentEnd-1];//pNode->PtrData[pNode->CurrentEnd-1]&0xffffffff;

        if(BasicInfo<=info){
            if(BasicInfo==info)  return pNode->PtrData+pNode->CurrentEnd-1;
            int low=pNode->CurrentBegin,high=pNode->CurrentEnd-1;
            int mid;
            while(low<=high)
            {
                mid=low+((high-low)>>1);
                info=pNode->PtrData[mid];
                if(BasicInfo<info){
                    high=mid-1;
                }else if(BasicInfo>info){
                    low=mid+1;
                }else{
                    return pNode->PtrData+mid;
                }
            }
            break;
        }
    }
    return NULL;
}

void CDirBasicInfoMap::AddLastBlock()
{
    m_pLastNode=m_pIndex[m_dwBlockCount]=(PDirFrnIndexBlockNode)g_MemoryMgr.GetMemory(sizeof(DirFrnIndexBlockNode));
    m_pLastNode->Init();
    ++m_dwBlockCount;
    if(m_dwBlockCount==m_dwMaxCount){//索引BUF已满，增加之
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PDirFrnIndexBlockNode*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PDirFrnIndexBlockNode));                
    }
}
void CDirBasicInfoMap::InsertBlock(int i)//在第i处插入新块
{
    for(int j=m_dwBlockCount++;j>i;--j) m_pIndex[j]=m_pIndex[j-1];
    m_pIndex[i]=(PDirFrnIndexBlockNode)g_MemoryMgr.GetMemory(sizeof(DirFrnIndexBlockNode));
    m_pIndex[i]->Init();
    if(m_dwBlockCount==m_dwMaxCount){//索引BUF已满，增加之
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PDirFrnIndexBlockNode*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PDirFrnIndexBlockNode));                
    }
}



