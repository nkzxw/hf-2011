// DirBasicInfoMap.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "DirBasicInfoMap.h"

CDirBasicInfoMap g_DirMap;

#define make_map(BasicInfo,DirPtr) (DWORDLONG((BasicInfo)|(DWORDLONG(DirPtr)<<32)))

static const DWORD INDEX_COUNT_DELT=32; //������չ��������ռ䣬������Сʱ����Ӧ�ϴ�

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

//���ڳ�ʼ��ʱʹ�ã��Ա����Ч��
void CDirBasicInfoMap::push_back(DWORD BasicInfo,IndexElemType DirPtr)
{       
    ++m_size;
    DWORDLONG _val=make_map(BasicInfo,DirPtr);
    if(m_pLastNode->IsEndFull()){
        AddLastBlock();
    }
    m_pLastNode->AddEnd(_val);
} 

//���Բ��ṩɾ���ڵ㹦��
//û��ɾ������ϢҲ������Ҫ������
//�����������õ���ǰ��FRN����
//��Ϊ��������������ظ�ֵ���DirOffset�������޸� 

//���ر�ɾ����ָ��
IndexElemType CDirBasicInfoMap::erase(DWORD BasicInfo)//ɾ���ڵ�
{
    --m_size;
    PIndexElemType pMapElem=(PIndexElemType)find(BasicInfo);
    assert(pMapElem && "һ������");
    ++pMapElem;
    IndexElemType tmp=*pMapElem;
    *pMapElem=NULL;//ֱ�ӽ�ֵ��ΪNULL
    return tmp;
}

