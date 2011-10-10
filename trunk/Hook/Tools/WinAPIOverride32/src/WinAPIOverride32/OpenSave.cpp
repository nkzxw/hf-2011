/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: manages log saving and reloading
//-----------------------------------------------------------------------------

#include "OpenSave.h"

// main var and func
extern CLinkList* pLogList;
extern HANDLE mhLogHeap;

//-----------------------------------------------------------------------------
// Name: Load
// Object: Load a previously saved log file
// Parameters :
//     in  : TCHAR* pszFile : name of the log file
//     out :
//     return : TRUE on load success, FALSE else
//-----------------------------------------------------------------------------
BOOL COpenSave::Load(TCHAR* pszFile)
{
    BOOL bUnicodeSave;
    TCHAR* pszContent;
    TCHAR* pszCurrentMarkup;
    TCHAR* pszNextMarkup;
    TCHAR* pszNextMarkupLog;
    TCHAR* pszNextMarkupParam;
    TCHAR* pszUpperMarkup;
    TCHAR* pszBuffer;
    TCHAR* pszLog;
    TCHAR* pszRegisters;
    TCHAR* pszParam;
    TCHAR* pszCallStack;
    TCHAR* psz;
    BYTE* Buffer;
    DWORD BufferSize;
    PARAMETER_LOG_INFOS* pParamLogInfo;
    LOG_LIST_ENTRY LogListEntry;
    PLOG_ENTRY pLogEntry;
    PLOG_ENTRY_FIXED_SIZE pHookInfos;
    DWORD dwValue;
    DWORD MajorVersion=0;
    DWORD MinorVersion=0;
    BYTE cnt;

    if(!CTextFile::Read(pszFile,&pszContent,&bUnicodeSave))
        return FALSE;
    pszCurrentMarkup=pszContent;

    if (!COpenSave::ReadXMLMarkupContent(pszCurrentMarkup,_T("WINAPIOVERRIDE_LOGLIST"),&pszBuffer,&BufferSize,&pszNextMarkup))
    {
        delete pszContent;
        MessageBox(NULL,_T("Invalid File"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    pszCurrentMarkup=pszBuffer;

    // get version info
    if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("MAJOR_VERSION"),&MajorVersion,&pszNextMarkup))
    {
        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("MINOR_VERSION"),&MinorVersion,&pszNextMarkup);
    }

    // 5.2 bug correction : even if file was saved with unicode file format, some xml editors (like m$ one) are happy to translate
    // file from unicode to ascii, and as our string are saved in hexa values it makes all string retrieval failed
    // add a field to specify file format
    if ( (MajorVersion>5)
        || ( (MajorVersion==5) && (MinorVersion>=2) )
        )
    {
        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("UNICODE"),(DWORD*)&bUnicodeSave,&pszNextMarkup);
    }

    pLogList->Lock(TRUE);

    // while we find another LOG tag
    while (COpenSave::ReadXMLMarkupContent(pszCurrentMarkup,_T("LOG"),&pszBuffer,&BufferSize,&pszNextMarkup))
    {
        pszLog=new TCHAR[BufferSize+1];
        memcpy(pszLog,pszBuffer,BufferSize*sizeof(TCHAR));
        pszLog[BufferSize]=0;

        pszCurrentMarkup=pszLog;
        pszNextMarkupLog=pszNextMarkup;

        // initialize an empty LOG_LIST_ENTRY
        memset(&LogListEntry,0,sizeof(LOG_LIST_ENTRY));

        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ID"),&LogListEntry.dwId,&pszNextMarkup);
        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("TYPE"),&dwValue,&pszNextMarkup);
        LogListEntry.Type=(tagLogListEntryTypes)dwValue;

        if (LogListEntry.Type==ENTRY_LOG)
        {
            // initialize an empty LOG_ENTRY
            // pLogEntry=new LOG_ENTRY;
            // memset(pLogEntry,0,sizeof(LOG_ENTRY));
            pLogEntry=(LOG_ENTRY*)HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,sizeof(LOG_ENTRY));
            LogListEntry.pLog=pLogEntry;
            

            //pHookInfos=new LOG_ENTRY_FIXED_SIZE;
            //memset(pHookInfos,0,sizeof(LOG_ENTRY_FIXED_SIZE));
            pHookInfos=(LOG_ENTRY_FIXED_SIZE*)HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,sizeof(LOG_ENTRY_FIXED_SIZE));
            pLogEntry->pHookInfos=pHookInfos;
            

            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PROCESS_ID"),&pHookInfos->dwProcessId,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("THREAD_ID"),&pHookInfos->dwThreadId,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("DIRECTION"),&dwValue,&pszNextMarkup);
            pHookInfos->bParamDirectionType=(BYTE)dwValue;
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ORIGIN_ADDR"),&pHookInfos->pOriginAddress,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("RELATIVE_ADDR"),&pHookInfos->RelativeAddressFromCallingModuleName,&pszNextMarkup);

            // save current markup        
            pszUpperMarkup=pszCurrentMarkup;

            // store content of REGISTERS_BEFORE_CALL in pszRegisters
            COpenSave::ReadXMLMarkupContent(pszCurrentMarkup,_T("REGISTERS_BEFORE_CALL"),&pszBuffer,&BufferSize,&pszNextMarkup);
            pszRegisters=new TCHAR[BufferSize+1];
            memcpy(pszRegisters,pszBuffer,BufferSize*sizeof(TCHAR));
            pszRegisters[BufferSize]=0;

            pszCurrentMarkup=pszRegisters;
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EAX"),&pHookInfos->RegistersBeforeCall.eax,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EBX"),&pHookInfos->RegistersBeforeCall.ebx,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ECX"),&pHookInfos->RegistersBeforeCall.ecx,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EDX"),&pHookInfos->RegistersBeforeCall.edx,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EDI"),&pHookInfos->RegistersBeforeCall.edi,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ESI"),&pHookInfos->RegistersBeforeCall.esi,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EFL"),&pHookInfos->RegistersBeforeCall.efl,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ES"),&pHookInfos->RegistersBeforeCall.es,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("FS"),&pHookInfos->RegistersBeforeCall.fs,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("GS"),&pHookInfos->RegistersBeforeCall.gs,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ESP"),&pHookInfos->RegistersBeforeCall.esp,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EBP"),&pHookInfos->RegistersBeforeCall.ebp,&pszNextMarkup);
            // free content of REGISTERS_BEFORE_CALL
            delete[] pszRegisters;
            // restore upper markup
            pszCurrentMarkup=pszUpperMarkup;

            // store content of REGISTERS_BEFORE_CALL in pszRegisters
            COpenSave::ReadXMLMarkupContent(pszCurrentMarkup,_T("REGISTERS_AFTER_CALL"),&pszBuffer,&BufferSize,&pszNextMarkup);
            pszRegisters=new TCHAR[BufferSize+1];
            memcpy(pszRegisters,pszBuffer,BufferSize*sizeof(TCHAR));
            pszRegisters[BufferSize]=0;

            pszCurrentMarkup=pszRegisters;
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EAX"),&pHookInfos->RegistersAfterCall.eax,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EBX"),&pHookInfos->RegistersAfterCall.ebx,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ECX"),&pHookInfos->RegistersAfterCall.ecx,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EDX"),&pHookInfos->RegistersAfterCall.edx,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EDI"),&pHookInfos->RegistersAfterCall.edi,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ESI"),&pHookInfos->RegistersAfterCall.esi,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EFL"),&pHookInfos->RegistersAfterCall.efl,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ES"),&pHookInfos->RegistersAfterCall.es,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("FS"),&pHookInfos->RegistersAfterCall.fs,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("GS"),&pHookInfos->RegistersAfterCall.gs,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ESP"),&pHookInfos->RegistersAfterCall.esp,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("EBP"),&pHookInfos->RegistersAfterCall.ebp,&pszNextMarkup);

            // free content of REGISTERS_BEFORE_CALL
            delete[] pszRegisters;
            // restore upper markup
            pszCurrentMarkup=pszUpperMarkup;

            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("RETURNED_VALUE"),&pHookInfos->ReturnValue,&pszNextMarkup);
            if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("RETURNED_DOUBLE_VALUE"),&Buffer,&BufferSize,&pszNextMarkup))
            {
                pHookInfos->DoubleResult=0;
                if (Buffer)
                {
                    if (BufferSize==sizeof(double))
                        memcpy(&pHookInfos->DoubleResult,Buffer,BufferSize);
                    // delete Buffer;
                    HeapFree(mhLogHeap,0,Buffer);
                }
            }


            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("FAILURE"),&dwValue,&pszNextMarkup);
            pHookInfos->bFailure=(BOOLEAN)dwValue;
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("LAST_ERROR"),&pHookInfos->dwLastError,&pszNextMarkup);

            if ((MajorVersion<=3)&&(MinorVersion<1))
            {
                // old monitoring file convert old time format to new one
                BEFORE_V3_1CALLTIME OldTime;
                COpenSave::ReadXMLValue(pszCurrentMarkup,_T("CALLTIME"),&OldTime.dw,&pszNextMarkup);
                // get time in ms
                ULONGLONG ul=OldTime.strBeforeV3_1CallTime.MilliSeconds
                    +OldTime.strBeforeV3_1CallTime.Second*1000
                    +OldTime.strBeforeV3_1CallTime.Minute*60*1000
                    +OldTime.strBeforeV3_1CallTime.Hour*60*60*1000;
                // translate it into 100ns
                ul=ul*10000;
                pHookInfos->CallTime.dwHighDateTime=(DWORD)(ul>>32);
                pHookInfos->CallTime.dwLowDateTime=(DWORD)ul;
            }
            else
            {
                if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("CALLTIME"),&Buffer,&BufferSize,&pszNextMarkup))
                {
                    if (Buffer)
                    {
                        memcpy(&pHookInfos->CallTime,Buffer,__min(BufferSize,sizeof(FILETIME)));
                        // delete Buffer;
                        HeapFree(mhLogHeap,0,Buffer);
                    }
                }
            }

            // introduced in 5.0 version
            if (MajorVersion>=5)
            {
                if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARENT_CALLTIME"),&Buffer,&BufferSize,&pszNextMarkup))
                {
                    if (Buffer)
                    {
                        memcpy(&pHookInfos->FirstHookedParentCallTime,Buffer,__min(BufferSize,sizeof(FILETIME)));
                        // delete Buffer;
                        HeapFree(mhLogHeap,0,Buffer);
                    }
                }
            }

            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("DURATION"),&pHookInfos->dwCallDuration,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("NB_PARAMS"),&dwValue,&pszNextMarkup);
            pHookInfos->bNumberOfParameters=(BYTE)dwValue;


            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("MODULE_NAME"),&pLogEntry->pszModuleName,bUnicodeSave,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("API_NAME"),&pLogEntry->pszApiName,bUnicodeSave,&pszNextMarkup);
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("CALLING_MODULE_NAME"),&pLogEntry->pszCallingModuleName,bUnicodeSave,&pszNextMarkup);


            // save current markup        
            pszUpperMarkup=pszCurrentMarkup;

            if (pHookInfos->bNumberOfParameters>0)
            {
                // pLogEntry->ParametersInfoArray=new PARAMETER_LOG_INFOS[pHookInfos->bNumberOfParameters];
                pLogEntry->ParametersInfoArray=(PARAMETER_LOG_INFOS*)HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,sizeof(PARAMETER_LOG_INFOS)*pHookInfos->bNumberOfParameters);
            }
            for (cnt=0;cnt<pHookInfos->bNumberOfParameters;cnt++)
            {
                pParamLogInfo=&pLogEntry->ParametersInfoArray[cnt];
                
                // store content of REGISTERS_BEFORE_CALL in pszRegisters
                COpenSave::ReadXMLMarkupContent(pszCurrentMarkup,_T("PARAM"),&pszBuffer,&BufferSize,&pszNextMarkupParam);

                pszParam=new TCHAR[BufferSize+1];
                memcpy(pszParam,pszBuffer,BufferSize*sizeof(TCHAR));
                pszParam[BufferSize]=0;
                
                COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAM_TYPE"),&pParamLogInfo->dwType,&pszNextMarkup);

                COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAM_NAME"),&psz,bUnicodeSave,&pszNextMarkup);
                if (psz)
                {
                    _tcsncpy(pParamLogInfo->pszParameterName,psz,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1);
                    pParamLogInfo->pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;
                    // delete psz;
                    HeapFree(mhLogHeap,0,psz);
                }
                else
                    *pParamLogInfo->pszParameterName=0;

                COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAM_VALUE"),&pParamLogInfo->Value,&pszNextMarkup);
                COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAM_BUFFERVALUE_SIZE"),&pParamLogInfo->dwSizeOfData,&pszNextMarkup);
                COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAM_POINTEDVALUE_SIZE"),&pParamLogInfo->dwSizeOfPointedValue,&pszNextMarkup);
                COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAM_BUFFERVALUE"),&pParamLogInfo->pbValue,&dwValue,&pszNextMarkup);

                if (((pParamLogInfo->dwType==PARAM_PUNICODE_STRING)||(pParamLogInfo->dwType==PARAM_PANSI_STRING))
                    &&(pParamLogInfo->dwSizeOfPointedValue!=0))
                {
                    if ((MajorVersion<=3)&&(MinorVersion<1))
                    {
                        if (pParamLogInfo->dwType==PARAM_PUNICODE_STRING)
                        {
                            // create a fake struct
                            UNICODE_STRING us;
                            us.Buffer=0;
                            us.Length=(USHORT)wcslen((WCHAR*)pParamLogInfo->pbValue);
                            us.MaximumLength=us.Length+1;

                            // store current buffer 
                            BYTE* tmp=pParamLogInfo->pbValue;

                            // allocate a new buffer
                            // pParamLogInfo->pbValue=new BYTE[pParamLogInfo->dwSizeOfPointedValue+sizeof(UNICODE_STRING)];
                            pParamLogInfo->pbValue=(BYTE*)HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,pParamLogInfo->dwSizeOfPointedValue+sizeof(UNICODE_STRING));
                            // copy content of struct in new buffer
                            memcpy(pParamLogInfo->pbValue,&us,sizeof(UNICODE_STRING));
                            // copy old buffer into new one after struct
                            memcpy(&pParamLogInfo->pbValue[sizeof(UNICODE_STRING)],tmp,pParamLogInfo->dwSizeOfPointedValue);
                            // increase the buffer size
                            pParamLogInfo->dwSizeOfPointedValue+=sizeof(UNICODE_STRING);
                            // delete the old allocated buffer
                            // delete tmp;
                            HeapFree(mhLogHeap,0,tmp);
                        }
                        else if (pParamLogInfo->dwType==PARAM_PANSI_STRING)
                        {
                            // create a fake struct
                            ANSI_STRING as;
                            as.Buffer=0;
                            as.Length=(USHORT)strlen((CHAR*)pParamLogInfo->pbValue);
                            as.MaximumLength=as.Length+1;

                            // store current buffer 
                            BYTE* tmp=pParamLogInfo->pbValue;

                            // allocate a new buffer
                            // pParamLogInfo->pbValue=new BYTE[pParamLogInfo->dwSizeOfPointedValue+sizeof(ANSI_STRING)];
                            pParamLogInfo->pbValue=(BYTE*)HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,pParamLogInfo->dwSizeOfPointedValue+sizeof(ANSI_STRING));
                            // copy content of struct in new buffer
                            memcpy(pParamLogInfo->pbValue,&as,sizeof(ANSI_STRING));
                            // copy old buffer into new one after struct
                            memcpy(&pParamLogInfo->pbValue[sizeof(ANSI_STRING)],tmp,pParamLogInfo->dwSizeOfPointedValue);
                            // increase the buffer size
                            pParamLogInfo->dwSizeOfPointedValue+=sizeof(ANSI_STRING);
                            // delete the old allocated buffer
                            // delete tmp;
                            HeapFree(mhLogHeap,0,tmp);
                        }

                    }
                }
                // 5.2 support of extended type (defines values and user define types)
                if ( (MajorVersion>5)
                    || ( (MajorVersion==5) && (MinorVersion>=2) )
                    )
                {
                    if (pParamLogInfo->dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE)
                    {
                        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("DEFINES"),&pParamLogInfo->pszDefineNamesFile,bUnicodeSave,&pszNextMarkup);
                    }
                    if (pParamLogInfo->dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE)
                    {
                        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("USER_DATA_TYPE"),&pParamLogInfo->pszUserDataTypeName,bUnicodeSave,&pszNextMarkup);
                    }
                }

                delete[] pszParam;
                pszCurrentMarkup=pszNextMarkupParam;
            }
            // restore markup 
            pszCurrentMarkup=pszUpperMarkup;

            if (!COpenSave::ReadXMLMarkupContent(pszCurrentMarkup,_T("CALLSTACK"),&pszBuffer,&BufferSize,&pszNextMarkup))
            {
                pLogEntry->pHookInfos->CallStackSize=0;
                pLogEntry->CallSackInfoArray=NULL;
            }
            else
            {
                // store content of CALLSTACK in pszCallStack

                pszCallStack=new TCHAR[BufferSize+1];
                memcpy(pszCallStack,pszBuffer,BufferSize*sizeof(TCHAR));
                pszCallStack[BufferSize]=0;

                pszCurrentMarkup=pszCallStack;

                if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("SIZE"),&dwValue,&pszNextMarkup))
                {
                    pLogEntry->pHookInfos->CallStackSize=(WORD)dwValue;
                    COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAMS_SIZE"),&dwValue,&pszNextMarkup);
                    pLogEntry->pHookInfos->CallStackEbpRetrievalSize=(WORD)dwValue;

                    // pLogEntry->CallSackInfoArray=new CALLSTACK_ITEM_INFO[pLogEntry->pHookInfos->CallStackSize];
                    pLogEntry->CallSackInfoArray=(CALLSTACK_ITEM_INFO*)HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,pLogEntry->pHookInfos->CallStackSize*sizeof(CALLSTACK_ITEM_INFO));

                    for (cnt=0;cnt<pLogEntry->pHookInfos->CallStackSize;cnt++)
                    {
                        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("ADDR"),&pLogEntry->CallSackInfoArray[cnt].Address,&pszNextMarkup);
                        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("RELATIVE_ADDR"),&pLogEntry->CallSackInfoArray[cnt].RelativeAddress,&pszNextMarkup);
                        COpenSave::ReadXMLValue(pszCurrentMarkup,_T("MODULE_NAME"),&pLogEntry->CallSackInfoArray[cnt].pszModuleName,bUnicodeSave,&pszNextMarkup);

                        if (pLogEntry->pHookInfos->CallStackEbpRetrievalSize==0)
                            pLogEntry->CallSackInfoArray[cnt].Parameters=NULL;
                        else
                        {
                            // copy params assuming to get a buffer with size==pLogEntry->pHookInfos->CallStackEbpRetrievalSize
                            // pLogEntry->CallSackInfoArray[cnt].Parameters=new BYTE[pLogEntry->pHookInfos->CallStackEbpRetrievalSize];
                            // memset(pLogEntry->CallSackInfoArray[cnt].Parameters,0,pLogEntry->pHookInfos->CallStackEbpRetrievalSize);
                            pLogEntry->CallSackInfoArray[cnt].Parameters=(BYTE*)HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,pLogEntry->pHookInfos->CallStackEbpRetrievalSize);
                            if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("PARAMS"),&Buffer,&BufferSize,&pszNextMarkup))
                            {
                                if ( (BufferSize>0) && Buffer)
                                {
                                    if (BufferSize>pLogEntry->pHookInfos->CallStackEbpRetrievalSize)
                                        BufferSize=pLogEntry->pHookInfos->CallStackEbpRetrievalSize;
                                    // copy data from locally allocated buffer
                                    memcpy(pLogEntry->CallSackInfoArray[cnt].Parameters,Buffer,BufferSize);

                                    // delete locally allocated buffer
                                    // delete Buffer;
                                    HeapFree(mhLogHeap,0,Buffer);
                                }
                            }
                        }


                        pszCurrentMarkup=pszNextMarkup;
                    }
                }

                delete[] pszCallStack;
            }
            // restore markup 
            pszCurrentMarkup=pszUpperMarkup;


            // introduced in 5.2 version
            // SHOULD BE IN LAST POSITION FOR PERFORMANCE OF XML RETRIVAL AS IT'S OPTIONAL DATA
            pLogEntry->pHookInfos->HookType = HOOK_TYPE_API;
            if ( (MajorVersion>5)
                 || ( (MajorVersion==5) && (MinorVersion>=2) )
               )
            {

                if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("HOOK_TYPE"),(DWORD*)&pLogEntry->pHookInfos->HookType,&pszNextMarkup))
                {
                    if (pLogEntry->pHookInfos->HookType != HOOK_TYPE_API) // should always be the case
                    {
                        TCHAR* pszExtendedFunctionInformations;

                        // save current markup        
                        pszUpperMarkup=pszCurrentMarkup;

                        // store content of EXT_FUNC_INFOS in pszExtendedFunctionInformations
                        if (COpenSave::ReadXMLMarkupContent(pszCurrentMarkup,_T("EXT_FUNC_INFOS"),&pszBuffer,&BufferSize,&pszNextMarkup))
                        {
                            pszExtendedFunctionInformations=new TCHAR[BufferSize+1];
                            memcpy(pszExtendedFunctionInformations,pszBuffer,BufferSize*sizeof(TCHAR));
                            pszExtendedFunctionInformations[BufferSize]=0;

                            pszCurrentMarkup=pszExtendedFunctionInformations;

                            switch (pLogEntry->pHookInfos->HookType)
                            {
                                case HOOK_TYPE_COM:
                                    // get Clsid
                                    if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("CLSID"),&Buffer,&BufferSize,&pszNextMarkup))
                                    {
                                        if (Buffer)
                                        {
                                            memcpy(&pLogEntry->HookTypeExtendedFunctionInfos.InfosForCOM.ClassID,Buffer,__min(BufferSize,sizeof(CLSID)));
                                            // delete Buffer;
                                            HeapFree(mhLogHeap,0,Buffer);
                                        }
                                    }

                                    // get iid
                                    if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("IID"),&Buffer,&BufferSize,&pszNextMarkup))
                                    {
                                        if (Buffer)
                                        {
                                            memcpy(&pLogEntry->HookTypeExtendedFunctionInfos.InfosForCOM.InterfaceID,Buffer,__min(BufferSize,sizeof(IID)));
                                            // delete Buffer;
                                            HeapFree(mhLogHeap,0,Buffer);
                                        }
                                    }

                                    // get vtbl index
                                    COpenSave::ReadXMLValue(pszCurrentMarkup,_T("VTBL_INDEX"),&pLogEntry->HookTypeExtendedFunctionInfos.InfosForCOM.VTBLIndex,&pszNextMarkup);

                                    break;
                                case HOOK_TYPE_NET:
                                    // get function token
                                    COpenSave::ReadXMLValue(pszCurrentMarkup,_T("FUNC_TOKEN"),(DWORD*)&pLogEntry->HookTypeExtendedFunctionInfos.InfosForNET.FunctionToken,&pszNextMarkup);
                                    break;
                            }

                            // restore upper markup
                            pszCurrentMarkup=pszUpperMarkup;
                        }
                    }

                }
            }
        }
        else
        {
            COpenSave::ReadXMLValue(pszCurrentMarkup,_T("MSG"),&LogListEntry.ReportEntry.pUserMsg,bUnicodeSave,&pszNextMarkup);
            if (LogListEntry.Type == ENTRY_MSG_EXCEPTION)
            {
                // 5.4 change message translated from "exception msg | registers | ProcessId | ThreadId" to "exception msg || registers | ProcessId | ThreadId"
                // because "exception msg" can contain "|"
                if ( (MajorVersion<=5) && (MinorVersion<4))
                {
                    TCHAR* OldString = LogListEntry.ReportEntry.pUserMsg;
                    // find delimiter : the 3rd begining from end
                    TCHAR* pc;
                    SIZE_T NbDelimiterFound = 0;
                    SIZE_T OldStringSize = _tcslen(OldString);
                    for (pc = OldString + OldStringSize;pc>OldString;pc--)
                    {
                        if (*pc=='|')
                        {
                            NbDelimiterFound++;
                            if (NbDelimiterFound == 3)
                            {
                                LogListEntry.ReportEntry.pUserMsg =(TCHAR*) HeapAlloc(mhLogHeap,HEAP_ZERO_MEMORY,(OldStringSize+2)*sizeof(TCHAR));//+1 for \0 +1 for new '|'
                                memcpy(LogListEntry.ReportEntry.pUserMsg,OldString,(pc-OldString)*sizeof(TCHAR));
                                _tcscat(LogListEntry.ReportEntry.pUserMsg,_T("|"));
                                _tcscat(LogListEntry.ReportEntry.pUserMsg,pc);
                                HeapFree(mhLogHeap,0,OldString);
                                break;
                            }
                        }
                    }
                }
            }

            memset(&LogListEntry.ReportEntry.ReportTime,0,sizeof(LogListEntry.ReportEntry.ReportTime));
            if (COpenSave::ReadXMLValue(pszCurrentMarkup,_T("CALLTIME"),&Buffer,&BufferSize,&pszNextMarkup))
            {
                if (Buffer)
                {
                    memcpy(&LogListEntry.ReportEntry.ReportTime,Buffer,__min(BufferSize,sizeof(FILETIME)));
                    // delete Buffer;
                    HeapFree(mhLogHeap,0,Buffer);
                }
            }
        }

        // add log to log list
        pLogList->AddItem(&LogListEntry);

        delete[] pszLog;
        pszCurrentMarkup=pszNextMarkupLog;
    }

    delete pszContent;

    pLogList->Unlock();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save logs to file
