// StrMatch.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Record.h"
//src中包含L'?'不包含L'*'
//src不以L'\0'结尾
//dest不以L'\0'结尾
__forceinline BOOL Helper_MatchQuestion1(PWCHAR dest,int destLen,PWCHAR src,int srcLen)
{
    if(srcLen==destLen){//长度相等才有希望
        for(int i=0;i<srcLen;++i){
            if(src[i]!=dest[i] && src[i]!=L'?') return FALSE;  
        }      
        return TRUE;
    }
    else return FALSE;
}

//src中包含L'?'不包含L'*'
//src以L'\0'结尾
//dest不以L'\0'结尾
__forceinline BOOL Helper_MatchQuestion2(PWCHAR dest,int destLen,PWCHAR src,int srcLen)
{
    if(srcLen==destLen){//长度相等才有希望
        for(;*src!=L'\0' && (L'?'==*src || *src==*dest);++src,++dest);
        return *src==L'\0';   
    }
    else return FALSE;
}

__forceinline BOOL Helper_MatchSubString(PWCHAR dest,int destLen,PWCHAR src,int srcLen)
{
    assert(srcLen>0);
    if(srcLen<=destLen){
        PWCHAR s_dest//指向目标串
            ,s_src//指向模式串
            ,srcEnd=src+srcLen;
        PWCHAR destEnd=dest+destLen-srcLen+1;//
        for(;dest<destEnd;++dest)
        {
            s_dest = dest;
            s_src = src;
            for(;s_src<srcEnd && *s_src==*s_dest;++s_src,++s_dest);
            if (s_src==srcEnd) return TRUE;
        }
    }
    return FALSE;//srcLen>destLen 或 srcLen<=destLen 未匹配
}

//注意dest串可能含有?
__forceinline BOOL Helper_MatchSubStringToQStr(PWCHAR dest,int destLen,PWCHAR src,int srcLen)
{
    assert(srcLen>0);
    if(srcLen<=destLen){
        PWCHAR s_dest//指向目标串
            ,s_src//指向模式串
            ,srcEnd=src+srcLen;
        PWCHAR destEnd=dest+destLen-srcLen+1;//
        for(;dest<destEnd;++dest)
        {
            s_dest = dest;
            s_src = src;
            for(;s_src<srcEnd && (*s_src==*s_dest||L'?'==*s_dest);++s_src,++s_dest);
            if (s_src==srcEnd) return TRUE;
        }
    }
    return FALSE;//srcLen>destLen 或 srcLen<=destLen 未匹配
}


// const WCHAR g_wCharSpec[]={L'*',L'\"',L'?',L'|',L':',L'<',L'>',L'\\',L'/'};
// const int g_nSpecCharSize=sizeof(g_wCharSpec)/sizeof(WCHAR);
// 
// __forceinline BOOL IsSpecialChar(const WCHAR wch)
// {
//     for(int i=0;i<g_nSpecCharSize;++i){
//         if(wch==g_wCharSpec[i]){
//             return TRUE;
//         }
//     }
//     return FALSE;
// }

//去空格
//返回串结果长度
//大小写敏感
//返回结果：
//仅含?     ?*               *              普通(长度大到小) 
//长度=a1   长度>=a2         长度>=a3       长度>=a4 ...
//type=01   11               10             00 

//如果输入串不合法，返回FALSE，不会进行搜索
//返回TRUE 表示输入串合法

//输入串，搜索选项
struct ListQuestion
{
    ListQuestion(){}
    ListQuestion(int len,PWCHAR wchs,ListQuestion *pNext=NULL)
        :_len(len),_pNext(pNext){
            wcsncpy(_wchs,wchs,len);
            _wchs[len]=L'\0';
    }
    ListQuestion& operator=(const ListQuestion &list)
    {
        if(this!=&list)
        {
            _len=list._len;
            wcscpy(_wchs,list._wchs);
            ListQuestion *pTail=this;
            ListQuestion *p=list._pNext;
            while(p){
                pTail->_pNext=new ListQuestion(p->_len,p->_wchs); 
                pTail=pTail->_pNext;
                p=p->_pNext;
            }
            pTail->_pNext=NULL;
        }
        return *this;
    }
    BOOL operator==(const ListQuestion& list)const{
        if(_len!=list._len) return FALSE;
        if(wcscmp(_wchs,list._wchs)) return FALSE;
        if(0==_pNext) {
            if(0==list._pNext) return TRUE;
            else return FALSE;
        }
        else{
            if(0==list._pNext) return FALSE;
            else return *_pNext==*list._pNext;
        }
    }
    int _len;
    WCHAR _wchs[1024];
    ListQuestion * _pNext;
};

