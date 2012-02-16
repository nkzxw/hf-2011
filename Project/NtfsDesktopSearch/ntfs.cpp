// ntfs.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "ntfs.h"


__forceinline ULONG RunLength(PUCHAR run)
{
    return(*run & 0x0f) + ((*run >> 4) & 0x0f) + 1;
}


//返回LCN-d-gap  注意d-gap可为负
__forceinline LONGLONG RunLCN(PUCHAR run)
{
    UCHAR n1 = *run & 0x0f;
    UCHAR n2 = (*run >> 4) & 0x0f;
    if(0==n2) return 0;
    LONGLONG lcn=(CHAR)(run[n1 + n2]);//带符号转换
    LONG i = 0;
    for (i = n1 +n2 - 1; i > n1; --i)
        lcn = (lcn << 8) + run[i];
    return lcn;
}

__forceinline ULONGLONG RunCount(PUCHAR run)
{
    UCHAR n =  *run & 0xf;
    ULONGLONG count = 0;
    ULONG i = 0;
    for (i = n; i > 0; i--)
        count = (count << 8) + run[i];
    return count;
}


/**
*	Function:
*      获取文件的时间(创建、最近访问、最近修改)和大小信息
*	Parameter(s):
*      BasicInfo 高5位是盘符 低27位是文件ID（FRN低48位）
*      ftCreate 32位文件创建系统时间
*      pfileSize 文件大小 为0表示不需要返回文件大小
*	Return:	
*      void
*	Commons:
*      其中大小信息获取较复杂，可能需要直接访问MFT
**/

extern HANDLE g_hVols[];
extern DWORD g_BytesPerCluster[];
extern DWORD g_FileRecSize[];
extern PBYTE g_pOutBuffer[];

