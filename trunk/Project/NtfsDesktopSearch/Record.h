// Record.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "global.h"

#define GetBasicInfo(dwDri,frn) (((dwDri)<<27)|(DWORD)(frn))
#define GetDirectoryRecordLength(dwCodeNameLen) ((dwCodeNameLen)+10)
#define GetExtIdLength(dwExtId) ((dwExtId)<0x100 ? 1:((dwExtId)<0x10000 ? 2:4))
#define GetNameLenLength(dwCodeNameLen) ((dwCodeNameLen)<0x100?1:2)

//dwCodeNameLenΪ�����ļ�������
#define GetNormalFileRecordLength(dwCodeNameLen,dwNameLenLength,dwExtIdLength,dwIconLen) (8+(dwCodeNameLen)+(dwNameLenLength)+(dwExtIdLength)+(dwIconLen))
    

struct DirectoryRecord
{
    static const  WORD BITMASK_NONASCII=0x8000;
    static const  WORD BITMASK_NAMElEN=0x7FFF;

    DWORD           BasicInfo;          //��5λΪ��� ''-'A' ~ ''-'Z'

    union
    {
        DirectoryRecord*        ParentPtr;          //��Ŀ¼��ָ��
        DWORD                   ParentOffset;       //��Ŀ¼��ƫ��(�������ݿ⵼������)
        DWORD                   ParentBasicInfo;    //���ڳ�ʼʱ����Ŀ¼��
    }; 
    WORD            NameLength;         //���λ��ʾĿ¼�ļ����Ƿ���>0x7F�ַ�
                                        //����ʾ�ļ�����
    BYTE            Name[1];            //������ �ļ���

    //��pAddr��д��ѹ������
    //�ļ����������ֳ���ΪdwCommLen
    //����д���ֽ���
    DWORD WriteCompressData(DWORD dwCommLen,OUT PBYTE pAddr)
    {
        PBYTE pBase=pAddr;
        *(DWORDLONG*)pAddr=*(DWORDLONG*)this;
        pAddr+=8;
        //ʹ�����ֽڴ洢�ļ��������Լ�0x7F��Ϣ
        if(NameLength&BITMASK_NONASCII)
        {
            NameLength&=BITMASK_NAMElEN;
            *(DWORD*)pAddr=(dwCommLen|((NameLength-dwCommLen)<<10)|(1<<20));
        }
        else
        {
            *(DWORD*)pAddr=(dwCommLen|((NameLength-dwCommLen)<<10));
        }
        pAddr+=3;
        for(PBYTE p=Name+dwCommLen,pEnd=Name+NameLength;p<pEnd;++p)
        {
            *pAddr++=*p;
        }
        return pAddr-pBase;
    }

    void InitializeData(DWORD dwDri,DWORDLONG frn,DWORD parent_frn,PBYTE pCodeName,DWORD dwCodeNameLen,BOOL bNonAscii)
    {
        if((frn&0xffffffff)>0x7ffffff)
        {
            MessageBox(0,0,0,0);
        }
        BasicInfo=GetBasicInfo(dwDri,frn);
        ParentBasicInfo=GetBasicInfo(dwDri,parent_frn);
        NameLength=(WORD)dwCodeNameLen;
        for(PBYTE pName=Name,pNameEnd=Name+dwCodeNameLen;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }       
        if(bNonAscii) NameLength|=BITMASK_NONASCII;
    }

    void SetData(DWORD dwDri,DWORDLONG frn,DirectoryRecord* pParent,PBYTE pCodeName,DWORD dwCodeNameLen,BOOL bNonAscii)
    {
        BasicInfo=GetBasicInfo(dwDri,frn);
        ParentPtr=pParent;
        NameLength=(WORD)dwCodeNameLen;
        for(PBYTE pName=Name,pNameEnd=Name+dwCodeNameLen;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }       
        if(bNonAscii) NameLength|=BITMASK_NONASCII;
    }