#define LEFT            0x1 //左非*
#define RIGHT           0x2 //右非*
#define QUESTION_TAG    0x1
#define STAR_TAG        0x2

struct ListStar
{
    ListStar(){}
    ListStar(int len,PWCHAR wchs,int type,int nStars,ListStar *pNext=NULL)
        :_len(len),_type(type),_nStars(nStars),_pNext(pNext){
            wcsncpy(_wchs,wchs,len);
            _wchs[len]=L'\0';
    }
    ListStar& operator=(const ListStar &list)
    {
        if(this!=&list)
        {
            _type=list._type;
            _nStars=list._nStars;
            _len=list._len;
            wcscpy(_wchs,list._wchs);
            ListStar *pTail=this;
            ListStar *p=list._pNext;
            while(p){
                pTail->_pNext=new ListStar(p->_len,p->_wchs,p->_type,p->_nStars); 
                pTail=pTail->_pNext; 
                p=p->_pNext;            
            }
            pTail->_pNext=NULL;
        }
        return *this;
    }
    BOOL operator==(const ListStar& list)const{
        if(_type!=list._type) return FALSE;
        if(_nStars!=list._nStars) return FALSE;
        if(_len!=list._len) return FALSE;
        if(wcscmp(_wchs,list._wchs)) return FALSE;
        if(0==_pNext) {
            if(0==list._pNext) return TRUE;
            else return FALSE;
        }
        else{
            if(0==list._pNext) return FALSE;
            else return *_pNext==*list._pNext;
        }
    }
    //查看_wchs是否能匹配ucs2Name
    BOOL StarSubStringMatch(PWCHAR ucs2Name,int ucs2Len)
    {
        if(_len-_nStars>ucs2Len) return FALSE;

        PWCHAR pWch1=_wchs,pWch2=_wchs+_len-1,pUcs1=ucs2Name,pUcs2=ucs2Name+ucs2Len-1;
        if(_type&LEFT){         
            for(;*pWch1!=L'*';++pWch1,++pUcs1){
                if(*pUcs1!=*pWch1) return FALSE;
            }
        }
        if(_type&RIGHT){
            for(;*pWch2!=L'*';--pWch2,--pUcs2){
                if(*pUcs2!=*pWch2) return FALSE;
            }
        }
        //pWch1 pWch2 都指向*
        if(pWch1==pWch2) return TRUE;
        ++pWch1;
        int i;
match:
        for(i=0;pWch1[i]!=L'*';++i){
            if(pUcs1+i>pUcs2) return FALSE;
            if(pWch1[i]!=pUcs1[i]){
                ++pUcs1; 
                goto match;
            }
        }

        pWch1+=i;
        if(pWch1==pWch2) return TRUE;
        else {
            ++pWch1;
            goto match;
        }
        //【* |  *..*  | *..*..* | ...  】匹配 [pUcs1,pUcs2]
    }
    int _type;//LEFT表示最左非*  RIGHT最右非*  LEFT|RIGHT左右皆非*
    int _nStars;
    int _len;
    WCHAR _wchs[1024];
    ListStar * _pNext;
};
struct ListStarQuestion
{
    ListStarQuestion(){}
    ListStarQuestion(int len,PWCHAR wchs,int type,int nStars,ListStarQuestion *pNext=NULL)
        :_len(len),_type(type),_nStars(nStars),_pNext(pNext){
            wcsncpy(_wchs,wchs,len);
            _wchs[len]=L'\0';
    }
    ListStarQuestion& operator=(const ListStarQuestion &list)
    {
        if(this!=&list)
        {
            _type=list._type;
            _nStars=list._nStars;
            _len=list._len;
            wcscpy(_wchs,list._wchs);
            ListStarQuestion *pTail=this;
            ListStarQuestion *p=list._pNext;
            while(p){
                pTail->_pNext=new ListStarQuestion(p->_len,p->_wchs,p->_type,p->_nStars); 
                pTail=pTail->_pNext; 
                p=p->_pNext;
            }
            pTail->_pNext=NULL;
        }
        return *this;
    }
    BOOL operator==(const ListStarQuestion& list)const{
        if(_type!=list._type) return FALSE;
        if(_nStars!=list._nStars) return FALSE;
        if(_len!=list._len) return FALSE;
        if(wcscmp(_wchs,list._wchs)) return FALSE;
        if(0==_pNext) {
            if(0==list._pNext) return TRUE;
            else return FALSE;
        }
        else{
            if(0==list._pNext) return FALSE;
            else return *_pNext==*list._pNext;
        }
    }
    //查看_wchs是否能匹配ucs2Name
    BOOL StarQuestionSubStringMatch(PWCHAR ucs2Name,int ucs2Len)
    {
        if(_len-_nStars>ucs2Len) return FALSE;

        PWCHAR pWch1=_wchs,pWch2=_wchs+_len-1,pUcs1=ucs2Name,pUcs2=ucs2Name+ucs2Len-1;
        if(_type&LEFT){         
            for(;*pWch1!=L'*';++pWch1,++pUcs1){
                if(*pWch1!=L'?' && *pUcs1!=*pWch1) return FALSE;
            }
        }
        if(_type&RIGHT){
            for(;*pWch2!=L'*';--pWch2,--pUcs2){
                if(*pWch2!=L'?' && *pUcs2!=*pWch2) return FALSE;
            }
        }
        //pWch1 pWch2 都指向*
        if(pWch1==pWch2) return TRUE;
        ++pWch1;
        int i;
match:
        for(i=0;pWch1[i]!=L'*';++i){
            if(pUcs1+i>pUcs2) return FALSE;
            if(pWch1[i]!=L'?' && pWch1[i]!=pUcs1[i]){
                ++pUcs1; 
                goto match;
            }
        }

        pWch1+=i;
        if(pWch1==pWch2) return TRUE;
        else {
            ++pWch1;
            goto match;
        }
    }
    int _type;//LEFT表示最左非*  RIGHT最右非*  LEFT|RIGHT左右皆非*
    int _nStars;
    int _len;
    WCHAR _wchs[1024];
    ListStarQuestion * _pNext;
};
struct ListNormal
{
    ListNormal(){}
    ListNormal(int len,PWCHAR wchs,ListNormal *pNext=NULL)
        :_len(len),_pNext(pNext){
            wcsncpy(_wchs,wchs,len);
            _wchs[len]=L'\0';
    }
    ListNormal& operator=(const ListNormal &list)
    {
        if(this!=&list)
        {
            _len=list._len;
            wcscpy(_wchs,list._wchs);
            ListNormal *pTail=this;
            ListNormal *p=list._pNext;
            while(p){
                pTail->_pNext=new ListNormal(p->_len,p->_wchs); 
                pTail=pTail->_pNext; 
                p=p->_pNext;
            }
            pTail->_pNext=NULL;
        }
        return *this;
    }
    BOOL operator==(const ListNormal& list)const{
        if(_len!=list._len) return FALSE;
        if(wcscmp(_wchs,list._wchs)) return FALSE;
        if(0==_pNext) {
            if(0==list._pNext) return TRUE;
            else return FALSE;
        }
        else{
            if(0==list._pNext) return FALSE;
            else return *_pNext==*list._pNext;
        }
    }
    int _len;
    WCHAR _wchs[1024];
    ListNormal * _pNext;
};

