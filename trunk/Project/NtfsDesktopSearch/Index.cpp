// Index.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "Index.h"

CIndex g_vDirIndex,g_vFileIndex;
static const DWORD INDEX_COUNT_DELT=32; //������չ��������ռ䣬������Сʱ����Ӧ�ϴ�

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

//���ļ���˳�����
void CIndex::insert(IndexElemType Record,BOOL bDir)
{
    DebugTrace("**********�����¼\n");
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
        DebugTrace("��ǰ�����������ļ�Ŀ¼��\n");
        //�����ڵ��Ƿ��пռ���룬���û�У��������ӽڵ�
        //��ʱ��m_pLastNode->CurrentBegin==0 m_pLastNode->CurrentEnd==DirFrnIndexBlockNode::CountPerIndex
        //[CurrentBegin,CurrentEnd)
        if(m_pLastNode->IsEndFull()){//����
            DebugTrace("����\n");
            if(m_pLastNode->IsBeginFull()){//����
                DebugTrace("����\n");
                //�������ռ�
                if(m_dwBlockCount>=2 && !m_pIndex[m_dwBlockCount-2]->IsFull()){//���н�
                    DebugTrace("���н�\n");
                    pNode=m_pIndex[m_dwBlockCount-2];
                    if(pNode->IsEndFull()){//����
                        DebugTrace("���Ҷ���������ǰBeg=%d,End=%d\n",pNode->CurrentBegin,pNode->CurrentEnd);
                        pNode->LeftMove();
                        DebugTrace("���Ҷ��������ƺ�Beg=%d,End=%d\n",pNode->CurrentBegin,pNode->CurrentEnd);
                    }
                    DebugTrace("������ƶ�ǰ����Beg1=%d,End1=%d����Beg2=%d,End2=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    pNode->AddEnd(m_pLastNode->RemoveBegin());//���BEG����
                    DebugTrace("������ƶ��󣬡�Beg1=%d,End1=%d����Beg2=%d,End2=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    m_pLastNode->LeftMove();//���ƶ�һ��
                    DebugTrace("���ƶ�һ�񣬡�Beg=%d,End=%d��\n",m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                }else{//���޽裬�����һ���ڵ���ѱ���3/4 1/4���½ڵ�
                    DebugTrace("���޽衾Beg=%d,End=%d��\n",m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode��ָ���¿�
                    DebugTrace("�����¿飬��Beg1=%d,End1=%d����Beg2=%d,End2=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                    //����pNode
                    m_pLastNode->CutFromLeft(pNode);
                    DebugTrace("����У���Beg1=%d,End1=%d����Beg2=%d,End2=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);         
                }
                DebugTrace("AddEnd��Beg=%d,End=%d��\n",m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                m_pLastNode->AddEnd(Record);
            }
            else{//��δ��
                DebugTrace("��δ��\n");
                //�������ƶ�һ��
                m_pLastNode->LeftMove();
                m_pLastNode->AddEnd(Record);
            }
        }else{//��δ��
            DebugTrace("��δ��\n");
            m_pLastNode->AddEnd(Record);
        }           
    }else{//�����ֵ�����ֵ
        //pNodeָ�����ڵ�
        //Ѱ�Ҳ���λ����
        
        int low=pNode->CurrentBegin,high=pNode->CurrentEnd-1;
        int mid;  
        DWORD dwInsert;
        while(low<=high)
        {
            mid=low+((high-low)>>1);
            cmp=FunCmpare(Record,pNode->PtrData[mid]);
            if(cmp<0) high=mid-1;
            else if(cmp>0) low=mid+1;
            else{//�ҵ�ͬ��
                dwInsert=mid;
                break;
            }
        }
        if(low>high) dwInsert=low;   
        DebugTrace("Ѱ�Ҳ���λ���ȡ�Beg=%d,dwInsert=%d,End=%d��\n",pNode->CurrentBegin,dwInsert,pNode->CurrentEnd);
        assert(dwInsert>=pNode->CurrentBegin && dwInsert<pNode->CurrentEnd);
        //����λ�� ע��[dwInsert ,CurEnd) Ӧ�����ƶ�
        if(pNode->IsFull()){
            DebugTrace("����ڵ���\n");
            if(pNode==m_pLastNode){//�����һ���ڵ��в���
                DebugTrace("�������һ���ڵ��в���\n");
                if(i>=1 && !m_pIndex[i-1]->IsFull()){//���н�
                    DebugTrace("���н�\n");
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//����
                        DebugTrace("��ڵ� ������Beg=%d,End=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd);
                        pNode->LeftMove();
                        DebugTrace("���ƶ� ������Beg=%d,End=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd);
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode������ֵ
                        DebugTrace("ֱ�Ӳ��������\n");
                        pNode->AddEnd(Record);
                        DebugTrace("��Beg=%d,End=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd);
                        return;
                    }else pNode->AddEnd(m_pLastNode->RemoveBegin());//���BEG����
                    m_pLastNode->Insert(dwInsert,Record);
                    DebugTrace("������ƶ��󣬡�Beg1=%d,End1=%d����Beg2=%d,End2=%d��\n",pNode->CurrentBegin,pNode->CurrentEnd,m_pLastNode->CurrentBegin,m_pLastNode->CurrentEnd);
                }else{//���޽裬�����һ���ڵ���ѱ���3/4 1/4���½ڵ�
                    DebugTrace("���޽裬���������ڵ�\n");
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode��ָ���¿�
                    //����pNode
                    m_pLastNode->CutFromLeft(pNode,&dwInsert);
                    pNode->AddEnd(Record);
                }
            }else if(pNode==*m_pIndex){//�ڵ�һ���ڵ��в���
                DebugTrace("�ڵ�һ���ڵ��в���\n");
                if(m_dwBlockCount>1 && !m_pIndex[1]->IsFull()){//���н�
                    DebugTrace("���н�\n");
                    pNode=m_pIndex[1];
                    if(pNode->IsBeginFull()){//����
                        DebugTrace("���������ƶ�\n");
                        pNode->RightMove();
                    }
                    //*m_pIndex�����һ��ֵһ�����ڲ���ָ
                    pNode->AddBegin((*m_pIndex)->RemoveEnd());
                    (*m_pIndex)->Insert(dwInsert,Record);
                }else{//���޽�
                    DebugTrace("���޽�\n");
                    InsertBlock(1);
                    pNode=m_pIndex[1];
                    //����pNode
                    pNode->CutFromLeft(*m_pIndex,&dwInsert);
                    (*m_pIndex)->AddEnd(Record);
                }              
            }else{//�����ǵ�һ���ڵ� �ֲ������ڵ���� ����Ҫ�����߽�
                DebugTrace("�����߽�\n");
                if(!m_pIndex[i-1]->IsFull()){//��ͼ�����
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//����
                        pNode->LeftMove();
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode������ֵ
                        pNode->AddEnd(Record);
                        return;
                    }else pNode->AddEnd(m_pIndex[i]->RemoveBegin());//���BEG����
                    m_pIndex[i]->Insert(dwInsert,Record);
                }else if(!m_pIndex[i+1]->IsFull()){//��ͼ���ҽ�
                    pNode=m_pIndex[i+1];
                    if(pNode->IsBeginFull()){//����
                        pNode->RightMove();
                    }
                    //*m_pIndex�����һ��ֵһ�����ڲ���ָ
                    pNode->AddBegin(m_pIndex[i]->RemoveEnd());
                    m_pIndex[i]->Insert(dwInsert,Record);
                }else{//���Ҿ��޽��
                    //�����½ڵ㣬Ȼ�������߽裬������ʵ�ֵĿ���һ�ߵ�
                    //�Ż������Կ������ߵ����ݣ�ʹ������3���ڵ㶼δ��
                    if(dwInsert<(INDEX_BLOCK_NODE::CountPerIndex>>1)){ 
                        //����λ����ǰ�� ������鵽��һ������
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
        }else{//���ڵ���λ�ÿɲ�
            DebugTrace("����ڵ�δ��\n");
            pNode->Insert(dwInsert,Record);
            DebugTrace("Ѱ�Ҳ���λ���ȡ�Beg=%d,dwInsert=%d,End=%d��\n",pNode->CurrentBegin,dwInsert,pNode->CurrentEnd);
        }
    }   
    DebugTrace("**********�������===============\n");
}

//ɾ����ʱ�����ڻ�����������󣬱����ṩ�ļ�BasicInfo
//���ڵ�Ϊ��ʱ���ͷſսڵ�飬��������û�пսڵ��
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
    assert(i<m_dwBlockCount && "�ļ���һ���ڿ��д���");
    //if(i==m_dwBlockCount) return;//�ļ������ڿ���

    //pNodeָ�����ڵ�
    //Ѱ�Ҳ���λ����
    int low=pNode->CurrentBegin,high=pNode->CurrentEnd-1;
    int mid;  
    while(low<=high)
    {
        mid=low+((high-low)>>1);
        cmp=FunCmpare(Record,pNode->PtrData[mid]);
        if(cmp<0) high=mid-1;
        else if(cmp>0) low=mid+1;
        else{//�ҵ�ͬ��
            break;
        }
    }
    assert(low<=high && "�ļ���һ���ڿ��д���");
    //if(low>high) return;//�ļ������ڿ���

    DebugTrace("erase2\n");
    //mid��Ϊ�ļ���������������������
    //Ӧ��һ�������߼��BasicInfo �����߷��ʵĹ����п�����Ҫ���
    if(*(DWORD*)(pNode->PtrData[mid])==BasicInfo){//������������ظ��ļ������
        DebugTrace("erase3 i=%d beg=%d mid=%d end=%d\n",i,pNode->CurrentBegin,mid,pNode->CurrentEnd);
        assert(m_pIndex[i]==pNode);
        pNode->Erase(mid);//ɾ����mid��ֵ
        DebugTrace("erase3.5 i=%d beg=%d mid=%d end=%d\n",i,pNode->CurrentBegin,mid,pNode->CurrentEnd);
        if(pNode->IsEmpty()) EraseBlock(i);//ɾ����ǰ��
        DebugTrace("erase4\n");
    }
    else{//�ļ�����ͬ������ǰλ��mid���������ļ�
        DebugTrace("erase5\n");
        int j=i;
        PIndexElemType p
            ,pBeg//[
            ,pEnd;//)
        BOOL bFail=FALSE;
        assert(pNode==m_pIndex[i]);
        int left=pNode->CurrentBegin,right=mid-1;
        DebugTrace("erase ������\n");
        for(j=i;;)
        {
            p=pNode->PtrData+right;
            pBeg=pNode->PtrData+left;
            for(;p>=pBeg;--p)
            {
                cmp=FunCmpare(Record,*p);
                if(0==cmp){
                    if(*(DWORD*)*p==BasicInfo){
                        pNode->Erase(pNode->CurrentBegin+(p-pBeg));//ɾ��
                        if(pNode->IsEmpty()) EraseBlock(j);//ɾ����ǰ�� 
                        return;
                    }
                }else{//û���ҵ�
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

        //���û���ҵ����ұ߿���
        DebugTrace("erase ������\n");
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
                        pNode->Erase(pNode->CurrentEnd-(pEnd-p));//ɾ��
                        if(pNode->IsEmpty()) EraseBlock(j);//ɾ����ǰ�� 
                        return;
                    }
                }else{//û���ҵ�
                    assert(0 && "����ĳ���ȣ���Ȼ�Ҳ���?");
                    return;
                }                  
            }
            ++j;
            if(j<m_dwBlockCount){
                pNode=m_pIndex[j];
                left=pNode->CurrentBegin;
                right=pNode->CurrentEnd;               
            }else{
                assert(0 && "�����ˣ���Ȼ�Ҳ���?");
                return;
            }
        }
    }
}


//����˳����������û���������ٶ�
PINDEX_BLOCK_NODE *CIndex::GetBlockIndex()const{return m_pIndex;}
int CIndex::GetBlockCount()const{return m_dwBlockCount;}

void CIndex::AddLastBlock()
{
    m_pLastNode=m_pIndex[m_dwBlockCount]=(PINDEX_BLOCK_NODE)g_MemoryMgr.GetMemory(sizeof(INDEX_BLOCK_NODE));
    m_pLastNode->Init();
    ++m_dwBlockCount;
    if(m_dwBlockCount==m_dwMaxCount){//����BUF����������֮
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PINDEX_BLOCK_NODE*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PINDEX_BLOCK_NODE));                
    }
}
void CIndex::InsertBlock(int i)//�ڵ�i�������¿�
{
    for(int j=m_dwBlockCount++;j>i;--j) m_pIndex[j]=m_pIndex[j-1];
    m_pIndex[i]=(PINDEX_BLOCK_NODE)g_MemoryMgr.GetMemory(sizeof(INDEX_BLOCK_NODE));
    m_pIndex[i]->Init();
    if(m_dwBlockCount==m_dwMaxCount){//����BUF����������֮
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PINDEX_BLOCK_NODE*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PINDEX_BLOCK_NODE));                
    }
}

void CIndex::EraseBlock(int i)//���ǵ��սڵ�Ӱ�����Ч�� ɾ����i��սڵ�
{
    assert(i>=0 && i<m_dwBlockCount);
    PINDEX_BLOCK_NODE pNode=m_pIndex[i];
    //[i ... m_dwBlockCount-1]
    --m_dwBlockCount;
    for(;i<m_dwBlockCount;++i) m_pIndex[i]=m_pIndex[i+1];
    g_MemoryMgr.FreeMemory((PBYTE)pNode);
}

