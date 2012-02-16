// NameSort.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"


extern CExtArray g_ExtMgr;

//初始化排序数据
//将数据文件导入，以便排序
const int _min=19968;
const int _max=40869;
const int _count=_max-_min+1;
//_min表示第一个汉字的UNICODE _max表示最后一个汉字的UNICODE
//_min=19968  为【一】字  _max=40869 为 【龥】字
WORD* g_sortTable;//[_count];//存储汉字位次，表示汉字i应该排在第g_sortTable[i]个位置

//比较两个code后的文件名字符串大小
//字母不区分大小写
//汉字按拼音有序
int comp_chinese_name(PBYTE pName1,PBYTE const pName1End,PBYTE pName2,PBYTE const pName2End)
{  
    while(pName1<pName1End && pName2<pName2End)
    {
        if(*pName1<0x80){                
            if(*pName2<0x80){
                BYTE ch1,ch2;
                ch1=g_NoCaseTable[*pName1];      
                ch2=g_NoCaseTable[*pName2]; 
                if(ch1==ch2) {++pName1;++pName2;}
                else if(ch1<ch2) return -1;
                else return 1;
            }else{
                return -1;
            }
        }else{
            if(*pName2<0x80){
                return 1;
            }else{//比较两个>0x80的编码
                WCHAR a=*(PWCHAR)(pName1+1),b=*(PWCHAR)(pName2+1);
                if(a==b) {pName1+=3;pName2+=3;}
                else {
                    if(a>=_min &&a<=_max){
                        if(b>=_min && b<=_max){
                            return g_sortTable[a-_min]-g_sortTable[b-_min];
                        }else return -1;
                    }else{
                        if(b>=_min && b<=_max){
                            return 1;
                        }else return a-b;
                    } 
                }             
            }
        }
    }

    if(pName1==pName1End){
        if(pName2<pName2End) return -1;
        else return 0;
    }else return 1;
}

//注意，此处按照区分大小写来排序
int comp_normal_name(PBYTE pName1,PBYTE const pName1End,PBYTE pName2,PBYTE const pName2End)
{
    while(pName1<pName1End && pName2<pName2End)
    {
        if(*pName1==*pName2) {++pName1;++pName2;}
        else if(*pName1<*pName2) return -1;
        else return 1;
    }
    if(pName1==pName1End){
        if(pName2<pName2End) return -1;
        else return 0;
    }else return 1;
}


PComp comp_name;//文件名比较函数


BOOL Help_InitCompare()
{    
    HRSRC hRsrc=::FindResourceW(NULL,MAKEINTRESOURCE(IDR_ZISORT4),L"ZISORT");
    HGLOBAL hGlobal = LoadResource(NULL,hRsrc);
    g_sortTable=(WORD*)LockResource(hGlobal);
    assert(sizeof(WORD)*_count==SizeofResource(NULL,hRsrc));
    //CopyMemory(g_sortTable,pData,sizeof(WORD)*_count);
    comp_name=comp_chinese_name;



//     pData=DeCompress((char*)LockResource(hGlobal)
//         ,SizeofResource(NULL,hRsrc)
// 
// 
//     HANDLE hFile=CreateFileW(CURDIR L"zi_sort.dat"
//         ,GENERIC_READ|GENERIC_WRITE
//         ,FILE_SHARE_READ|FILE_SHARE_WRITE
//         ,NULL
//         ,OPEN_EXISTING
//         ,FILE_ATTRIBUTE_NORMAL
//         ,NULL);
//     if(INVALID_HANDLE_VALUE==hFile){
//         DebugStringA("zi_sort文件打开失败:%d",GetLastError());
//         comp_name=comp_normal_name;
//         return FALSE;
//     }
//     DWORD dwRead;
//     if(ReadFile(hFile,g_sortTable,sizeof(WORD)*_count,&dwRead,NULL)){
//         comp_name=comp_chinese_name;
//     }else{
//         comp_name=comp_normal_name;
//     }
//     CloseHandle(hFile);  
    return TRUE;
}

