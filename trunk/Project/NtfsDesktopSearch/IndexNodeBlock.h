// IndexNodeBlock.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

typedef PBYTE IndexElemType,*PIndexElemType;
template <typename Type=IndexElemType,int _N=4096>

struct IndexBlockNode{
    static const DWORD CountPerIndex=_N;

    DWORD CurrentBegin,CurrentEnd;//[CurrentBegin,CurrentEnd)
    Type PtrData[_N];//�洢ƫ��ֵ

    void Init(){CurrentBegin=0;CurrentEnd=0;}
    DWORD GetCount(){return CurrentEnd-CurrentBegin;}   //��Ч��¼��
    BOOL IsEmpty(){return CurrentBegin==CurrentEnd;}
    BOOL IsNotEmpty(){return CurrentEnd>CurrentBegin;}
    BOOL IsFull(){return CurrentBegin+_N==CurrentEnd;}  //��¼��װ��
    BOOL IsEndFull(){return _N==CurrentEnd;}            //��β����
    BOOL IsBeginFull(){return 0==CurrentBegin;}         //��ʼ����
    Type& RemoveBegin(){
        assert(IsNotEmpty());
        return PtrData[CurrentBegin++];
    }
    Type& RemoveEnd(){
        assert(IsNotEmpty());
        return PtrData[--CurrentEnd];
    }
    void AddBegin(Type &_val){
        assert(!IsBeginFull());
        PtrData[--CurrentBegin]=_val;
    }
    void AddEnd(Type &_val){
        assert(!IsEndFull());
        PtrData[CurrentEnd++]=_val;
    }
    void LeftMove(int n=1){//��������n��
        assert(CurrentBegin>=n);
        CurrentBegin-=n;CurrentEnd-=n;
        Type *p=PtrData+CurrentBegin,*pEnd=PtrData+CurrentEnd;
        for(;p<pEnd;++p) *p=*(p+n);      
    }
    void RightMove(int n=1){//�������ƶ�n��
        assert(CurrentEnd+n<=_N);
        CurrentBegin+=n;CurrentEnd+=n;
        Type *pBeg=PtrData+CurrentBegin,*p=PtrData+CurrentEnd-1;
        for(;p>=pBeg;--p) *p=*(p-n); 
    }
    void Insert(DWORD dwInsert,Type&_val)
    {//�������Ӧ��������  ��[...dwInsert-1]���ƶ����� [dwInsert ...] ���ƶ�
        assert(!IsFull());
        DWORD tmp;
        if(_N-CurrentEnd>CurrentBegin){//�ұ߿ռ��
            tmp=CurrentBegin;
            CurrentBegin=dwInsert;
            RightMove();
            PtrData[dwInsert]=_val;
            CurrentBegin=tmp;
        }else{
            tmp=CurrentEnd;
            CurrentEnd=dwInsert;
            LeftMove();
            PtrData[dwInsert-1]=_val;
            CurrentEnd=tmp;
        }
    }

    //���нڵ�node������,node��*this�����
    //��pInsertPos��0ʱ *pInsertPosΪnode�Ĳ���λ�ã���node�Ѵ�λ��������
    void CutFromLeft(IndexBlockNode* pNode,DWORD *pInsertPos=NULL){
        assert(IsEmpty() && pNode->IsFull() && "CutFromLeft");
        Type *pSrc,*pSrcEnd=pNode->PtrData+pNode->CurrentEnd;
        if(NULL==pInsertPos) {
            pSrc=pNode->PtrData+(_N>>1); 
            CurrentEnd=CurrentBegin=_N>>2;
        }
        else {
            pSrc=pNode->PtrData+*pInsertPos;
            CurrentEnd=CurrentBegin=((*pInsertPos)>>1);
        }
        pNode->CurrentEnd=pSrc-pNode->PtrData;
        for(;pSrc<pSrcEnd;++pSrc)
        {
            PtrData[CurrentEnd++]=*pSrc;
        }       
    }
    //���нڵ�node������,node��*this���ұ�
    void CutFromRight(IndexBlockNode* pNode,DWORD *pInsertPos=NULL){
        assert(IsEmpty() && pNode->IsFull() && "CutFromRight");
        Type *pSrc=pNode->PtrData,*pSrcEnd;
        if(NULL==pInsertPos) {
            pSrcEnd=pNode->PtrData+(_N>>1); 
            CurrentEnd=CurrentBegin=_N>>2;
        }
        else {
            pSrcEnd=pNode->PtrData+*pInsertPos;
            CurrentEnd=CurrentBegin=(_N-*pInsertPos)>>1;
        }
        pNode->CurrentBegin=pSrcEnd-pNode->PtrData;
        for(;pSrc<pSrcEnd;++pSrc)
        {
            PtrData[CurrentEnd++]=*pSrc;
        }  
    }
    void Erase(DWORD pos)//ɾ��posλ�õ�ֵ
    {
        assert(pos>=CurrentBegin && pos<CurrentEnd);
        //[CurrentBegin  pos]  CurrentEnd)
        DWORD tmp;
        if(_N-CurrentEnd>CurrentBegin){//�ұ߿ռ�࣬��߲�������
            tmp=CurrentEnd;
            CurrentEnd=pos;
            RightMove();
            CurrentEnd=tmp;
        }else{//��߿ռ�࣬�ұ߲�������
            tmp=CurrentBegin;
            CurrentBegin=pos+1;
            LeftMove();
            CurrentBegin=tmp;
        }            
    }
};


