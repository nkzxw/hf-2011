// IndexNodeBlock.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

typedef PBYTE IndexElemType,*PIndexElemType;
template <typename Type=IndexElemType,int _N=4096>

struct IndexBlockNode{
    static const DWORD CountPerIndex=_N;

    DWORD CurrentBegin,CurrentEnd;//[CurrentBegin,CurrentEnd)
    Type PtrData[_N];//存储偏移值

    void Init(){CurrentBegin=0;CurrentEnd=0;}
    DWORD GetCount(){return CurrentEnd-CurrentBegin;}   //有效记录数
    BOOL IsEmpty(){return CurrentBegin==CurrentEnd;}
    BOOL IsNotEmpty(){return CurrentEnd>CurrentBegin;}
    BOOL IsFull(){return CurrentBegin+_N==CurrentEnd;}  //记录已装满
    BOOL IsEndFull(){return _N==CurrentEnd;}            //结尾已满
    BOOL IsBeginFull(){return 0==CurrentBegin;}         //开始处满
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
    void LeftMove(int n=1){//整体左移n格
        assert(CurrentBegin>=n);
        CurrentBegin-=n;CurrentEnd-=n;
        Type *p=PtrData+CurrentBegin,*pEnd=PtrData+CurrentEnd;
        for(;p<pEnd;++p) *p=*(p+n);      
    }
    void RightMove(int n=1){//整体右移动n格
        assert(CurrentEnd+n<=_N);
        CurrentBegin+=n;CurrentEnd+=n;
        Type *pBeg=PtrData+CurrentBegin,*p=PtrData+CurrentEnd-1;
        for(;p>=pBeg;--p) *p=*(p-n); 
    }
    void Insert(DWORD dwInsert,Type&_val)
    {//插入过程应保持有序  将[...dwInsert-1]左移动或者 [dwInsert ...] 右移动
        assert(!IsFull());
        DWORD tmp;
        if(_N-CurrentEnd>CurrentBegin){//右边空间多
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

    //剪切节点node的数据,node在*this的左边
    //当pInsertPos非0时 *pInsertPos为node的插入位置，将node已此位置做分裂
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
    //剪切节点node的数据,node在*this的右边
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
    void Erase(DWORD pos)//删除pos位置的值
    {
        assert(pos>=CurrentBegin && pos<CurrentEnd);
        //[CurrentBegin  pos]  CurrentEnd)
        DWORD tmp;
        if(_N-CurrentEnd>CurrentBegin){//右边空间多，左边部分右移
            tmp=CurrentEnd;
            CurrentEnd=pos;
            RightMove();
            CurrentEnd=tmp;
        }else{//左边空间多，右边部分左移
            tmp=CurrentBegin;
            CurrentBegin=pos+1;
            LeftMove();
            CurrentBegin=tmp;
        }            
    }
};