#ifdef MS_QSORT
int pcomp_dir(const void*a,const void *b)
{
    PBYTE pName1=PDIRECTORY_RECORD(g_pDirBuffer+*(PDWORD)a)->NameInformation;
    PBYTE pName2=PDIRECTORY_RECORD(g_pDirBuffer+*(PDWORD)b)->NameInformation;
    int nameLen1,nameLen2;
    if(*pName1<0x80){
        nameLen1=*pName1++;
    }else{
        nameLen1=((0x7f&*pName1++)<<8);
        nameLen1+=*pName1++;
    }
    if(*pName2<0x80){
        nameLen2=*pName2++;
    }else{
        nameLen2=((0x7f&*pName2++)<<8);
        nameLen2+=*pName2++;
    }
    return comp_name(pName1,pName1+nameLen1,pName2,pName2+nameLen2);   
}
int pcomp_file(const void*a,const void *b)
{
    PBYTE pName1=PNORMALFILE_RECORD(g_pFileBuffer+*(PDWORD)a)->NameInformation;
    PBYTE pName2=PNORMALFILE_RECORD(g_pFileBuffer+*(PDWORD)b)->NameInformation;
    int nameLen1,nameLen2;
    if(*pName1<0x80){
        nameLen1=*pName1++;
    }else{
        nameLen1=((0x7f&*pName1++)<<8);
        nameLen1+=*pName1++;
    }
    if(*pName2<0x80){
        nameLen2=*pName2++;
    }else{
        nameLen2=((0x7f&*pName2++)<<8);
        nameLen2+=*pName2++;
    }
    PBYTE pName1End=pName1+nameLen1,pName2End=pName2+nameLen2;
    int cmp=comp_name(pName1,pName1End,pName2,pName2End); 
    if(0==cmp){//文件名一样，看扩展名
        int idExt1,idExt2; //0表示无图标
        int idExt;
        PBYTE pName;
        pName=pName1End;
        if(*pName<0x80){
            idExt=*pName;
        }else{
            idExt=0x7f&*pName++;
            if(*pName<0x80){
                idExt|=*pName<<7;
            }else{
                idExt|=(0x7f&*pName++)<<7;
                if(*pName<0x80){
                    idExt|=*pName<<14;
                }else{
                    idExt|=(0x7f&*pName++)<<14;
                    if(*pName<0x80){
                        idExt|=*pName<<21;
                    }else{
                        idExt|=(0x7f&*pName++)<<21;
                    }
                }
            }
        }
        idExt1=idExt;

        pName=pName2End;
        if(*pName<0x80){
            idExt=*pName;
        }else{
            idExt=0x7f&*pName++;
            if(*pName<0x80){
                idExt|=*pName<<7;
            }else{
                idExt|=(0x7f&*pName++)<<7;
                if(*pName<0x80){
                    idExt|=*pName<<14;
                }else{
                    idExt|=(0x7f&*pName++)<<14;
                    if(*pName<0x80){
                        idExt|=*pName<<21;
                    }else{
                        idExt|=(0x7f&*pName++)<<21;
                    }
                }
            }
        }
        idExt2=idExt;
        if(idExt1==idExt2){//扩展名相等是
            return 0;
        }
        else{
            if(0==idExt1) return 1;//1无扩展名          
            else if(0==idExt2) return -1;//2无扩展名
            else {
                return g_ExtMgr.Compare(idExt1-1,idExt2-1);//
            }
        }  
    }
    return cmp;
}
#else
//比较两个目录
int comp_dir(IndexElemType a,IndexElemType b)
{
    PBYTE pName1,pName2;
    int nameLen1,nameLen2;
    nameLen1=PDIRECTORY_RECORD(a)->GetCodeName(pName1);
    nameLen2=PDIRECTORY_RECORD(b)->GetCodeName(pName2);
    return comp_name(pName1,pName1+nameLen1,pName2,pName2+nameLen2); 
}

//比较两个文件
int comp_file(IndexElemType a,IndexElemType b)
{
    PNORMALFILE_RECORD pFile1=PNORMALFILE_RECORD(a)
        ,pFile2=PNORMALFILE_RECORD(b);
    PBYTE pName1,pName2;
    int nameLen1,nameLen2;
    nameLen1=pFile1->GetCodeName(pName1);
    nameLen2=pFile2->GetCodeName(pName2);
    PBYTE pName1End=pName1+nameLen1,pName2End=pName2+nameLen2;
    int cmp=comp_name(pName1,pName1End,pName2,pName2End); 
    if(0==cmp){//文件名一样，看扩展名
        int idExt1=pFile1->GetExtendID(pName1End)
            ,idExt2=pFile2->GetExtendID(pName2End); //0表示无图标
        if(idExt1==idExt2){//扩展名相等是
            return 0;
        }
        else{
            if(-1==idExt1) return 1;//1无扩展名          
            else if(-1==idExt2) return -1;//2无扩展名
            else {
                return g_ExtMgr.Compare(idExt1,idExt2);//
            }
        }  
    }
    return cmp;
}

#endif