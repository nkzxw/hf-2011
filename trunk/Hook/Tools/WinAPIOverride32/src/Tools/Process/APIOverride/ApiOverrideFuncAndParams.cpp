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
// Object: class helper for doing remote function call in other processes with apioverride
//         it manages encoding and decoding of function and parameters
//-----------------------------------------------------------------------------

#include "ApiOverrideFuncAndParams.h"

CApiOverrideFuncAndParams::CApiOverrideFuncAndParams(void)
{
    this->DecodedLibName=NULL;
    this->DecodedFuncName=NULL;
    this->DecodedNbParams=0;
    this->DecodedParams=NULL;
    this->DecodedReturnedValue=0;
    this->DecodedCallSuccess=FALSE;
    this->DecodedThreadID=0;
    this->DecodedTimeOut=INFINITE;

    this->EncodedBuffer=NULL;
    this->EncodedBufferSize=0;

    memset(&this->DecodedRegisters,0,sizeof(REGISTERS));
}

CApiOverrideFuncAndParams::~CApiOverrideFuncAndParams(void)
{
    this->FreeDecodedMembers();
    this->FreeEncodedMembers();
}
//-----------------------------------------------------------------------------
// Name: FreeEncodedMembers
// Object: free all encoded members
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CApiOverrideFuncAndParams::FreeEncodedMembers()
{
    if (this->EncodedBuffer)
    {
        delete[] this->EncodedBuffer;
        this->EncodedBuffer=NULL;
    }
}
//-----------------------------------------------------------------------------
// Name: FreeDecodedMembers
// Object: Free all decoded members
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CApiOverrideFuncAndParams::FreeDecodedMembers()
{
    // free DecodedLibName
    if (this->DecodedLibName)
    {
        delete this->DecodedLibName;
        this->DecodedLibName=NULL;
    }
    // free DecodedFuncName
    if (this->DecodedFuncName)
    {
        delete this->DecodedFuncName;
        this->DecodedFuncName=NULL;
    }

    // free DecodedParams
    if (this->DecodedParams)
    {
        for(DWORD Cnt=0;Cnt<this->DecodedNbParams;Cnt++)
            delete this->DecodedParams[Cnt].pData;
        delete this->DecodedParams;
        this->DecodedParams=NULL;
    }
}


//-----------------------------------------------------------------------------
// Name: Encode
// Object: fill EncodedBuffer and EncodedBufferSize members (don't worry about memory allocation/desallocation for these members)
//         This func make a buffer containing all required informations for a remote host.
//         You can use any communication way (pipe, mailslot, tcp ...) to transmit params to remote host
//         To retrieve parameters for a given buffer just use the Decode func
// Parameters :
//     in  : PVOID ID : ID of the call
//           LPTSTR LibName : dll name or EXE_INTERNAL@0xAddr
//           LPTSTR FuncName : func name
//           DWORD NbParams : number of params
//           PSTRUCT_FUNC_PARAM pParams array of STRUCT_FUNC_PARAM for calling func in remote process
//           REGISTERS* pRegisters : registers
//           DWORD ThreadID : thread id into which call must be done, 0 if no thread preference
//     out :
//     return : FALSE if bad pointer in pParams
//              TRUE on success
//-----------------------------------------------------------------------------
BOOL CApiOverrideFuncAndParams::Encode(PVOID ID,TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,DWORD ThreadID,DWORD dwTimeOut,tagCALLING_CONVENTION CallingConvention)
{
    return this->Encode(ID,LibName,FuncName,NbParams,pParams,pRegisters,0,0.0,FALSE,ThreadID,dwTimeOut,CallingConvention);
}