void CDirBasicInfoMap::insert(DWORD BasicInfo,IndexElemType DirPtr)
{
    ++m_size;
    assert(DirPtr);
    DWORDLONG dd_map_val=make_map(BasicInfo,DirPtr);
    //�ҵ��ڵ����ڵ�λ�ã�������Ҫ���ѽڵ�
    PDirFrnIndexBlockNode pNode;
    DWORD info;
    //�ҵ����ڿ�
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
    if(i==m_dwBlockCount){//��ǰ����������FRN
        //�����ڵ��Ƿ��пռ���룬���û�У��������ӽڵ�
        //��ʱ��m_pLastNode->CurrentBegin==0 m_pLastNode->CurrentEnd==DirFrnIndexBlockNode::CountPerIndex
        //[CurrentBegin,CurrentEnd)
        if(m_pLastNode->IsEndFull()){//����
            if(m_pLastNode->IsBeginFull()){//����
                //�������ռ�
                if(m_dwBlockCount>=2 && !m_pIndex[m_dwBlockCount-2]->IsFull()){//���н�
                    pNode=m_pIndex[m_dwBlockCount-2];
                    if(pNode->IsEndFull()){//����
                        pNode->LeftMove();
                    }
                    pNode->AddEnd(m_pLastNode->RemoveBegin());//���BEG����
                    m_pLastNode->LeftMove();//���ƶ�һ��
                    m_pLastNode->AddEnd(dd_map_val);
                }else{//���޽裬�����һ���ڵ���ѱ���3/4 1/4���½ڵ�
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode��ָ���¿�
                    //����pNode
                    m_pLastNode->CutFromLeft(pNode);
                    m_pLastNode->AddEnd(dd_map_val);
                }
            }
            else{//��δ��
                //�������ƶ�һ��
                m_pLastNode->LeftMove();
                m_pLastNode->AddEnd(dd_map_val);
            }
        }else{//��δ��
            m_pLastNode->AddEnd(dd_map_val);
        }           
    }else{//�����ֵ�����ֵ
        //pNodeָ�����ڵ�
        //Ѱ�Ҳ���λ����
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
        DWORD dwInsert=low;//����λ�� ע��[low ,CurEnd) Ӧ�����ƶ�
        if(pNode->IsFull()){
            if(pNode==m_pLastNode){//�����һ���ڵ��в���
                if(i>=1 && !m_pIndex[i-1]->IsFull()){//���н�
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//����
                        pNode->LeftMove();
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode������ֵ
                        pNode->AddEnd(dd_map_val);
                        return;
                    }else pNode->AddEnd(m_pLastNode->RemoveBegin());//���BEG����
                    m_pLastNode->Insert(dwInsert,dd_map_val);
                }else{//���޽裬�����һ���ڵ���ѱ���3/4 1/4���½ڵ�
                    pNode=m_pLastNode;
                    AddLastBlock();//m_pLastNode��ָ���¿�
                    //����pNode
                    m_pLastNode->CutFromLeft(pNode,&dwInsert);
                    pNode->AddEnd(dd_map_val);
                }
            }else if(pNode==*m_pIndex){//�ڵ�һ���ڵ��в���
                if(m_dwBlockCount>1 && !m_pIndex[1]->IsFull()){//���н�
                    pNode=m_pIndex[1];
                    if(pNode->IsBeginFull()){//����
                        pNode->RightMove();
                    }
                    //*m_pIndex�����һ��ֵһ�����ڲ���ָ
                    pNode->AddBegin((*m_pIndex)->RemoveEnd());
                    (*m_pIndex)->Insert(dwInsert,dd_map_val);
                }else{//���޽�
                    InsertBlock(1);
                    pNode=m_pIndex[1];
                    //����pNode
                    pNode->CutFromLeft(*m_pIndex,&dwInsert);
                    (*m_pIndex)->AddEnd(dd_map_val);
                }              
            }else{//�����ǵ�һ���ڵ� �ֲ������ڵ���� ����Ҫ�����߽�
                if(!m_pIndex[i-1]->IsFull()){//��ͼ�����
                    pNode=m_pIndex[i-1];
                    if(pNode->IsEndFull()){//����
                        pNode->LeftMove();
                    }
                    if(0==dwInsert){//BasicInfo< m_pLastNode������ֵ
                        pNode->AddEnd(dd_map_val);
                        return;
                    }else pNode->AddEnd(m_pIndex[i]->RemoveBegin());//���BEG����
                    m_pIndex[i]->Insert(dwInsert,dd_map_val);
                }else if(!m_pIndex[i+1]->IsFull()){//��ͼ���ҽ�
                    pNode=m_pIndex[i+1];
                    if(pNode->IsBeginFull()){//����
                        pNode->RightMove();
                    }
                    //*m_pIndex�����һ��ֵһ�����ڲ���ָ
                    pNode->AddBegin(m_pIndex[i]->RemoveEnd());
                    m_pIndex[i]->Insert(dwInsert,dd_map_val);
                }else{//���Ҿ��޽��
                    if(dwInsert<(DirFrnIndexBlockNode::CountPerIndex>>1)){
                        //����λ����ǰ�� ������鵽��һ������
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
        }else{//���ڵ���λ�ÿɲ�
            pNode->Insert(dwInsert,dd_map_val);
        }
    }    
}

//����ƫ��ֵ
DWORDLONG* CDirBasicInfoMap::find(DWORD BasicInfo)
{
    PDirFrnIndexBlockNode pNode;
    DWORD info;
    for(int i=0;i<m_dwBlockCount;++i)
    {
        pNode=m_pIndex[i];

        //��ǰ�����ֵ
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
    if(m_dwBlockCount==m_dwMaxCount){//����BUF����������֮
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PDirFrnIndexBlockNode*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PDirFrnIndexBlockNode));                
    }
}
void CDirBasicInfoMap::InsertBlock(int i)//�ڵ�i�������¿�
{
    for(int j=m_dwBlockCount++;j>i;--j) m_pIndex[j]=m_pIndex[j-1];
    m_pIndex[i]=(PDirFrnIndexBlockNode)g_MemoryMgr.GetMemory(sizeof(DirFrnIndexBlockNode));
    m_pIndex[i]->Init();
    if(m_dwBlockCount==m_dwMaxCount){//����BUF����������֮
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_pIndex=(PDirFrnIndexBlockNode*)g_MemoryMgr.realloc(m_pIndex,m_dwMaxCount*sizeof(PDirFrnIndexBlockNode));                
    }
}



