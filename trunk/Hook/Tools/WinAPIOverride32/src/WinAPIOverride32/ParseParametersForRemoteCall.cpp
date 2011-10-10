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
// Object: user input parameter parsing
//-----------------------------------------------------------------------------

#include "parseparametersforremotecall.h"

CParseParametersForRemoteCall::CParseParametersForRemoteCall(void)
{
    this->pLinkListParams=new CLinkList(sizeof(STRUCT_FUNC_PARAM));
}

CParseParametersForRemoteCall::~CParseParametersForRemoteCall(void)
{
    this->EmptyParamList();
    delete this->pLinkListParams;
}

//-----------------------------------------------------------------------------
// Name: EmptyParamList
// Object: free memory allocated by a call of Parse
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CParseParametersForRemoteCall::EmptyParamList()
{
    CLinkListItem* pItem;
    STRUCT_FUNC_PARAM* pParam;
    for(pItem=this->pLinkListParams->Head;pItem;pItem=pItem->NextItem)
    {
        pParam=(STRUCT_FUNC_PARAM*)pItem->ItemData;
        if (pParam->pData)
            // delete associated data
            delete pParam->pData;
    }
    this->pLinkListParams->RemoveAllItems();
}

//-----------------------------------------------------------------------------
// Name: IsVARIANT_BYREF
// Object: check if psz begins with "_BYREF"
// Parameters :
//     in  : 
//     out : psz: is trimmed and the potential "_BYREF" is removed
//     return : return TRUE if psz begins with "_BYREF"
//-----------------------------------------------------------------------------
BOOL CParseParametersForRemoteCall::IsVARIANT_BYREF(IN OUT TCHAR* psz)
{
    CTrimString::TrimString(psz);
    // if string doesn't begin with "_BYREF"
    if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_BYREF,_tcslen(CParseParametersForRemoteCall_VARIANT_BYREF)))
        // nothing to do
        return FALSE;

    size_t PrefixSize=_tcslen(CParseParametersForRemoteCall_VARIANT_BYREF);
    size_t Size=_tcslen(psz);
    // move memory
    memmove((PBYTE)psz,(PBYTE)(psz+PrefixSize),(Size-PrefixSize+1)*sizeof(TCHAR));

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse user entry
//         warning change content of pszParams
//         ppParams must be deleted by caller delete (*ppParams) --> delete pParam; if called with &pParam
// Parameters :
//     in  : IN TCHAR* pszParams : user entry
//           size_t ErrorMessageMaxSize : error message max size
//     out : OUT STRUCT_FUNC_PARAM** ppParams : decoded parameters array
//           OUT DWORD* pNbParams : number of parameters
//           OUT BOOL* pbAreSomeParameterPassedAsRef : specified if parameters are passed by ref
//           OUT TCHAR* ErrorMessage : error message content
//     return : return TRUE on success
//-----------------------------------------------------------------------------
BOOL CParseParametersForRemoteCall::Parse(IN TCHAR* pszParams,
                                            OUT STRUCT_FUNC_PARAM** ppParams,
                                            OUT DWORD* pNbParams,
                                            OUT BOOL* pbAreSomeParameterPassedAsRef,
                                            OUT TCHAR* ErrorMessage,
                                            size_t ErrorMessageMaxSize)
{

    BOOL bError=FALSE;
    TCHAR* pszRemainingParams;
    TCHAR* psz;
    TCHAR* pszParamsLocalCopy=NULL;
    TCHAR* pszSplitter;
    TCHAR* NextQuote;
    STRUCT_FUNC_PARAM Param;
    int iScanfRes;
    double DoubleValue;
    float FloatValue;
    PBYTE ParamValue;

    // clear previous parsing allocated values if any
    this->EmptyParamList();
    
    *ppParams=NULL;
    *pNbParams=0;
    *pbAreSomeParameterPassedAsRef=FALSE;
    if (ErrorMessage)
        *ErrorMessage=0;

    // check if empty arg func
    CTrimString::TrimString(pszParams);
    if (*pszParams==0)
        return TRUE;
    
    pszSplitter=pszParams;

    // from now params are splitted by ','
    while (pszSplitter)
    {
        // find next ','
        pszRemainingParams=_tcschr(pszSplitter,',');
        if (pszRemainingParams)
        {
            *pszRemainingParams=0;// ends current parameter representation
            pszRemainingParams++; // point to next param
        }
        // make pszParams point to next parameter
        pszSplitter=pszRemainingParams;

        // free previous local copy if needed
        if (pszParamsLocalCopy)
            free(pszParamsLocalCopy);

        // do a local copy of parameter (avoid to modify it in case of ',' char inside a string)
        pszParamsLocalCopy=_tcsdup(pszParams);
        if (!pszParamsLocalCopy)
        {
            _tcsncpy(ErrorMessage,
                _T("Strings can't be passed directly by reference.\r\n")
                _T("You have to allocate the string inside the hooked process, and next use pointer to the allocated buffer as argument"),
                ErrorMessageMaxSize
                );
            bError=TRUE;
            break;
        }
        // trim current param
        psz=CTrimString::TrimString(pszParamsLocalCopy);

        // clear current Param struct
        memset(&Param,0,sizeof(STRUCT_FUNC_PARAM));

        // by default param is considered not being passed as ref
        Param.bPassAsRef=FALSE; // quite useless after memset 0, but more understandable

        // if there's data
        if (!*psz)
        {
            bError=TRUE;
            break;
        }

        Param.pData=NULL;

        // check for pointed value
        if (*psz=='&')
        {
            // signal that some parameters are passed as pointer
            *pbAreSomeParameterPassedAsRef=TRUE;

            // set Param field value
            Param.bPassAsRef=TRUE;

            // point after '&'
            psz++;
        }

        // Double
        if (_tcsnicmp(psz,CParseParametersForRemoteCall_DOUBLE,_tcslen(CParseParametersForRemoteCall_DOUBLE))==0)
        {
            // point after Double=
            psz+=_tcslen(CParseParametersForRemoteCall_DOUBLE);

            iScanfRes=_stscanf(psz,_T("%le"),&DoubleValue);

            if (iScanfRes<=0)
            {
                bError=TRUE;
                break;
            }
            Param.dwDataSize=sizeof(double);
            // allocate memory
            Param.pData=new BYTE[Param.dwDataSize];
            // set allocated memory to 0
            memcpy(Param.pData,&DoubleValue,Param.dwDataSize);
        }

        // Float
        else if (_tcsnicmp(psz,CParseParametersForRemoteCall_FLOAT,_tcslen(CParseParametersForRemoteCall_FLOAT))==0)
        {
            // point after Float=
            psz+=_tcslen(CParseParametersForRemoteCall_FLOAT);

            iScanfRes=_stscanf(psz,_T("%e"),&FloatValue);

            if (iScanfRes<=0)
            {
                bError=TRUE;
                break;
            }

            Param.dwDataSize=sizeof(float);
            // allocate memory
            Param.pData=new BYTE[Param.dwDataSize];
            // set allocated memory to 0
            memcpy(Param.pData,&FloatValue,Param.dwDataSize);
        }

        // if struct or buffer
        else if (_tcsnicmp(psz,CParseParametersForRemoteCall_BUFFER,_tcslen(CParseParametersForRemoteCall_BUFFER))==0)
        {
            // point after Buffer=
            psz+=_tcslen(CParseParametersForRemoteCall_BUFFER);
            // only a byte buffer
            Param.pData=CStrToHex::StrHexArrayToByteArray(psz,&Param.dwDataSize);
        }
        // if ref OutValue
        else if (_tcsnicmp(psz,CParseParametersForRemoteCall_REF_OUTVALUE,_tcslen(CParseParametersForRemoteCall_REF_OUTVALUE))==0)
        {
            // check pointed value flag
            if (!Param.bPassAsRef)
            {
                bError=TRUE;
                break;
            }

            Param.dwDataSize=sizeof(PBYTE);
            // allocate memory
            Param.pData=new BYTE[Param.dwDataSize];
            // set allocated memory to 0
            memset(Param.pData,0,Param.dwDataSize);
        }
        // if ref OutBuffer[255]
        else if (_tcsnicmp(psz,CParseParametersForRemoteCall_REF_OUTBUFFER,_tcslen(CParseParametersForRemoteCall_REF_OUTBUFFER))==0)
        {
            // check pointed value flag
            if (!Param.bPassAsRef)
            {
                bError=TRUE;
                break;
            }

            // point after &OutBuffer
            psz+=_tcslen(CParseParametersForRemoteCall_REF_OUTBUFFER);
            // pc point to [255]
            iScanfRes=_stscanf(psz,_T("[%u]"),&Param.dwDataSize);
            if (iScanfRes<=0)
            {
                bError=TRUE;
                break;
            }
            // allocate memory
            Param.pData=new BYTE[Param.dwDataSize];
            // set allocated memory to 0
            memset(Param.pData,0,Param.dwDataSize);
        }
        // strings "String,\r\n..\\.\"ex\'\tok", L"String" warning to '\"'
        else if (    (_tcsnicmp(psz,CParseParametersForRemoteCall_ANSI_PREFIX,_tcslen(CParseParametersForRemoteCall_ANSI_PREFIX))==0)
                  || (_tcsnicmp(psz,CParseParametersForRemoteCall_UNICODE_PREFIX,_tcslen(CParseParametersForRemoteCall_UNICODE_PREFIX))==0)
                  )
        {
            BOOL bUnicode;
            bUnicode=(*psz==CParseParametersForRemoteCall_UNICODE_PREFIX_FIRSTCHAR);

            // if a '&' was entered before the string
            if (Param.bPassAsRef)
            {
                _tcsncpy(ErrorMessage,
                    _T("Strings can't be passed directly by reference.\r\n")
                    _T("You have to allocate the string inside the hooked process, and next use pointer to the allocated buffer as argument"),
                    ErrorMessageMaxSize
                    );
                bError=TRUE;
                break;
            }

            // point after '"'
            if (bUnicode)
                psz+=_tcslen(CParseParametersForRemoteCall_UNICODE_PREFIX);
            else
                psz+=_tcslen(CParseParametersForRemoteCall_ANSI_PREFIX);

            NextQuote=psz-1;
            for(;;) 
            {
                NextQuote++;
                // find next '"'
                NextQuote=_tcschr(NextQuote,'"');
                if (!NextQuote)
                    break;

                if (*(NextQuote-1))
                {
                    int cnt=0;
                    while(*(NextQuote-1-cnt)=='\\')
                    {
                        cnt++;
                    }
                    
                    // if number of \ is not multiple of 2, that means last \ applies to ",
                    // and so found " is not ending the string, so we have to found next "
                    if (cnt%2==1)
                        continue;
                }
                break;
            }
            
            // if ending '"' not found, ',' was not a splitter but is in the string
            if (!NextQuote)
            {
                // if there is no data after ','
                if (!pszRemainingParams)
                {
                    _tcsncpy(ErrorMessage,_T("Unterminatted string"),ErrorMessageMaxSize);
                    bError=TRUE;
                    break;
                }
                // else
                // restore ',' replaced by \0
                pszRemainingParams--;
                *pszRemainingParams=',';
                pszRemainingParams++;
                // find next splitter
                continue;
            }
            // remove end of char definition
            *NextQuote=0;

            // remove backslash from string
            CParseParametersForRemoteCall::RemoveBackslash(psz);

            // string are always pass by ref, so put flag for the struct
            Param.bPassAsRef=TRUE;

            // store string content according to it's encoding type
#if (defined(UNICODE)||defined(_UNICODE))
            if (bUnicode)
            {
                Param.dwDataSize=(DWORD)((wcslen(psz)+1)*sizeof(WCHAR));
                // allocate memory
                Param.pData=new BYTE[Param.dwDataSize];
                // copy memory
                memcpy(Param.pData,psz,Param.dwDataSize);
            }
            else
            {
                CHAR* pszAnsi;
                CAnsiUnicodeConvert::UnicodeToAnsi(psz,&pszAnsi);

                Param.dwDataSize=(DWORD)strlen(pszAnsi)+1;//*sizeof(CHAR)
                // allocate memory
                Param.pData=new BYTE[Param.dwDataSize];
                // copy memory
                memcpy(Param.pData,pszAnsi,Param.dwDataSize);

                free(pszAnsi);
            }
#else
            if (bUnicode)
            {
                WCHAR* pszUnicode;
                CAnsiUnicodeConvert::AnsiToUnicode(psz,&pszUnicode);

                Param.dwDataSize=(DWORD)((wcslen(pszUnicode)+1)*sizeof(WCHAR));
                // allocate memory
                Param.pData=new BYTE[Param.dwDataSize];
                // copy memory
                memcpy(Param.pData,pszUnicode,Param.dwDataSize);

                free(pszUnicode);
            }
            else
            {
                Param.dwDataSize=(DWORD)strlen(psz)+1;//*sizeof(CHAR)
                // allocate memory
                Param.pData=new BYTE[Param.dwDataSize];
                // copy memory
                memcpy(Param.pData,psz,Param.dwDataSize);
            }
#endif

        }
        // if VARIANT
        else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_PREFIX,_tcslen(CParseParametersForRemoteCall_VARIANT_PREFIX))==0)
        {
            VARIANT vt;
            // point after &OutBuffer
            psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_PREFIX);
            memset(&vt,0,sizeof(VARIANT));

            // find variant subtype
            
            // VT_EMPTY
            if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_EMPTY,_tcslen(CParseParametersForRemoteCall_VARIANT_EMPTY))==0)
            {
                // set variant type
                vt.vt=VT_EMPTY;

                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_EMPTY);

                // check pointer flag
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    _tcsncpy(ErrorMessage,
                        _T("VT_EMPTY can't be passe as ref.\r\n"),
                        ErrorMessageMaxSize
                        );
                    bError=TRUE;
                    break;
                }
            }
            // VT_NULL
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_NULL,_tcslen(CParseParametersForRemoteCall_VARIANT_NULL))==0)
            {
                // set variant type
                vt.vt=VT_NULL;

                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_NULL);

                // check pointer flag
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    _tcsncpy(ErrorMessage,
                        _T("VT_NULL can't be passe as ref.\r\n"),
                        ErrorMessageMaxSize
                        );
                    bError=TRUE;
                    break;
                }
            }
            // VT_I1
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_I1,_tcslen(CParseParametersForRemoteCall_VARIANT_I1))==0)
            {
                // set variant type
                vt.vt=VT_I1;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_I1);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pcVal);
                }
                else
                {
                    int i=0;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToSignedInt(psz,&i);
                    vt.cVal=(CHAR)(i&0xFF);
                }

                if (bError)
                    break;

            }
            // VT_I2
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_I2,_tcslen(CParseParametersForRemoteCall_VARIANT_I2))==0)
            {
                // set variant type
                vt.vt=VT_I2;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_I2);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.piVal);
                }
                else
                {
                    int i=0;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToSignedInt(psz,&i);
                    vt.iVal=(SHORT)(i&0xFFFF);
                }

                if (bError)
                    break;

            }
            // VT_I4
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_I4,_tcslen(CParseParametersForRemoteCall_VARIANT_I4))==0)
            {
                // set variant type
                vt.vt=VT_I4;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_I4);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.plVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToSignedInt(psz,(INT*)&vt.lVal);
                }

                if (bError)
                    break;
            }
            // VT_I8
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_I8,_tcslen(CParseParametersForRemoteCall_VARIANT_I8))==0)
            {
                // set variant type
                vt.vt=VT_I8;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_I8);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pllVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToSignedInt64(psz,(INT64*)&vt.llVal);
                }

                if (bError)
                    break;
            }
            // VT_INT
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_INT,_tcslen(CParseParametersForRemoteCall_VARIANT_INT))==0)
            {
                // set variant type
                vt.vt=VT_INT;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_INT);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pintVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToSignedInt(psz,&vt.intVal);
                }

                if (bError)
                    break;
            }
 
            // VT_UI1
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_UI1,_tcslen(CParseParametersForRemoteCall_VARIANT_UI1))==0)
            {
                // set variant type
                vt.vt=VT_UI1;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_UI1);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pbVal);
                }
                else
                {
                    UINT i=0;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToUnsignedInt(psz,&i);
                    vt.bVal=(BYTE)(i&0xFF);
                }

                if (bError)
                    break;

            }
            // VT_UI2
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_UI2,_tcslen(CParseParametersForRemoteCall_VARIANT_UI2))==0)
            {
                // set variant type
                vt.vt=VT_UI2;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_UI2);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.puiVal);
                }
                else
                {
                    UINT i=0;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToUnsignedInt(psz,&i);
                    vt.uiVal=(USHORT)(i&0xFFFF);
                }

                if (bError)
                    break;

            }
            // VT_UI4
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_UI4,_tcslen(CParseParametersForRemoteCall_VARIANT_UI4))==0)
            {
                // set variant type
                vt.vt=VT_UI4;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_UI4);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pulVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToUnsignedInt(psz,(UINT*)&vt.ulVal);
                }

                if (bError)
                    break;
            }
            // VT_UI8
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_UI8,_tcslen(CParseParametersForRemoteCall_VARIANT_UI8))==0)
            {
                // set variant type
                vt.vt=VT_UI8;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_UI8);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pullVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToUnsignedInt64(psz,(UINT64*)&vt.ullVal);
                }

                if (bError)
                    break;
            }
            // VT_UINT
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_UINT,_tcslen(CParseParametersForRemoteCall_VARIANT_UINT))==0)
            {
                // set variant type
                vt.vt=VT_UINT;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_UINT);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.puintVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToUnsignedInt(psz,&vt.uintVal);
                }

                if (bError)
                    break;
            }
            // VT_CY
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_CY,_tcslen(CParseParametersForRemoteCall_VARIANT_CY))==0)
            {
                // set variant type
                vt.vt=VT_CY;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_CY);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pcyVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToSignedInt64(psz,(INT64*)&vt.cyVal);
                }

                if (bError)
                    break;
            }
            // VT_BOOL
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_BOOL,_tcslen(CParseParametersForRemoteCall_VARIANT_BOOL))==0)
            {
                // set variant type
                vt.vt=VT_BOOL;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_BOOL);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pboolVal);
                }
                else
                {
                    UINT i=0;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToUnsignedInt(psz,&i);
                    vt.boolVal=(USHORT)(i&0xFFFF);
                }

                if (bError)
                    break;

            }
            // VT_ERROR
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_ERROR,_tcslen(CParseParametersForRemoteCall_VARIANT_ERROR))==0)
            {
                // set variant type
                vt.vt=VT_ERROR;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_ERROR);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pscode);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToSignedInt(psz,(INT*)&vt.scode);
                }

                if (bError)
                    break;
            }

            // VT_R4
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_R4,_tcslen(CParseParametersForRemoteCall_VARIANT_R4))==0)
            {
                // set variant type
                vt.vt=VT_R4;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_R4);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pfltVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToFloat(psz,&vt.fltVal);
                }

                if (bError)
                    break;
            }
            // VT_R8
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_R8,_tcslen(CParseParametersForRemoteCall_VARIANT_R8))==0)
            {
                // set variant type
                vt.vt=VT_R8;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_R8);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pdblVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToDouble(psz,&vt.dblVal);
                }

                if (bError)
                    break;
            }

            // VT_DATE
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_DATE,_tcslen(CParseParametersForRemoteCall_VARIANT_DATE))==0)
            {
                // set variant type
                vt.vt=VT_DATE;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_DATE);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pdate);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToDouble(psz,&vt.date);
                }

                if (bError)
                    break;
            }

            // VT_DISPATCH
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_DISPATCH,_tcslen(CParseParametersForRemoteCall_VARIANT_DISPATCH))==0)
            {
                // set variant type
                vt.vt=VT_DISPATCH;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_DISPATCH);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.ppdispVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pdispVal);
                }

                if (bError)
                    break;
            }

            // VT_UNKNOWN
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_UNKNOWN,_tcslen(CParseParametersForRemoteCall_VARIANT_UNKNOWN))==0)
            {
                // set variant type
                vt.vt=VT_UNKNOWN;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_UNKNOWN);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.ppunkVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.punkVal);
                }

                if (bError)
                    break;
            }

            // VT_BSTR
            else if (_tcsnicmp(psz,CParseParametersForRemoteCall_VARIANT_BSTR,_tcslen(CParseParametersForRemoteCall_VARIANT_BSTR))==0)
            {
                // set variant type
                vt.vt=VT_BSTR;

                bError=FALSE;
                psz+=_tcslen(CParseParametersForRemoteCall_VARIANT_BSTR);
                if (CParseParametersForRemoteCall::IsVARIANT_BYREF(psz))
                {
                    vt.vt|=VT_BYREF;
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.pbstrVal);
                }
                else
                {
                    psz++;// point after '='
                    bError=!CStringConverter::StringToPBYTE(psz,(PBYTE*)&vt.bstrVal);
                }

                if (bError)
                    break;
            }
            else
            {
                _tcsncpy(ErrorMessage,
                    _T("A type of VARIANT is not directly supported.\r\n")
                    _T("You have to set parameter as a buffer using \"Buffer=\" or \"&Buffer=\""),
                    ErrorMessageMaxSize
                    );
                bError=TRUE;
                break;
            }

            // on success copy initialized variant struct
            Param.dwDataSize=sizeof(VARIANT);
            // allocate memory
            Param.pData=new BYTE[Param.dwDataSize];
            // copy memory
            memcpy(Param.pData,&vt,Param.dwDataSize);

        }
        else // only a value
        {
            if(!CStringConverter::StringToPBYTE(psz,&ParamValue))
            {
                bError=TRUE;
                break;
            }
            Param.dwDataSize=sizeof(PBYTE);
            // allocate memory
            Param.pData=new BYTE[Param.dwDataSize];
            // put param value into our allocated buffer
            memcpy(Param.pData,&ParamValue,Param.dwDataSize);
        }

        // add param to list
        this->pLinkListParams->AddItem((&Param));

        // get next parameter (waring : pszParams musn't be updated in case of unterminated string)
        pszParams=pszSplitter;
    }

    if (pszParamsLocalCopy)
        free(pszParamsLocalCopy);

    // in case of parsing error
    if (bError)
    {
        // if ErrorMessage buffer is significant
        if (ErrorMessage)
        {
            // if ErrorMessage hasn't already been fill
            if (!*ErrorMessage)
                _sntprintf(ErrorMessage,ErrorMessageMaxSize,_T("Error Retreiving value of parameter %u"),this->pLinkListParams->GetItemsCount()+1);
        }
        return FALSE;
    }


    *ppParams=(STRUCT_FUNC_PARAM*)this->pLinkListParams->ToSecureArray(pNbParams);
    if (!(*ppParams))
    {
        *pNbParams=0;
        // if ErrorMessage buffer is significant
        if (ErrorMessage)
        {
            // if ErrorMessage hasn't already been fill
            if (!*ErrorMessage)
                _sntprintf(ErrorMessage,ErrorMessageMaxSize,_T("Error Retreiving parameters"));
        }
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveBackslash
// Object: replace string special chars by special chars
//          ex: "\r\n"
//          warning psz is modified
// Parameters :
//     in  : TCHAR* psz
//     out :
//     return : string with decoded special chars
//-----------------------------------------------------------------------------
TCHAR* CParseParametersForRemoteCall::RemoveBackslash(TCHAR* psz)
{
    size_t pszLen;
    size_t CntDest=0;
    size_t CntSrc=0;

    pszLen=_tcslen(psz);

    // for all TCHAR including \0
    for (;CntSrc<=pszLen;CntSrc++,CntDest++)
    {
        if (psz[CntSrc]=='\\')
        {
            CntSrc++;
            switch(psz[CntSrc])
            {
            case 'b':
                psz[CntDest]='\b';
                break;
            case 'f':
                psz[CntDest]='\f';
                break;
            case 'r':
                psz[CntDest]='\r';
                break;
            case 'n':
                psz[CntDest]='\n';
                break;
            case 't':
                psz[CntDest]='\t';
                break;
            case '\\':
                psz[CntDest]='\\';
                break;
            case '\'':
                psz[CntDest]='\'';
                break;
            case '"':
                psz[CntDest]='"';
                break;
            default:
                // not supported special char, copy '\\'
                psz[CntDest]='\\';
                CntDest++;
                psz[CntDest]=psz[CntSrc];
                break;
            }
        }
        else
            psz[CntDest]=psz[CntSrc];
    }
    return psz;
}