//扩展名搜索
// struct ExtCond
// {
//     static int CheckExtension(PWCHAR pStarStr,int starLen,int Len)//检查是否为扩展名
//     {
//         if(Len<3) return -1;
//         int i;
//         for(i=0;pStarStr[i]==L'*';++i);
//         if(i!=starLen) return -1;
// 
//         if(Len<3 || starLen!=1 || *pStarStr!=L'*'  || pStarStr[1]!=L'*') return -1;
//         return g_ExtMgr.find(pStarStr+2);
//     }
//     BOOL IsValidExt(){return idExt_1>-2;}
//     int GetExtId(){return idExt_1+1;}//扩展名数据库中的ID
//     BOOL operator==(const ExtCond & obj){return idExt_1==obj.idExt_1;}
//     int idExt_1;//-2输入了多个扩展名查询 或者 指定的扩展名不存在  (此时查询不应查询)
//     //指定了一个扩展名且存在
// };


struct SearchStrOpt
{
    SearchStrOpt(){
        pNormal=NULL;pQuestion=NULL;pStar=NULL;pStarQuestion=NULL;
        _bAllChar=TRUE;
    }
    SearchStrOpt& operator=(const SearchStrOpt& obj){
        if(this!=&obj)
        {
            Reset();
            _bAllChar=obj._bAllChar;


            if(NULL==obj.pNormal) pNormal=NULL;
            else {
                pNormal=new ListNormal;
                *pNormal=*obj.pNormal;
            }
            if(NULL==obj.pStar) pStar=NULL;
            else {
                pStar=new ListStar;
                *pStar=*obj.pStar;
            }
            if(NULL==obj.pQuestion) pQuestion=NULL;
            else {
                pQuestion=new ListQuestion;
                *pQuestion=*obj.pQuestion;
            }
            if(NULL==obj.pStarQuestion) pStarQuestion=NULL;
            else {
                pStarQuestion=new ListStarQuestion;
                *pStarQuestion=*obj.pStarQuestion;
            }
        }
        return *this;
    }
    BOOL operator==(const SearchStrOpt& obj) const{//判断是否相等
        if(_bAllChar!=obj._bAllChar) return FALSE;
        if(NULL==pNormal){
            if(obj.pNormal) return FALSE;
        }else{
            if(NULL==obj.pNormal) return FALSE;
            else{
                if(!(*pNormal==*obj.pNormal))return FALSE;
            }
        }
        if(NULL==pQuestion){
            if(obj.pQuestion) return FALSE;
        }else{
            if(NULL==obj.pQuestion) return FALSE;
            else{
                if(!(*pQuestion==*obj.pQuestion))return FALSE;
            }
        }
        if(NULL==pStar){
            if(obj.pStar) return FALSE;
        }else{
            if(NULL==obj.pStar) return FALSE;
            else{
                if(!(*pStar==*obj.pStar))return FALSE;
            }
        }
        if(NULL==pStarQuestion){
            if(obj.pStarQuestion) return FALSE;
        }else{
            if(NULL==obj.pStarQuestion) return FALSE;
            else{
                if(!(*pStarQuestion==*obj.pStarQuestion))return FALSE;
            }
        }
        return TRUE;      
    }
    VOID Reset(){
        for(ListNormal *p=pNormal;p!=NULL;){
            ListNormal *q=p;
            p=p->_pNext;
            delete q;
        }
        for(ListQuestion *p=pQuestion;p!=NULL;){
            ListQuestion *q=p;
            p=p->_pNext;
            delete q;
        }
        for(ListStar *p=pStar;p!=NULL;){
            ListStar *q=p;
            p=p->_pNext;
            delete q;
        }
        for(ListStarQuestion *p=pStarQuestion;p!=NULL;){
            ListStarQuestion *q=p;
            p=p->_pNext;
            delete q;
        }
        pNormal=NULL;pQuestion=NULL;pStar=NULL;pStarQuestion=NULL;
        _bAllChar=TRUE;
    }
    //len串长
    //wchs串
    //nStars星号个数 用于10 11
    BOOL AddNode(BYTE type,int len,PWCHAR wchs,int nStars){
        if(0==type){//无通配符
            if(pQuestion){//仅含? 的通配符限定长度
                //检测是否与含?的串冲突，并此元素的增加可改变1==type串
                if(!Helper_MatchSubStringToQStr(pQuestion->_wchs,pQuestion->_len,wchs,len)) return FALSE;
                if(pQuestion->_len==len){
                    wcsncpy(pQuestion->_wchs,wchs,len);
                }
            }
            if(NULL==pNormal){
                pNormal=new ListNormal(len,wchs);
                return TRUE;
            }
            //插入00 长度递减
            ListNormal *p=pNormal,*q;
            if(p->_len>=len){
                if(Helper_MatchSubString(p->_wchs,p->_len,wchs,len)) return TRUE;//已存在
            }
            q=p;p=p->_pNext;
            for(;p!=NULL;q=p,p=p->_pNext){
                if(p->_len>=len){
                    if(Helper_MatchSubString(p->_wchs,p->_len,wchs,len)) return TRUE;//已存在
                }else break;
            }
            q->_pNext=new ListNormal(len,wchs,p); 
            return TRUE;
        }else if(QUESTION_TAG==type){//？ 该类型仅含一个元素，并且可能其中不含? 因为在插入过程中化简可消掉？
            //例如： 1?  ?1   表示11 长度为2 
            if(!pQuestion){
                //由于此时查询串长度固定为len,对先前已插入的00 STAR_TAG 及 STAR_TAG|QUESTION_TAG进行清理
                for(ListNormal* p=pNormal;p!=NULL;p=p->_pNext)
                {
                    if(!Helper_MatchSubStringToQStr(wchs,len,p->_wchs,p->_len)) return FALSE;
                }

                for(ListStar* p=pStar;p!=NULL;p=p->_pNext)
                {
                    if(p->_len-p->_nStars>len) return FALSE;
                }
                for(ListStarQuestion* p=pStarQuestion;p!=NULL;p=p->_pNext)
                {
                    if(p->_len-p->_nStars>len) return FALSE;
                }
                pQuestion=new ListQuestion(len,wchs);
            }else{
                if(len!=pQuestion->_len) return FALSE;
                PWCHAR pWchar=pQuestion->_wchs;
                for(int i=0;i<len;++i){//两者都可能含有? 比较是否相等，并做适当化简
                    if(L'?'==pWchar[i]){
                        if(L'?'!=wchs[i]){
                            pWchar[i]=wchs[i];//去问号，化简
                        }
                    }else{
                        if(L'?'!=wchs[i]){
                            if(pWchar[i]!=wchs[i]) return FALSE;
                        }                  
                    }
                }     
            }         
            return TRUE;
        }else if(STAR_TAG==type){//* 去掉多余的*号
            if(len<=nStars) return TRUE;//全星无效        
            if(pQuestion){//仅含? 的通配符限定长度
                if(len-nStars>pQuestion->_len) return FALSE;
            }
            pStar=new ListStar(0,0,0,0,pStar);
            PWCHAR pWch=pStar->_wchs; int &_len=pStar->_len;  
            if(*wchs!=L'*') {
                pStar->_type|=LEFT;
            }else pStar->_nStars++; //左边是星号，*pWch=*wchs==L'*'     
            if(wchs[len-1]!=L'*') pStar->_type|=RIGHT;
            *pWch=*wchs;
            for(int i=1;i<len;++i){
                if(L'*'==wchs[i]){
                    if(L'*'!=*pWch) {
                        *(++pWch)=L'*';
                        pStar->_nStars++;
                    }
                }else{
                    *(++pWch)=wchs[i];
                }  
            }
            *(++pWch)=L'\0';
            _len=pWch-pStar->_wchs;
            return TRUE;
        }else if((QUESTION_TAG|STAR_TAG)==type){//?*
            if(len<=nStars) return FALSE;//全星无效
            if(pQuestion){//仅含? 的通配符限定长度
                if(len-nStars>pQuestion->_len) return FALSE;
            }
            pStarQuestion=new ListStarQuestion(0,0,0,0,pStarQuestion);
            PWCHAR pWch=pStarQuestion->_wchs; int &_len=pStarQuestion->_len;   
            if(*wchs!=L'*') {
                pStarQuestion->_type|=LEFT;
            }else pStarQuestion->_nStars++; //左边是星号，*pWch=*wchs==L'*'   
            if(wchs[len-1]!=L'*') pStarQuestion->_type|=RIGHT;
            *pWch=*wchs;
            for(int i=1;i<len;++i){
                if(L'*'==wchs[i]){
                    if(L'*'!=*pWch) {
                        *(++pWch)=L'*';
                        pStarQuestion->_nStars++;
                    }
                }else{
                    *(++pWch)=wchs[i];
                }  
            }
            *(++pWch)=L'\0';
            _len=pWch-pStarQuestion->_wchs;
            return TRUE;
        }else return FALSE;
    }
    BOOL HasOnlyNormal(){//仅是普通串的组合 无通配符，多数是此种情况
        return pNormal!=NULL&&NULL==pQuestion&&NULL==pStar&&NULL==pStarQuestion;
    }
    BOOL HasQuestion(){return pQuestion!=NULL;}
    BOOL HasStar(){return pStar!=NULL;}
    BOOL HasStarQuestion(){return pStarQuestion!=NULL;}