    //������BasicInfo ���Լ��ļ�����Ϣ�������ø�
    void SimpleSetData(DWORD dwDri,DWORDLONG frn,PBYTE pCodeName,DWORD dwCodeNameLen,BOOL bNonAscii)
    {
        BasicInfo=GetBasicInfo(dwDri,frn);
        NameLength=(WORD)dwCodeNameLen;
        for(PBYTE pName=Name,pNameEnd=Name+dwCodeNameLen;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }       
        if(bNonAscii) NameLength|=BITMASK_NONASCII;
    }

    DirectoryRecord& operator=(const DirectoryRecord& obj)
    {
        if(this!=&obj)
        {
            CopyMemory(this,&obj,obj.GetLength());
        }
        return *this;
    }
    BOOL HasNonAscii()const//���к��ֻ�>0x7F��?
    {
        return NameLength&BITMASK_NONASCII;
    }

    int GetLength()const//��ȡ����¼����
    {
        return GetDirectoryRecordLength(NameLength&BITMASK_NAMElEN);
    }

    int GetCodeName(OUT PBYTE &pName)//��ȡ�����ļ������ȣ������׵�ַ
    {
        pName=Name;
        return NameLength&BITMASK_NAMElEN;
    }
     
    int GetUnicodeName(OUT PWCHAR pUcs2)const//����UNICODE�ַ�����
    {
        return Helper_CodeToUcs2Case(pUcs2,(PBYTE)Name,NameLength&BITMASK_NAMElEN);
    }

};

typedef DirectoryRecord DIRECTORY_RECORD,*PDIRECTORY_RECORD;


//��ͨ�ļ���¼
struct NormalFileRecord
{
    static const  DWORD BITMASK_NONASCII=0x80000000;
    static const  DWORD BITMASK_TWOBYTE_NAMELEN=0x40000000;
    static const  DWORD BITMASK_EXTTENDNAME=0x30000000;
    static const  DWORD BITMASK_ONEBYTE_EXTENDNAME=0x10000000;
    static const  DWORD BITMASK_TWOBYTE_EXTENDNAME=0x20000000;
    static const  DWORD BITMASK_ICON=0x8000000;//����ICON��
    static const  DWORD BITMASK_FOURBYTE_EXTENDNAME=BITMASK_EXTTENDNAME;

    DWORD           BasicInfo;          //���λ��ʾ�ļ����Ƿ���>0x7F�ַ� 1��ʾ����
                                        //�θ�λΪ0��ʾ�ļ�����ռ1�ֽ� 1�����ļ�����ռ2�ֽ� 
                                        //�ε�2λ����ʾ��չ���ֽ���0 1 2 4
                                        //��λ��ʾFRN_ID
    union
    {
        PDIRECTORY_RECORD   ParentPtr;          //��Ŀ¼��ָ��
        DWORD               ParentOffset;       //��Ŀ¼��ƫ��(�������ݿ⵼������)
        DWORD               ParentBasicInfo;    //���ڳ�ʼʱ����Ŀ¼��
    }; 
    BYTE            NameInformation[1]; //�洢��ʽ:�ļ������볤��+�ļ�������+[��չ��ID ��ext����]