//-----------------------------------------------------------------------------
// Name: Encode
// Object: fill EncodedBuffer and EncodedBufferSize members (don't worry about memory allocation/desallocation for these members)
//         This func make a buffer containing all required informations for a remote host.
//         You can use any communication way (pipe, mailslot, tcp ...) to transmit params to remote host
//         To retrieve parameters for a given buffer just use the Decode func
// Parameters :
//     in  : PVOID ID : ID of the call
//           TCHAR* LibName : dll name or EXE_INTERNAL@0xAddr
//           TCHAR* FuncName : func name
//           DWORD NbParams : number of params
//           PSTRUCT_FUNC_PARAM pParams array of STRUCT_FUNC_PARAM for calling func in remote process
//           PBYTE ReturnValue : return value of called func
//           REGISTERS* pRegisters : registers
//           PBYTE ReturnValue : function return
//           double FloatingReturn : floating return
//           BOOL CallSuccess : function call status
//           DWORD ThreadID : thread id into which call must be done, 0 if no thread preference
//     out :
//     return : FALSE if bad pointer in pParams
//              TRUE on success
//-----------------------------------------------------------------------------
BOOL CApiOverrideFuncAndParams::Encode(PVOID ID,LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE ReturnValue,double FloatingReturn,BOOL CallSuccess,DWORD ThreadID,DWORD dwTimeOut,tagCALLING_CONVENTION CallingConvention)
{
    // pBuffer:
    // DWORD BufferSize
    // PVOID ID
    // DWORD Return
    // DWORD LibNameSize(in TCHAR)|LibName|
    // DWORD FuncNameSize(in TCHAR)|FuncName|
    // DWORD NbParams
    // for each param
    // BOOL bPassAsRef|DWORD dwDataSize|Data
    // REGISTERS Registers
    // double FloatingReturn
    // BOOL CallSuccess
    // DWORD ThreadID
    // DWORD dwTimeOut
    // CALLING_CONVENTION CallingConvention (on DWORD)

    DWORD dwSize;
    DWORD dwTmp;
    DWORD dwPos;
    DWORD dwCnt;
    TCHAR pszMsg[2*MAX_PATH];
    
    this->FreeEncodedMembers();

    PBYTE pBuffer;

    ////////////////////////////////////////
    // compute required buffer size
    ////////////////////////////////////////
    dwSize=(DWORD)sizeof(DWORD)+sizeof(PBYTE); // for DWORD BufferSize,PBYTE Return
    // PVOID ID
    dwSize+=(DWORD)sizeof(PVOID);
    // DWORD LibNameSize|LibName
    dwSize+=(DWORD)sizeof(DWORD)+_tcslen(LibName)*sizeof(TCHAR);
    // DWORD FuncNameSize|FuncName
    dwSize+=(DWORD)sizeof(DWORD)+_tcslen(FuncName)*sizeof(TCHAR);
    // DWORD NbParams
    dwSize+=(DWORD)sizeof(DWORD);
    // params needed size
    for(dwCnt=0;dwCnt<NbParams;dwCnt++)
        dwSize+=pParams[dwCnt].dwDataSize+sizeof(BOOL)+sizeof(DWORD);//for BOOL bPassAsRef and DWORD dwDataSize
    // REGISTERS Registers
    dwSize+=(DWORD)sizeof(REGISTERS);
    // floating return
    dwSize+=(DWORD)sizeof(double);
    // call success
    dwSize+=(DWORD)sizeof(BOOL);
    // ThreadID
    dwSize+=(DWORD)sizeof(DWORD);
    // TimeOut
    dwSize+=(DWORD)sizeof(DWORD);
    // Calling Convention
    dwSize+=(DWORD)sizeof(DWORD);

    pBuffer=new BYTE[dwSize];
    ////////////////////////////////////////
    // fill buffer
    ////////////////////////////////////////
    
    // fill BufferSize
    memcpy(pBuffer,&dwSize,sizeof(DWORD));
    dwPos=(DWORD)sizeof(DWORD);

    // fill ID
    memcpy(&pBuffer[dwPos],&ID,sizeof(PVOID));
    dwPos+=(DWORD)sizeof(PVOID);

    // fill Return
    memcpy(&pBuffer[dwPos],&ReturnValue,sizeof(PBYTE));
    dwPos+=(DWORD)sizeof(DWORD);
    // LibNameSize
    dwTmp=(DWORD)_tcslen(LibName);
    memcpy(&pBuffer[dwPos],&dwTmp,sizeof(DWORD));
    dwPos+=(DWORD)sizeof(DWORD);
    // LibName
    memcpy(&pBuffer[dwPos],LibName,dwTmp*sizeof(TCHAR));
    dwPos+=dwTmp*sizeof(TCHAR);
    // FuncNameSize
    dwTmp=(DWORD)_tcslen(FuncName);
    memcpy(&pBuffer[dwPos],&dwTmp,sizeof(DWORD));
    dwPos+=(DWORD)sizeof(DWORD);
    // FuncName
    memcpy(&pBuffer[dwPos],FuncName,dwTmp*sizeof(TCHAR));
    dwPos+=dwTmp*sizeof(TCHAR);

    // NbParams
    memcpy(&pBuffer[dwPos],&NbParams,sizeof(DWORD));
    dwPos+=sizeof(DWORD);

    // for each param
    for (dwCnt=0;dwCnt<NbParams;dwCnt++)
    {
        // BOOL bPassAsRef|DWORD dwDataSize|Data
        memcpy(&pBuffer[dwPos],&pParams[dwCnt].bPassAsRef,sizeof(DWORD));
        dwPos+=sizeof(DWORD);
        memcpy(&pBuffer[dwPos],&pParams[dwCnt].dwDataSize,sizeof(DWORD));
        dwPos+=sizeof(DWORD);
        // check we have given pointer to param not param value
        if (IsBadReadPtr(pParams[dwCnt].pData,pParams[dwCnt].dwDataSize))
        {
            _sntprintf(pszMsg,2*MAX_PATH,_T("Error encoding parameter %d for function call %s in module %s.\r\nMake sure you give pointer to parameter not parameter value."),dwCnt,FuncName,LibName);
#if (!defined(TOOLS_NO_MESSAGEBOX))
            MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
#endif
            delete[] pBuffer;
            this->EncodedBufferSize=0;
            return FALSE;
        }
        memcpy(&pBuffer[dwPos],pParams[dwCnt].pData,pParams[dwCnt].dwDataSize);
        dwPos+=pParams[dwCnt].dwDataSize;
    }

    memcpy(&pBuffer[dwPos],pRegisters,sizeof(REGISTERS));
    dwPos+=sizeof(REGISTERS);

    memcpy(&pBuffer[dwPos],&FloatingReturn,sizeof(double));
    dwPos+=sizeof(double);

    memcpy(&pBuffer[dwPos],&CallSuccess,sizeof(BOOL));
    dwPos+=sizeof(BOOL);

    memcpy(&pBuffer[dwPos],&ThreadID,sizeof(DWORD));
    dwPos+=sizeof(DWORD);

    memcpy(&pBuffer[dwPos],&dwTimeOut,sizeof(DWORD));
    dwPos+=sizeof(DWORD);

    memcpy(&pBuffer[dwPos],&CallingConvention,sizeof(DWORD));
    dwPos+=sizeof(DWORD);

    this->EncodedBuffer=pBuffer;
    this->EncodedBufferSize=dwSize;

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: Decode
// Object: fill all DecodeXXX members (don't worry about memory allocation/freeing for these members)
//         This func decode a buffer containing all informations from a remote host.
//         You can use any communication way (pipe, mailslot, tcp ...) to receive params from remote host
// Parameters :
//     in  : PBYTE pBuffer : a buffer encoded with Encode func
//     out :
//     return : FALSE on error
//              TRUE on success
//-----------------------------------------------------------------------------
BOOL CApiOverrideFuncAndParams::Decode(PBYTE pBuffer)
{
    // pBuffer:
    // DWORD BufferSize|
    // PVOID ID
    // PBYTE Return|
    // DWORD LibNameSize(in TCHAR)|LibName|
    // DWORD FuncNameSize(in TCHAR)|FuncName|
    // DWORD NbParams
    // for each param
    // BOOL bPassAsRef|DWORD dwDataSize|Data
    // REGISTERS Registers
    // double FloatingReturn
    // BOOL CallSuccess
    // DWORD ThreadID
    // DWORD Timeout
    // CALLING_CONVENTION CallingConvention (on DWORD)
    
    DWORD dwSize;
    DWORD dwCnt;

    this->FreeDecodedMembers();

    // get buffer size
    DWORD dwBufferSize;
    memcpy(&dwBufferSize,pBuffer,sizeof(DWORD));
    // check buffer size
    if (IsBadReadPtr(pBuffer,dwBufferSize))
        return FALSE;
    pBuffer+=sizeof(DWORD);

    // get ID
    memcpy(&this->DecodedID,pBuffer,sizeof(PVOID));
    pBuffer+=sizeof(PVOID);

    // get ret value
    memcpy(&this->DecodedReturnedValue,pBuffer,sizeof(PBYTE));
    pBuffer+=sizeof(DWORD);

    // get lib name
    memcpy(&dwSize,pBuffer,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);
    if (IsBadReadPtr(pBuffer,dwSize))
        return FALSE;
    this->DecodedLibName=new TCHAR[dwSize+1];
    _tcsncpy(this->DecodedLibName,(TCHAR*)pBuffer,dwSize);
    this->DecodedLibName[dwSize]=0;
    pBuffer+=dwSize*sizeof(TCHAR);

    // get func name
    memcpy(&dwSize,pBuffer,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);
    if (IsBadReadPtr(pBuffer,dwSize))
        return FALSE;
    this->DecodedFuncName=new TCHAR[dwSize+1];
    _tcsncpy(this->DecodedFuncName,(TCHAR*)pBuffer,dwSize);
    this->DecodedFuncName[dwSize]=0;
    pBuffer+=dwSize*sizeof(TCHAR);
    
    // get nb params
    memcpy(&this->DecodedNbParams,pBuffer,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);

    // get params
    this->DecodedParams=new STRUCT_FUNC_PARAM[this->DecodedNbParams];
    for(dwCnt=0;dwCnt<this->DecodedNbParams;dwCnt++)
    {
        memcpy(&this->DecodedParams[dwCnt].bPassAsRef,pBuffer,sizeof(BOOL));
        pBuffer+=sizeof(BOOL);
        memcpy(&this->DecodedParams[dwCnt].dwDataSize,pBuffer,sizeof(DWORD));
        pBuffer+=sizeof(DWORD);
        this->DecodedParams[dwCnt].pData=new BYTE[this->DecodedParams[dwCnt].dwDataSize];
        memcpy(this->DecodedParams[dwCnt].pData,pBuffer,this->DecodedParams[dwCnt].dwDataSize);
        pBuffer+=this->DecodedParams[dwCnt].dwDataSize;
    }

    // registers
    memcpy(&this->DecodedRegisters,pBuffer,sizeof(REGISTERS));
    pBuffer+=sizeof(REGISTERS);

    // floating return
    memcpy(&this->DecodedFloatingReturn,pBuffer,sizeof(double));
    pBuffer+=sizeof(double);

    // call success
    memcpy(&this->DecodedCallSuccess,pBuffer,sizeof(BOOL));
    pBuffer+=sizeof(BOOL);

    // ThreadID
    memcpy(&this->DecodedThreadID,pBuffer,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);

    // Timeout
    memcpy(&this->DecodedTimeOut,pBuffer,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);

    // CALLING_CONVENTION CallingConvention (on DWORD)
    // Timeout
    memcpy(&this->DecodedCallingConvention,pBuffer,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);

    return TRUE;
}