//同时获取多个所需要的文件属性
void Helper_GetBasicInformation(
                                DWORD iDri//盘号
                                ,DWORD dwFrn
                                ,DWORDLONG *pOutTimeCreate
                                ,DWORDLONG *pOutTimeAccess
                                ,DWORDLONG *pOutTimeChange
                                ,DWORDLONG *pOutSize
                                ,DWORD *pOutAttr
                                )
{
    HANDLE hVol=g_hVols[iDri];
    DWORD dwBytesPerCluster=g_BytesPerCluster[iDri];
    DWORD dwFileRecSize=g_FileRecSize[iDri];

    PNTFS_FILE_RECORD_OUTPUT_BUFFER pMftRecord=(PNTFS_FILE_RECORD_OUTPUT_BUFFER)g_pOutBuffer[iDri];
    PFILE_RECORD_HEADER pfileRecordheader=(PFILE_RECORD_HEADER)pMftRecord->FileRecordBuffer; 
    PATTRIBUTE pAttribute=NULL;
    PFILENAME_ATTRIBUTE pFileNameAttr;

    NTFS_FILE_RECORD_INPUT_BUFFER mftRecordInput;
    mftRecordInput.FileReferenceNumber.QuadPart=dwFrn;

    DWORD dwRet;
    DeviceIoControl(hVol,FSCTL_GET_NTFS_FILE_RECORD
        ,&mftRecordInput,sizeof(mftRecordInput)
        ,pMftRecord,dwFileRecSize,&dwRet,NULL);

    for(pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader +pfileRecordheader->AttributeOffset)
        ;pAttribute->AttributeType!=AttributeFileName
        ;pAttribute=(PATTRIBUTE)((PBYTE)pAttribute +pAttribute->Length)
        );
    pFileNameAttr=PFILENAME_ATTRIBUTE(
        (PBYTE)pAttribute+PRESIDENT_ATTRIBUTE(pAttribute)->ValueOffset
        );

    if(pOutTimeAccess) *pOutTimeAccess=pFileNameAttr->LastAccessTime;
    if(pOutTimeCreate) *pOutTimeCreate=pFileNameAttr->CreationTime;
    if(pOutTimeChange) *pOutTimeChange=pFileNameAttr->LastWriteTime;
    if(pOutAttr) *pOutAttr=pFileNameAttr->FileAttributes;

    if(NULL==pOutSize) return;
    assert(!(pFileNameAttr->FileAttributes&FILE_ATTRIBUTE_DIRECTORY) && "不能是目录");
    
    //以下获取文件大小信息
    if(pFileNameAttr->DataSize!=0){
        *pOutSize=pFileNameAttr->DataSize;
        return;
    }

    //30属性大小为0时可能不正确
    //读80属性获得文件大小
    for(pAttribute=(PATTRIBUTE)((PBYTE)pAttribute +pAttribute->Length)
        ;pAttribute->AttributeType!=AttributeEnd && pAttribute->AttributeType<AttributeData
        ;pAttribute=(PATTRIBUTE)((PBYTE)pAttribute +pAttribute->Length)
        ); 
    if(pAttribute->AttributeType==AttributeEnd || pAttribute->AttributeType>AttributeData){
        for(pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader +pfileRecordheader->AttributeOffset)
            ;pAttribute->AttributeType<AttributeAttributeList
            ;pAttribute=(PATTRIBUTE)((PBYTE)pAttribute +pAttribute->Length)
            );
        if(pAttribute->AttributeType>AttributeAttributeList){
            assert(0&&"没有找到80属性也没有20属性");
        }else{
            PATTRIBUTE_LIST pAttriList;
            if(pAttribute->Nonresident){
                PNONRESIDENT_ATTRIBUTE pNonResident=PNONRESIDENT_ATTRIBUTE(pAttribute);
                PBYTE pRun=(PBYTE)pAttribute+pNonResident->RunArrayOffset;       
                ULONGLONG Lcn = RunLCN( pRun );//只获取第1簇                              
                ULONGLONG nCount = RunCount( pRun ); 
                assert(nCount<=pNonResident->HighVcn-pNonResident->LowVcn+1);
                LARGE_INTEGER file_offset;
                file_offset.QuadPart=Lcn*dwBytesPerCluster;
                SetFilePointerEx(hVol,file_offset, NULL, FILE_BEGIN );
                PBYTE   pBuffferRead=g_MemoryMgr.GetMemory(dwBytesPerCluster);///第一簇                   
                DWORD   dwRead = 0;
                ReadFile(hVol,pBuffferRead,dwBytesPerCluster, &dwRead, NULL );
                PBYTE   pBufferEnd=pBuffferRead+dwRead;
                for(pAttriList=PATTRIBUTE_LIST(pBuffferRead);
                    pAttriList->AttributeType!=AttributeData;
                    pAttriList=PATTRIBUTE_LIST(PBYTE(pAttriList)+pAttriList->Length)
                    );
                assert(pAttriList->AttributeType==AttributeData && "难道不等?");
                if(pAttriList->AttributeType==AttributeData){
                    mftRecordInput.FileReferenceNumber.QuadPart=0xffffffffffff&pAttriList->FileReferenceNumber;
                    DeviceIoControl(hVol,FSCTL_GET_NTFS_FILE_RECORD
                        ,&mftRecordInput,sizeof(mftRecordInput)
                        ,pMftRecord,dwFileRecSize,&dwRet,NULL);
                    pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader +pfileRecordheader->AttributeOffset);
                    assert(AttributeData==pAttribute->AttributeType && "一定是80属性");
                    if(pAttribute->Nonresident){
                        *pOutSize=PNONRESIDENT_ATTRIBUTE(pAttribute)->DataSize;  
                    }else{
                        *pOutSize=PRESIDENT_ATTRIBUTE(pAttribute)->ValueLength; 
                    }                        
                }
                g_MemoryMgr.FreeMemory(pBuffferRead);
            }else{
                for(pAttriList=PATTRIBUTE_LIST((PBYTE)pAttribute+PRESIDENT_ATTRIBUTE(pAttribute)->ValueOffset);
                    pAttriList->AttributeType!=AttributeEnd && pAttriList->AttributeType<AttributeData;
                    pAttriList=PATTRIBUTE_LIST(PBYTE(pAttriList)+pAttriList->Length)
                    );
                if(pAttriList->AttributeType==AttributeData){
                    mftRecordInput.FileReferenceNumber.QuadPart=0xffffffffffff&pAttriList->FileReferenceNumber;
                    DeviceIoControl(hVol,FSCTL_GET_NTFS_FILE_RECORD
                        ,&mftRecordInput,sizeof(mftRecordInput)
                        ,pMftRecord,dwFileRecSize,&dwRet,NULL);
                    pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader +pfileRecordheader->AttributeOffset);
                    assert(AttributeData==pAttribute->AttributeType && "一定是80属性");
                    if(pAttribute->Nonresident){
                        *pOutSize=PNONRESIDENT_ATTRIBUTE(pAttribute)->DataSize;  
                    }else{
                        *pOutSize=PRESIDENT_ATTRIBUTE(pAttribute)->ValueLength; 
                    }                       
                }else{
                    assert(0&&"属性列表中没有80属性");
                }
            }
        }
    }else{
        if(pAttribute->Nonresident){
            *pOutSize=PNONRESIDENT_ATTRIBUTE(pAttribute)->DataSize;  
        }else{
            *pOutSize=PRESIDENT_ATTRIBUTE(pAttribute)->ValueLength; 
        }  
    }
}