    //��pAddr��д��ѹ������
    //�ļ����������ֳ���ΪdwCommLen
    //����д���ֽ���
    DWORD WriteCompressData(DWORD dwCommLen,PBYTE pAddr)
    {
        PBYTE pBase=pAddr;
        *(DWORDLONG*)pAddr=*(DWORDLONG*)this; //д��ͷ8�ֽ�
        pAddr+=8;
        PBYTE Name;
        DWORD NameLength;
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            Name=NameInformation+2;
            NameLength=*(WORD*)NameInformation;
        }else{
            Name=NameInformation+1;
            NameLength=*NameInformation;
        }
        DWORD *pTag=(DWORD*)pAddr;
        pAddr+=3;
        *pTag=(dwCommLen|((NameLength-dwCommLen)<<10));
        //ʹ�����ֽڴ洢�ļ������� 10���� 10ʣ�³� 4��չ����       
        PBYTE p=Name+dwCommLen,pEnd=Name+NameLength;
        for(;p<pEnd;++p)
        {
            *pAddr++=*p;
        }
        DWORD dwExtIdLength=(BasicInfo&BITMASK_EXTTENDNAME)>>28;
        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1�ֽ�
                *pAddr++=*pEnd;
                *pTag|=(1<<20);
            }else{
                if(2==dwExtIdLength){//2�ֽ�
                    *(WORD*)pAddr=*(WORD*)pEnd;
                    pAddr+=2;
                    *pTag|=(2<<20);
                }else{//4�ֽ�
                    *(DWORD*)pAddr=*(DWORD*)pEnd;
                    pAddr+=4;
                    *pTag|=(4<<20);
                }
            }
        }
        //���д����չ��
        return pAddr-pBase;
    }


    //ע�⣬��0==dwExtIdLengthʱ�������dwExtId��Ч
    void InitializeData(BOOL bNonAscii,DWORD dwNameLenLength,DWORD dwExtIdLength,DWORDLONG frn
        ,DWORD dwDri,DWORDLONG parent_frn,PBYTE pCodeName,DWORD dwCodeNameLen,DWORD dwExtId)
    {
        if((frn&0xffffffff)>0x7ffffff)
        {
            MessageBox(0,0,0,0);
        }
        PBYTE pName=NameInformation,pNameEnd;
        BasicInfo=(DWORD)frn;
        if(bNonAscii) BasicInfo|=BITMASK_NONASCII;
        if(2==dwNameLenLength) {
            BasicInfo|=BITMASK_TWOBYTE_NAMELEN;
            *(WORD*)pName=(WORD)dwCodeNameLen;
            pName+=2;           
        }else{
            *pName=(BYTE)dwCodeNameLen;
            ++pName;
        }
        pNameEnd=pName+dwCodeNameLen;

        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1�ֽ�
                BasicInfo|=BITMASK_ONEBYTE_EXTENDNAME; 
                *pNameEnd=(BYTE)dwExtId;
            }else{
                if(2==dwExtIdLength){//2�ֽ�
                    BasicInfo|=BITMASK_TWOBYTE_EXTENDNAME; 
                    *(WORD*)pNameEnd=(WORD)dwExtId;
                }else{//4�ֽ�
                    BasicInfo|=BITMASK_FOURBYTE_EXTENDNAME;  
                    *(DWORD*)pNameEnd=dwExtId;
                }
            }
            if(dwExtId<CExtArray::s_dwOmitExt) {
                BasicInfo|=BITMASK_ICON;//��ʼ��ICON BIT
                *(DWORD*)(pNameEnd+dwExtIdLength)=0xFFFFFFFF;//��ʼ��ICON
            }
        }

        ParentBasicInfo=GetBasicInfo(dwDri,parent_frn);
        for(;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }   
    }

    //�����ļ�ʱ����
    void SetData(BOOL bNonAscii,DWORD dwNameLenLength,DWORD dwExtIdLength,DWORDLONG frn
        ,DWORD dwDri,PDIRECTORY_RECORD pParent,PBYTE pCodeName,DWORD dwCodeNameLen,DWORD dwExtId)
    {
        PBYTE pName=NameInformation,pNameEnd;
        BasicInfo=(DWORD)frn;
        if(bNonAscii) BasicInfo|=BITMASK_NONASCII;
        if(2==dwNameLenLength) {
            BasicInfo|=BITMASK_TWOBYTE_NAMELEN;
            *(WORD*)pName=(WORD)dwCodeNameLen;
            pName+=2;           
        }else{
            *pName=(BYTE)dwCodeNameLen;
            ++pName;
        }
        pNameEnd=pName+dwCodeNameLen;

        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1�ֽ�
                BasicInfo|=BITMASK_ONEBYTE_EXTENDNAME; 
                *pNameEnd=(BYTE)dwExtId;
            }else{
                if(2==dwExtIdLength){//2�ֽ�
                    BasicInfo|=BITMASK_TWOBYTE_EXTENDNAME; 
                    *(WORD*)pNameEnd=(WORD)dwExtId;
                }else{//4�ֽ�
                    BasicInfo|=BITMASK_FOURBYTE_EXTENDNAME;  
                    *(DWORD*)pNameEnd=dwExtId;
                }
            }
            if(dwExtId<CExtArray::s_dwOmitExt) {
                BasicInfo|=BITMASK_ICON;//��ʼ��ICON BIT
                *(DWORD*)(pNameEnd+dwExtIdLength)=0xFFFFFFFF;//��ʼ��ICON
            }
        }
        ParentPtr=pParent;
        for(;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }          
    }

    //������BasicInfo ���Լ��ļ�����Ϣ�������ø�
    //����ɾ���ļ�ʱ ���� ����BasicInfo��Ϣ�Լ��ļ��� ��չ����Ϣ ����ȫ
    //�����븸ָ�� �Լ�ICON
    void SimpleSetData(BOOL bNonAscii,DWORD dwNameLenLength,DWORD dwExtIdLength,DWORDLONG frn
        ,DWORD dwDri,PBYTE pCodeName,DWORD dwCodeNameLen,DWORD dwExtId)
    {
        PBYTE pName=NameInformation,pNameEnd;
        BasicInfo=(DWORD)frn;
        if(bNonAscii) BasicInfo|=BITMASK_NONASCII;
        if(2==dwNameLenLength) {
            BasicInfo|=BITMASK_TWOBYTE_NAMELEN;
            *(WORD*)pName=(WORD)dwCodeNameLen;
            pName+=2;           
        }else{
            *pName=(BYTE)dwCodeNameLen;
            ++pName;
        }
        pNameEnd=pName+dwCodeNameLen;

        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1�ֽ�
                BasicInfo|=BITMASK_ONEBYTE_EXTENDNAME; 
                *pNameEnd=(BYTE)dwExtId;
            }else{
                if(2==dwExtIdLength){//2�ֽ�
                    BasicInfo|=BITMASK_TWOBYTE_EXTENDNAME; 
                    *(WORD*)pNameEnd=(WORD)dwExtId;
                }else{//4�ֽ�
                    BasicInfo|=BITMASK_FOURBYTE_EXTENDNAME;  
                    *(DWORD*)pNameEnd=dwExtId;
                }
            }
            if(dwExtId<CExtArray::s_dwOmitExt) {
                BasicInfo|=BITMASK_ICON;
                //����ICONֵ�ɲ�������
            }
        }
        for(;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }          
    }
    
    
    int GetLength()const//��ȡ��¼����
    {
        DWORD dwExtIdLength=(BasicInfo&BITMASK_EXTTENDNAME)>>28;
        DWORD dwIconLen=0;
        if(BasicInfo&BITMASK_ICON) dwIconLen=4;
        if(3==dwExtIdLength) dwExtIdLength=4;
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            return GetNormalFileRecordLength(*(WORD*)NameInformation,2,dwExtIdLength,dwIconLen); 
        }else{
            return GetNormalFileRecordLength(*NameInformation,1,dwExtIdLength,dwIconLen); 
        }
    }

    NormalFileRecord& operator=(const NormalFileRecord& obj)
    {
        if(this!=&obj)
        {
            CopyMemory(this,&obj,obj.GetLength());
        }
        return *this;
    }

    BOOL HasNonAscii()const//���к��ֻ�>0x7F��?
    {
        return BasicInfo&BITMASK_NONASCII;
    }
    
    BOOL HasExtendName()const
    {
        return BasicInfo&BITMASK_EXTTENDNAME;
    }

    //������չ��ID
    //�������ֵΪ-1 ��ʾ����չ��
    //������չ����ռ���ڴ��ֽ���pExtLen
    DWORD GetExtendID(PBYTE pNameEnd,DWORD *pExtLen=NULL)
    {
        DWORD dwExtIdLength=(BasicInfo&BITMASK_EXTTENDNAME)>>28;
        if(pExtLen) *pExtLen=dwExtIdLength;
        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1�ֽ�
                return *pNameEnd;
            }else{
                if(2==dwExtIdLength){//2�ֽ�
                    return *(WORD*)pNameEnd;
                }else{//4�ֽ�
                    return  *(DWORD*)pNameEnd;
                }
            }
        }
        return -1;
    }

    DWORD GetExtendID()
    {
        PBYTE pName;
        int len=GetCodeName(pName);//��ȡ�����ļ������ȣ������׵�ַ
        return GetExtendID(pName+len);
    } 
    
    int GetCodeName(OUT PBYTE &pName)//��ȡ�����ļ������ȣ������׵�ַ
    {
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            pName=NameInformation+2;
            return *(WORD*)NameInformation;
        }else{
            pName=NameInformation+1;
            return *NameInformation;
        }
    }

    int GetUnicodeName(OUT PWCHAR pUcs2)const
    {
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            return Helper_CodeToUcs2Case(pUcs2,PBYTE(NameInformation+2),*(WORD*)NameInformation);
        }else{
            return Helper_CodeToUcs2Case(pUcs2,PBYTE(NameInformation+1),*NameInformation);
        }
    }

    DWORD GetIcon()
    {

    }

    void SetIcon(DWORD iIcon)
    {

    }
    
};