    ListNormal *pNormal;    //普通串链，长度递减  
    ListQuestion *pQuestion;    //?串链
    ListStar *pStar;    //*串链
    ListStarQuestion *pStarQuestion;    //?*串链
    BOOL _bAllChar;//搜索框串中都是ASCII字符么？
    //若_bAllChar为TRUE，将文件库中的所有>0x80字符转换为字符?，进而实现AC算法
    //若_bAllChar为FALSE，即包含>0x7F的字符，此时满足条件的文件名很少，仅需蛮力搜索
};


extern SearchStrOpt g_StrOptCase,g_StrOptNoCase;
/**
*  Function:
*      整理新串
*  Parameter:
*      输入的串
**/
__forceinline BOOL ArrangeSearchStrCase(IN PWCHAR szSearchStr)
{
    BOOL bRet=FALSE;
    g_StrOptCase.Reset();  
    PWCHAR  p=szSearchStr;
    for(;L' '==*p;++p);//寻找第一个非空格
    while(*p)
    {
        BYTE type=0;
        PWCHAR pBeg=p,pEnd=p;
        int nStars=0;//星号个数
        for(;*pEnd && L' '!=*pEnd;++pEnd){//寻找第一个空格或0
            if(L'?'==*pEnd) type|=QUESTION_TAG;
            else if(L'*'==*pEnd) {
                type|=STAR_TAG;
                nStars++;
            }else if(*pEnd>0x7F){g_StrOptCase._bAllChar=FALSE;}
        }
        if(!g_StrOptCase.AddNode(type,pEnd-pBeg,pBeg,nStars)) return FALSE;
        bRet=TRUE;
        p=pEnd;
        for(;L' '==*p;++p);//寻找第一个非空格
    }
    return bRet;
}

