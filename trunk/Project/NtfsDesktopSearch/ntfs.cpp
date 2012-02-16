// ntfs.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "ntfs.h"


__forceinline ULONG RunLength(PUCHAR run)
{
    return(*run & 0x0f) + ((*run >> 4) & 0x0f) + 1;
}


//����LCN-d-gap  ע��d-gap��Ϊ��
__forceinline LONGLONG RunLCN(PUCHAR run)
{
    UCHAR n1 = *run & 0x0f;
    UCHAR n2 = (*run >> 4) & 0x0f;
    if(0==n2) return 0;
    LONGLONG lcn=(CHAR)(run[n1 + n2]);//������ת��
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
*      ��ȡ�ļ���ʱ��(������������ʡ�����޸�)�ʹ�С��Ϣ
*	Parameter(s):
*      BasicInfo ��5λ���̷� ��27λ���ļ�ID��FRN��48λ��
*      ftCreate 32λ�ļ�����ϵͳʱ��
*      pfileSize �ļ���С Ϊ0��ʾ����Ҫ�����ļ���С
*	Return:	
*      void
*	Commons:
*      ���д�С��Ϣ��ȡ�ϸ��ӣ�������Ҫֱ�ӷ���MFT
**/

extern HANDLE g_hVols[];
extern DWORD g_BytesPerCluster[];
extern DWORD g_FileRecSize[];
extern PBYTE g_pOutBuffer[];

//ͬʱ��ȡ�������Ҫ���ļ�����
void Helper_GetBasicInformation(
                                DWORD iDri//�̺�
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
    assert(!(pFileNameAttr->FileAttributes&FILE_ATTRIBUTE_DIRECTORY) && "������Ŀ¼");
    
    //���»�ȡ�ļ���С��Ϣ
    if(pFileNameAttr->DataSize!=0){
        *pOutSize=pFileNameAttr->DataSize;
        return;
    }

    //30���Դ�СΪ0ʱ���ܲ���ȷ
    //��80���Ի���ļ���С
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
            assert(0&&"û���ҵ�80����Ҳû��20����");
        }else{
            PATTRIBUTE_LIST pAttriList;
            if(pAttribute->Nonresident){
                PNONRESIDENT_ATTRIBUTE pNonResident=PNONRESIDENT_ATTRIBUTE(pAttribute);
                PBYTE pRun=(PBYTE)pAttribute+pNonResident->RunArrayOffset;       
                ULONGLONG Lcn = RunLCN( pRun );//ֻ��ȡ��1��                              
                ULONGLONG nCount = RunCount( pRun ); 
                assert(nCount<=pNonResident->HighVcn-pNonResident->LowVcn+1);
                LARGE_INTEGER file_offset;
                file_offset.QuadPart=Lcn*dwBytesPerCluster;
                SetFilePointerEx(hVol,file_offset, NULL, FILE_BEGIN );
                PBYTE   pBuffferRead=g_MemoryMgr.GetMemory(dwBytesPerCluster);///��һ��                   
                DWORD   dwRead = 0;
                ReadFile(hVol,pBuffferRead,dwBytesPerCluster, &dwRead, NULL );
                PBYTE   pBufferEnd=pBuffferRead+dwRead;
                for(pAttriList=PATTRIBUTE_LIST(pBuffferRead);
                    pAttriList->AttributeType!=AttributeData;
                    pAttriList=PATTRIBUTE_LIST(PBYTE(pAttriList)+pAttriList->Length)
                    );
                assert(pAttriList->AttributeType==AttributeData && "�ѵ�����?");
                if(pAttriList->AttributeType==AttributeData){
                    mftRecordInput.FileReferenceNumber.QuadPart=0xffffffffffff&pAttriList->FileReferenceNumber;
                    DeviceIoControl(hVol,FSCTL_GET_NTFS_FILE_RECORD
                        ,&mftRecordInput,sizeof(mftRecordInput)
                        ,pMftRecord,dwFileRecSize,&dwRet,NULL);
                    pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader +pfileRecordheader->AttributeOffset);
                    assert(AttributeData==pAttribute->AttributeType && "һ����80����");
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
                    assert(AttributeData==pAttribute->AttributeType && "һ����80����");
                    if(pAttribute->Nonresident){
                        *pOutSize=PNONRESIDENT_ATTRIBUTE(pAttribute)->DataSize;  
                    }else{
                        *pOutSize=PRESIDENT_ATTRIBUTE(pAttribute)->ValueLength; 
                    }                       
                }else{
                    assert(0&&"�����б���û��80����");
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