typedef NormalFileRecord NORMALFILE_RECORD,*PNORMALFILE_RECORD;

//��ȡ�ļ�����Ŀ¼,��L'\\'��β
//����ļ���c:�ȣ��丸Ŀ¼Ϊ��
__forceinline int Helper_GetPath(
            OUT WCHAR path[]                    //��ȡ��·��
            ,IN PDIRECTORY_RECORD ParentPtr     //��Ŀ¼ָ��
)
{
    static WCHAR dirFileName[MAX_PATH];//������ʾ����߳���ʹ�ñ�����������������̷߳��ʴ���
    int nameLen;
    int i,j,iPath=0;
    PDIRECTORY_RECORD pRecord=NULL;

    for(;ParentPtr!=0;ParentPtr=pRecord->ParentPtr)
    {
        pRecord=PDIRECTORY_RECORD(ParentPtr);
        path[iPath++]=L'\\';
        nameLen=pRecord->GetUnicodeName(dirFileName);
        for(i=nameLen-1;i>=0;--i)
        {
            path[iPath++]=dirFileName[i];
        }
    }   
    WCHAR wch;
    for(i=0,j=iPath-1;i<j;++i,--j)
    {
        wch=path[i];path[i]=path[j];path[j]=wch;
    }
    path[iPath]=L'\0';
    return iPath;
}