// Parameters :
//     in  : TCHAR* pszFile : name of the log file
//     out :
//     return : TRUE on save success, FALSE else
//-----------------------------------------------------------------------------
BOOL COpenSave::Save(TCHAR* pszFile)
{
    DWORD dwWrittenBytes;
    BOOL bUnicodeSave;
    HANDLE hFile = CreateFile(pszFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }


#if (defined(UNICODE)||defined(_UNICODE))
    // in unicode mode, write the unicode little endian header (FFFE)
    BYTE pbUnicodeHeader[2]={0xFF,0xFE};
    WriteFile(hFile,pbUnicodeHeader,2,&dwWrittenBytes,NULL);

    bUnicodeSave = TRUE;
#else
    bUnicodeSave = FALSE;
#endif

    WriteFile(hFile,_T("<WINAPIOVERRIDE_LOGLIST>"),24*sizeof(TCHAR),&dwWrittenBytes,NULL);

    CVersion Version;
    TCHAR ExeFileName[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL),ExeFileName,MAX_PATH);
    Version.Read(ExeFileName);

    WriteXMLValue(hFile,_T("MAJOR_VERSION"),Version.FixedFileInfo.dwFileVersionMS >> 16);
    WriteXMLValue(hFile,_T("MINOR_VERSION"),Version.FixedFileInfo.dwFileVersionMS & 0xffff);
    WriteXMLValue(hFile,_T("UNICODE"),bUnicodeSave);

    CLinkListItem* pItem;
    PLOG_LIST_ENTRY pLog;
    REGISTERS Reg;
    
    BYTE cnt;
    pLogList->Lock(TRUE);
    for (pItem=pLogList->Head;pItem;pItem=pItem->NextItem)
    {
        pLog=(PLOG_LIST_ENTRY)(pItem->ItemData);

        WriteFile(hFile,_T("<LOG>"),5*sizeof(TCHAR),&dwWrittenBytes,NULL);

        WriteXMLValue(hFile,_T("ID"),pLog->dwId);
        WriteXMLValue(hFile,_T("TYPE"),pLog->Type);

        if (pLog->Type==ENTRY_LOG)
        {
            WriteXMLValue(hFile,_T("PROCESS_ID"),pLog->pLog->pHookInfos->dwProcessId);
            WriteXMLValue(hFile,_T("THREAD_ID"),pLog->pLog->pHookInfos->dwThreadId);
            WriteXMLValue(hFile,_T("DIRECTION"),pLog->pLog->pHookInfos->bParamDirectionType);
            WriteXMLValue(hFile,_T("ORIGIN_ADDR"),pLog->pLog->pHookInfos->pOriginAddress);
            WriteXMLValue(hFile,_T("RELATIVE_ADDR"),pLog->pLog->pHookInfos->RelativeAddressFromCallingModuleName);

            Reg=pLog->pLog->pHookInfos->RegistersBeforeCall;
            WriteFile(hFile,_T("<REGISTERS_BEFORE_CALL>"),23*sizeof(TCHAR),&dwWrittenBytes,NULL);
                WriteXMLValue(hFile,_T("EAX"),Reg.eax);
                WriteXMLValue(hFile,_T("EBX"),Reg.ebx);
                WriteXMLValue(hFile,_T("ECX"),Reg.ecx);
                WriteXMLValue(hFile,_T("EDX"),Reg.edx);
                WriteXMLValue(hFile,_T("EDI"),Reg.edi);
                WriteXMLValue(hFile,_T("ESI"),Reg.esi);
                WriteXMLValue(hFile,_T("EFL"),Reg.efl);
                WriteXMLValue(hFile,_T("ES"),Reg.es);
                WriteXMLValue(hFile,_T("FS"),Reg.fs);
                WriteXMLValue(hFile,_T("GS"),Reg.gs);
                WriteXMLValue(hFile,_T("ESP"),Reg.esp);
                WriteXMLValue(hFile,_T("EBP"),Reg.ebp);
            WriteFile(hFile,_T("</REGISTERS_BEFORE_CALL>"),24*sizeof(TCHAR),&dwWrittenBytes,NULL);

            Reg=pLog->pLog->pHookInfos->RegistersAfterCall;
            WriteFile(hFile,_T("<REGISTERS_AFTER_CALL>"),22*sizeof(TCHAR),&dwWrittenBytes,NULL);
                WriteXMLValue(hFile,_T("EAX"),Reg.eax);
                WriteXMLValue(hFile,_T("EBX"),Reg.ebx);
                WriteXMLValue(hFile,_T("ECX"),Reg.ecx);
                WriteXMLValue(hFile,_T("EDX"),Reg.edx);
                WriteXMLValue(hFile,_T("EDI"),Reg.edi);
                WriteXMLValue(hFile,_T("ESI"),Reg.esi);
                WriteXMLValue(hFile,_T("EFL"),Reg.efl);
                WriteXMLValue(hFile,_T("ES"),Reg.es);
                WriteXMLValue(hFile,_T("FS"),Reg.fs);
                WriteXMLValue(hFile,_T("GS"),Reg.gs);
                WriteXMLValue(hFile,_T("ESP"),Reg.esp);
                WriteXMLValue(hFile,_T("EBP"),Reg.ebp);
            WriteFile(hFile,_T("</REGISTERS_AFTER_CALL>"),23*sizeof(TCHAR),&dwWrittenBytes,NULL);

            WriteXMLValue(hFile,_T("RETURNED_VALUE"),pLog->pLog->pHookInfos->ReturnValue);
            WriteXMLValue(hFile,_T("RETURNED_DOUBLE_VALUE"),(PBYTE)&pLog->pLog->pHookInfos->DoubleResult,sizeof(double));
            WriteXMLValue(hFile,_T("FAILURE"),pLog->pLog->pHookInfos->bFailure);
            WriteXMLValue(hFile,_T("LAST_ERROR"),pLog->pLog->pHookInfos->dwLastError);
            WriteXMLValue(hFile,_T("CALLTIME"),(PBYTE)&pLog->pLog->pHookInfos->CallTime,sizeof(FILETIME));
            WriteXMLValue(hFile,_T("PARENT_CALLTIME"),(PBYTE)&pLog->pLog->pHookInfos->FirstHookedParentCallTime,sizeof(FILETIME));
            WriteXMLValue(hFile,_T("DURATION"),pLog->pLog->pHookInfos->dwCallDuration);
            WriteXMLValue(hFile,_T("NB_PARAMS"),pLog->pLog->pHookInfos->bNumberOfParameters);

            WriteXMLValue(hFile,_T("MODULE_NAME"),pLog->pLog->pszModuleName);
            WriteXMLValue(hFile,_T("API_NAME"),pLog->pLog->pszApiName);
            WriteXMLValue(hFile,_T("CALLING_MODULE_NAME"),pLog->pLog->pszCallingModuleName);


            for (cnt=0;cnt<pLog->pLog->pHookInfos->bNumberOfParameters;cnt++)
            {
                WriteFile(hFile,_T("<PARAM>"),7*sizeof(TCHAR),&dwWrittenBytes,NULL);

                WriteXMLValue(hFile,_T("PARAM_TYPE"),pLog->pLog->ParametersInfoArray[cnt].dwType);
                WriteXMLValue(hFile,_T("PARAM_NAME"),pLog->pLog->ParametersInfoArray[cnt].pszParameterName);
                WriteXMLValue(hFile,_T("PARAM_VALUE"),pLog->pLog->ParametersInfoArray[cnt].Value);
                WriteXMLValue(hFile,_T("PARAM_BUFFERVALUE_SIZE"),pLog->pLog->ParametersInfoArray[cnt].dwSizeOfData);
                WriteXMLValue(hFile,_T("PARAM_POINTEDVALUE_SIZE"),pLog->pLog->ParametersInfoArray[cnt].dwSizeOfPointedValue);

                if (pLog->pLog->ParametersInfoArray[cnt].dwSizeOfData<=REGISTER_BYTE_SIZE)
                    WriteXMLValue(hFile,_T("PARAM_BUFFERVALUE"),pLog->pLog->ParametersInfoArray[cnt].pbValue,pLog->pLog->ParametersInfoArray[cnt].dwSizeOfPointedValue);
                else
                    WriteXMLValue(hFile,_T("PARAM_BUFFERVALUE"),pLog->pLog->ParametersInfoArray[cnt].pbValue,pLog->pLog->ParametersInfoArray[cnt].dwSizeOfData);


                if (pLog->pLog->ParametersInfoArray[cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE)
                    WriteXMLValue(hFile,_T("DEFINES"),pLog->pLog->ParametersInfoArray[cnt].pszDefineNamesFile);
                if (pLog->pLog->ParametersInfoArray[cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE)
                    WriteXMLValue(hFile,_T("USER_DATA_TYPE"),pLog->pLog->ParametersInfoArray[cnt].pszUserDataTypeName);

                WriteFile(hFile,_T("</PARAM>"),8*sizeof(TCHAR),&dwWrittenBytes,NULL);
            }

            if (pLog->pLog->pHookInfos->CallStackSize>0)
            {
                WriteFile(hFile,_T("<CALLSTACK>"),11*sizeof(TCHAR),&dwWrittenBytes,NULL);
                WriteXMLValue(hFile,_T("SIZE"),(DWORD)pLog->pLog->pHookInfos->CallStackSize);
                WriteXMLValue(hFile,_T("PARAMS_SIZE"),(DWORD)pLog->pLog->pHookInfos->CallStackEbpRetrievalSize);

                for (cnt=0;cnt<pLog->pLog->pHookInfos->CallStackSize;cnt++)
                {
                    WriteFile(hFile,_T("<CALLER>"),8*sizeof(TCHAR),&dwWrittenBytes,NULL);
                    WriteXMLValue(hFile,_T("ADDR"),pLog->pLog->CallSackInfoArray[cnt].Address);
                    WriteXMLValue(hFile,_T("RELATIVE_ADDR"),pLog->pLog->CallSackInfoArray[cnt].RelativeAddress);
                    WriteXMLValue(hFile,_T("MODULE_NAME"),pLog->pLog->CallSackInfoArray[cnt].pszModuleName);
                    WriteXMLValue(hFile,_T("PARAMS"),pLog->pLog->CallSackInfoArray[cnt].Parameters,pLog->pLog->pHookInfos->CallStackEbpRetrievalSize);
                    WriteFile(hFile,_T("</CALLER>"),9*sizeof(TCHAR),&dwWrittenBytes,NULL);    
                }

                WriteFile(hFile,_T("</CALLSTACK>"),12*sizeof(TCHAR),&dwWrittenBytes,NULL);
            }

            // SHOULD BE IN LAST POSITION FOR PERFORMANCE OF XML RETRIVAL AS IT'S OPTIONAL DATA
            // store informations only if necessary, as 90% of use will be HOOK_TYPE_API
            if (pLog->pLog->pHookInfos->HookType != HOOK_TYPE_API)
            {
                WriteXMLValue(hFile,_T("HOOK_TYPE"),pLog->pLog->pHookInfos->HookType);
                WriteFile(hFile,_T("<EXT_FUNC_INFOS>"),16*sizeof(TCHAR),&dwWrittenBytes,NULL);
                switch (pLog->pLog->pHookInfos->HookType)
                {
                case HOOK_TYPE_COM:
                    WriteXMLValue(hFile,_T("CLSID"),(PBYTE)&pLog->pLog->HookTypeExtendedFunctionInfos.InfosForCOM.ClassID,sizeof(CLSID));
                    WriteXMLValue(hFile,_T("IID"),(PBYTE)&pLog->pLog->HookTypeExtendedFunctionInfos.InfosForCOM.InterfaceID,sizeof(IID));
                    WriteXMLValue(hFile,_T("VTBL_INDEX"),pLog->pLog->HookTypeExtendedFunctionInfos.InfosForCOM.VTBLIndex);
                    break;

                case HOOK_TYPE_NET:
                    WriteXMLValue(hFile,_T("FUNC_TOKEN"),pLog->pLog->HookTypeExtendedFunctionInfos.InfosForNET.FunctionToken);
                    break;
                }
                WriteFile(hFile,_T("</EXT_FUNC_INFOS>"),17*sizeof(TCHAR),&dwWrittenBytes,NULL);
            }
        }
        else
        {
            WriteXMLValue(hFile,_T("MSG"),pLog->ReportEntry.pUserMsg);
            WriteXMLValue(hFile,_T("CALLTIME"),(PBYTE)&pLog->ReportEntry.ReportTime,sizeof(FILETIME));
        }

        WriteFile(hFile,_T("</LOG>"),6*sizeof(TCHAR),&dwWrittenBytes,NULL);
    }
    pLogList->Unlock();

    WriteFile(hFile,_T("</WINAPIOVERRIDE_LOGLIST>"),25*sizeof(TCHAR),&dwWrittenBytes,NULL);

    CloseHandle(hFile);

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ReadXMLMarkupContent
// Object: read xml content. This func allow version soft read ascii saved files
//          or ansi version read unicode saved files
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//     out : TCHAR** ppszContent : content of the markup in the same encoding as FullString
//           DWORD* pContentLength : content length in TCHAR
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL COpenSave::ReadXMLMarkupContent(TCHAR* FullString,TCHAR* Markup,TCHAR** ppszContent,DWORD* pContentLength,TCHAR** pPointerAfterEndingMarkup)
{
    *ppszContent=0;
    *pContentLength=0;
    *pPointerAfterEndingMarkup=0;

    if (IsBadReadPtr(FullString,sizeof(TCHAR*)))
        return FALSE;
    if (*FullString==0)
        return FALSE;

    size_t MarkupSize=_tcslen(Markup);
    TCHAR* pszStartTag=(TCHAR*)_alloca(sizeof(TCHAR)*(MarkupSize+3));
    TCHAR* pszEndTag=(TCHAR*)_alloca(sizeof(TCHAR)*(MarkupSize+4));
    TCHAR* pszStartTagPos;
    TCHAR* pszEndTagPos;
    
    // make xml start tag and end tag (Tag --> <Tag> and </Tag>)
    _stprintf(pszStartTag,_T("<%s>"),Markup);
    _stprintf(pszEndTag,_T("</%s>"),Markup);

    // search xml start tag and end tag
    pszStartTagPos=_tcsstr(FullString,pszStartTag);
    pszEndTagPos=_tcsstr(FullString,pszEndTag);
    // if none found
    if ((pszStartTagPos==0)||(pszEndTagPos==0))
        return FALSE;

    // if bad positions
    if (pszEndTagPos<pszStartTagPos)
        return FALSE;

    // get content of markup
    *ppszContent=pszStartTagPos+MarkupSize+2;
    *pContentLength=(DWORD)(pszEndTagPos-*ppszContent);
    *pPointerAfterEndingMarkup=pszEndTagPos+MarkupSize+3;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ReadXMLValue
// Object: read xml hexa value. 
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//     out : DWORD* pValue : value contained in the markup
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL COpenSave::ReadXMLValue(TCHAR* FullString,TCHAR* Markup,DWORD* pValue,TCHAR** pPointerAfterEndingMarkup)
{
    TCHAR* pszContent;
    DWORD ContentLength;
    *pValue=0;
    // read content
    if (!COpenSave::ReadXMLMarkupContent(FullString,Markup,&pszContent,&ContentLength,pPointerAfterEndingMarkup))
        return FALSE;
    // if no content
    if (ContentLength==0)
        return FALSE;
    // check value retrieving
    if (_stscanf(pszContent,_T("0x%x"),pValue)!=1)
        return FALSE;
    return TRUE;

}

//-----------------------------------------------------------------------------
// Name: ReadXMLValue
// Object: read xml hexa value. 
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//     out : PBYTE* pValue : value contained in the markup
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL COpenSave::ReadXMLValue(TCHAR* FullString,TCHAR* Markup,PBYTE* pValue,TCHAR** pPointerAfterEndingMarkup)
{
    TCHAR* pszContent;
    DWORD ContentLength;
    *pValue=0;
    // read content
    if (!COpenSave::ReadXMLMarkupContent(FullString,Markup,&pszContent,&ContentLength,pPointerAfterEndingMarkup))
        return FALSE;
    // if no content
    if (ContentLength==0)
        return FALSE;
    // check value retrieving
    if (_stscanf(pszContent,_T("0x%p"),pValue)!=1)
        return FALSE;
    return TRUE;

}

//-----------------------------------------------------------------------------
// Name: ReadXMLValue
// Object: read xml PBYTE buffer. 
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//     out : BYTE** pBuffer : retrieved buffer . Must be free by calling delete
//           DWORD* pBufferLengthInByte : length of the buffer
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL COpenSave::ReadXMLValue(TCHAR* FullString,TCHAR* Markup,BYTE** pBuffer,DWORD* pBufferLengthInByte,TCHAR** pPointerAfterEndingMarkup)
{
    TCHAR* pszContent;
    DWORD ContentLength;
    *pBuffer=0;
    *pBufferLengthInByte=0;

    // read content
    if (!COpenSave::ReadXMLMarkupContent(FullString,Markup,&pszContent,&ContentLength,pPointerAfterEndingMarkup))
        return FALSE;
    if (ContentLength==0)
        return TRUE;

    // compute size of data
    *pBufferLengthInByte=ContentLength/2;
    // *pBuffer=new BYTE[*pBufferLengthInByte];
    *pBuffer=(BYTE*)HeapAlloc(mhLogHeap,0,*pBufferLengthInByte);
    // translate hex data to Byte
    for (DWORD cnt=0;cnt<*pBufferLengthInByte;cnt++)
        (*pBuffer)[cnt]=CStrToHex::StrHexToByte(&pszContent[cnt*2]);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ReadXMLValue
// Object: read xml string. 
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//           BOOL bUnicodeFile : TRUE if file is in unicode
//     out : TCHAR** pszValue : retrieved string . Must be free by calling delete
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL COpenSave::ReadXMLValue(TCHAR* FullString,TCHAR* Markup,TCHAR** pszValue,BOOL bUnicodeFile,TCHAR** pPointerAfterEndingMarkup)
{
    BYTE* Buffer;
    DWORD BufferSize;
    *pszValue=0;
    // get content in byte buffer
    if (!COpenSave::ReadXMLValue(FullString,Markup,&Buffer,&BufferSize,pPointerAfterEndingMarkup))
        return FALSE;

    // if empty field
    if  ( (BufferSize==0) || (!Buffer) )
    {
        // return an empty string
        *pszValue=(TCHAR*)HeapAlloc(mhLogHeap,0,sizeof(TCHAR));
        (*pszValue)[0]=0;
        return TRUE;
    }

    // we next have to convert buffer into ansi or unicode string

    // create string even BufferSize is null
#if (defined(UNICODE)||defined(_UNICODE))
    if (bUnicodeFile)
    {
        // *pszValue=new TCHAR[BufferSize/2+1];// 1 wchar_t is 2 bytes
        *pszValue=(TCHAR*)HeapAlloc(mhLogHeap,0,(BufferSize/2+1)*sizeof(WCHAR));
        memcpy(*pszValue,Buffer,BufferSize);
        (*pszValue)[BufferSize/2]=0;
    }
    else
    {
        // *pszValue=new TCHAR[BufferSize+1];// 1 char is 1 byte
        *pszValue=(TCHAR*)HeapAlloc(mhLogHeap,0,(BufferSize+1)*sizeof(WCHAR));
        // do not use CAnsiUnicodeConvert::AnsiToUnicode as Buffer is not a terminated string
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)Buffer, BufferSize,*pszValue,(int)BufferSize);
        (*pszValue)[BufferSize]=0;
    }
#else
    if (bUnicodeFile)
    {
        // *pszValue=new TCHAR[BufferSize/2+1];// 1 wchar_t is 2 byte
        *pszValue=(TCHAR*)HeapAlloc(mhLogHeap,0,BufferSize/2+1);
        // do not use CAnsiUnicodeConvert::UnicodeToAnsi as Buffer is not a terminated string
        WideCharToMultiByte(CP_ACP, 0,(LPCWSTR)Buffer, BufferSize/2, *pszValue, BufferSize/2, NULL, NULL);
        (*pszValue)[BufferSize/2]=0;
    }
    else
    {
        // *pszValue=new TCHAR[BufferSize+1];// 1 char is 1 byte
        *pszValue=(TCHAR*)HeapAlloc(mhLogHeap,0,BufferSize+1);
        memcpy(*pszValue,Buffer,BufferSize);
        (*pszValue)[BufferSize]=0;
    }
#endif

    // delete Buffer;
    HeapFree(mhLogHeap,0,Buffer);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WriteXMLValue
// Object: write dword. 
// Parameters :
//     in  : HANDLE hFile : file handle
//           TCHAR* Markup : xml markup
//           DWORD Value : value to write
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void COpenSave::WriteXMLValue(HANDLE hFile,TCHAR* Markup,DWORD Value)
{
    DWORD dwWrittenBytes;
    TCHAR psz[16];
    DWORD MarkupLength=(DWORD)_tcslen(Markup);
    WriteFile(hFile,_T("<"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    _stprintf(psz,_T("0x%X"),Value);
    WriteFile(hFile,psz,(DWORD)_tcslen(psz)*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T("</"),2*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: WriteXMLValue
// Object: write dword. 
// Parameters :
//     in  : HANDLE hFile : file handle
//           TCHAR* Markup : xml markup
//           DWORD Value : value to write
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void COpenSave::WriteXMLValue(HANDLE hFile,TCHAR* Markup,PBYTE Value)
{
    DWORD dwWrittenBytes;
    TCHAR psz[16];
    DWORD MarkupLength=(DWORD)_tcslen(Markup);
    WriteFile(hFile,_T("<"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    _stprintf(psz,_T("0x%p"),Value);
    WriteFile(hFile,psz,(DWORD)_tcslen(psz)*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T("</"),2*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: WriteXMLValue
// Object: write byte buffer. 
// Parameters :
//     in  : HANDLE hFile : file handle
//           TCHAR* Markup : xml markup
//           PBYTE Buffer : buffer to write
//           DWORD BufferLengthInByte : size of buffer
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void COpenSave::WriteXMLValue(HANDLE hFile,TCHAR* Markup,PBYTE Buffer,DWORD BufferLengthInByte)
{
    DWORD dwWrittenBytes;
    TCHAR psz[16];
    DWORD MarkupLength=(DWORD)_tcslen(Markup);
    WriteFile(hFile,_T("<"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);

    for (DWORD cnt=0;cnt<BufferLengthInByte;cnt++)
    {
        _stprintf(psz,_T("%.2X"),Buffer[cnt]);
        WriteFile(hFile,psz,(DWORD)_tcslen(psz)*sizeof(TCHAR),&dwWrittenBytes,NULL);
    }

    WriteFile(hFile,_T("</"),2*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: WriteXMLValue
// Object: write string. 
// Parameters :
//     in  : HANDLE hFile : file handle
//           TCHAR* Markup : xml markup
//           TCHAR* Value : string to write
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void COpenSave::WriteXMLValue(HANDLE hFile,TCHAR* Markup,TCHAR* Value)
{
    if (Value==NULL)
    {
        TCHAR t=0;
        COpenSave::WriteXMLValue(hFile,Markup,(BYTE*)&t,sizeof(TCHAR));
        return;
    }
    // convert to byte array to avoid a char to xml convertion at saving 
    //    and xml to char convertion at loading
    COpenSave::WriteXMLValue(hFile,Markup,(BYTE*)Value,(DWORD)_tcslen(Value)*sizeof(TCHAR));
}