//大小写不敏感
__forceinline BOOL ArrangeSearchStrNoCase(IN PWCHAR szSearchStr)
{
    BOOL bRet=FALSE;
    g_StrOptNoCase.Reset();  
    PWCHAR  p=szSearchStr;
    for(;L' '==*p;++p);//寻找第一个非空格
    while(*p)
    {
        BYTE type=0;
        PWCHAR pBeg=p,pEnd=p;
        int nStars=0;//星号个数
        for(;*pEnd && L' '!=*pEnd;++pEnd){//寻找第一个空格或0
            if(L'?'==*pEnd) type|=QUESTION_TAG;
            else if(L'*'==*pEnd) {
                type|=STAR_TAG;
                nStars++;
            }else if(*pEnd>0x7F){
                g_StrOptNoCase._bAllChar=FALSE;
            }else if(*pEnd>='A'&&*pEnd<='Z'){//转为小写
                *pEnd+=32;
            }
        }
        if(!g_StrOptNoCase.AddNode(type,pEnd-pBeg,pBeg,nStars)) return FALSE;
        bRet=TRUE;//成功插入一个串
        p=pEnd;
        for(;L' '==*p;++p);//寻找第一个非空格
    }
    return bRet;
}



inline BOOL Helper_CheckStarQuestionSearch(BOOL bCase,PWCHAR ucs2Name,int ucs2Len)
{
    ///搜索开始 
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;       
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        for(int i=0;i<ucs2Len;++i) {
            if(ucs2Name[i]<='Z') ucs2Name[i]=g_NoCaseTable[ucs2Name[i]];
        }
    }

    ListNormal          *pNormal;   
    ListStarQuestion    *pStarQuestion;

    BOOL bSearch=TRUE;
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
    } 
    return bSearch;
}
inline BOOL Helper_CheckStarSearch(BOOL bCase,PWCHAR ucs2Name,int ucs2Len)
{
    ///搜索开始 
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;       
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        for(int i=0;i<ucs2Len;++i) {
            if(ucs2Name[i]<='Z') ucs2Name[i]=g_NoCaseTable[ucs2Name[i]];
        }
    }

    ListStar            *pStar;
    ListNormal          *pNormal;   
    ListStarQuestion    *pStarQuestion;

    BOOL bSearch=TRUE;
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
        }    
    } 
    return bSearch;
}
/**
*	Function:
*      搜索串含有纯?时调用，可能含有普通串、纯?、*?混合
*      检查本串是否与查询串匹配，监视时使用
*	Parameter(s):
*      bCase,大小写敏感么?
*	Return:	
*      无
*	Commons:    
**/
inline BOOL Helper_CheckQuestionSearch(BOOL bCase,PWCHAR ucs2Name,int ucs2Len)
{
    ///搜索开始 
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;       
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        for(int i=0;i<ucs2Len;++i) {
            if(ucs2Name[i]<='Z') ucs2Name[i]=g_NoCaseTable[ucs2Name[i]];
        }
    }

    ListQuestion *pQuestion=pStrOpt->pQuestion;
    PWCHAR pQueWch=pQuestion->_wchs;
    int _nLen=pQuestion->_len;//文件长度限制 

    ListNormal          *pNormal;
    ListStar            *pStar;
    ListStarQuestion    *pStarQuestion;

    BOOL bSearch=FALSE;

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
                }    
            }
        }               
    }  
    return bSearch;
}