//��δ����
__forceinline int Helper_GetFullName(
            OUT WCHAR wszFullName[]             //��ȡ�ľ����ļ���
            ,IN PDIRECTORY_RECORD   ParentPtr   //��Ŀ¼ָ��
            ,IN PWCHAR fileName                 //��ǰUNICODE�ļ���
            ,IN int nameLen                     //�ļ����ļ�������UNICODE�ַ�
)
{    
    if(fileName[1]==L':'){
        wszFullName[0]=fileName[0];wszFullName[1]=L':';wszFullName[2]=L'\\';wszFullName[3]=L'\0';
        return 3;
    }
    int i,j,iPath=0;
    for(i=nameLen-1;i>=0;--i)
    {
        wszFullName[iPath++]=fileName[i];
    }
    static WCHAR dirFileName[MAX_PATH];
    PDIRECTORY_RECORD pRecord=NULL;
    for(;ParentPtr!=0;ParentPtr=pRecord->ParentPtr)
    {
        pRecord=PDIRECTORY_RECORD(ParentPtr);
        wszFullName[iPath++]=L'\\';
        nameLen=pRecord->GetUnicodeName(dirFileName);
        for(i=nameLen-1;i>=0;--i)
        {
            wszFullName[iPath++]=dirFileName[i];
        }
    }   
    WCHAR wch;
    for(i=0,j=iPath-1;i<j;++i,--j)
    {
        wch=wszFullName[i];wszFullName[i]=wszFullName[j];wszFullName[j]=wch;
    }
    wszFullName[iPath]=L'\0';
    return iPath;
}