/**
*	Function:
*	Parameter(s):
*      bCase,大小写敏感么?
*       bHanzi (pDir->FileAttribute>=0x80)
*	Return:	
*      无
*	Commons:    
**/
inline BOOL Helper_CheckOnlyNormalSearch(BOOL bCase,PWCHAR ucs2Name,int ucs2Len)
{
    ///搜索开始 
    SearchStrOpt *pStrOpt;
    if(bCase) {
        pStrOpt=&g_StrOptCase;       
    }
    else {
        pStrOpt=&g_StrOptNoCase;
        for(int i=0;i<ucs2Len;++i) {
            if(ucs2Name[i]<='Z') ucs2Name[i]=g_NoCaseTable[ucs2Name[i]];
        }
    }

    int _nMaxLen=pStrOpt->pNormal->_len;//最大长度，小于此长度的文件跳过 
    ListNormal* pNormal;

    BOOL bSearch=FALSE;

    if(ucs2Len>=_nMaxLen){
        bSearch=TRUE;
        for(pNormal=pStrOpt->pNormal;pNormal;pNormal=pNormal->_pNext)
        {
            if(!Helper_MatchSubString(ucs2Name,ucs2Len,pNormal->_wchs,pNormal->_len)){
                bSearch=FALSE;break;
            }
        }
    } 
    return bSearch;
}



void Helper_InitNoCaseTable();

void Helper_OnlyNormalSearchAscii(BOOL bCase
                                  ,BOOL bDirSearch,BOOL bFileSearch
                                  ,int *pIdExt,int nExt
                                  ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                                  );


//使用暴力匹配，仅含普通模式串 含>0x80字符 大小写敏感
void Helper_OnlyNormalSearch(BOOL bCase
                             ,BOOL bDirSearch,BOOL bFileSearch
                             ,int *pIdExt,int nExt
                             ,PDIRECTORY_RECORD* ppDir,int *pbSubDir,int nCheck
                             );


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
                           );
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
                       );

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
                               );

void KernelSearch();












