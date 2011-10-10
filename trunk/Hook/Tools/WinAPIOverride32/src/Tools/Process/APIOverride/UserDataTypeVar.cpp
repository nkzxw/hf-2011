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
// Object: manage user data type var (an union/struct var name) : an user data type this an associated var name, 
//          number of items (in case of array), bit fields infos, the number of referenced time
//-----------------------------------------------------------------------------
#include "UserDataTypeVar.h"

#include "../../String/SecureTcscat.h"

#pragma intrinsic (memcpy,memset,memcmp)

CUserDataTypeVar::CUserDataTypeVar()
{
    this->bParentIsUnion = FALSE;
    this->NbPointedTimes = 0;
    this->NbItems = 1;
    this->pUserDataType = NULL;
    *this->VarName=0;

    this->pBitsFieldInfosList = NULL; 
    this->BitsFieldsUsedSize = 0;
}

CUserDataTypeVar::~CUserDataTypeVar()
{

    if (this->pBitsFieldInfosList)
        delete this->pBitsFieldInfosList;
}

//-----------------------------------------------------------------------------
// Name: SetName
// Object: set var name
// Parameters :
//      const TCHAR* VarName : var name
// Return : 
//-----------------------------------------------------------------------------
void CUserDataTypeVar::SetName(const TCHAR* VarName)
{
    _tcsncpy(this->VarName,VarName,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1);
    this->VarName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;
}

//-----------------------------------------------------------------------------
// Name: GetSize
// Object: get the size in BYTES of the var according to it's type and item numbers
//          Warning : return a size aligned on register size (32 bits on a 32 bits processor 64 bits on a 64 bits processor)
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
SIZE_T CUserDataTypeVar::GetSize()
{
    if (this->NbPointedTimes>0)
        return GetPointerSize();
    
    // stack size or aligned size is the same
    SIZE_T Size = this->NbItems*this->pUserDataType->GetSize();
    if (Size<REGISTER_BYTE_SIZE)
        return REGISTER_BYTE_SIZE;
    SIZE_T Remain = Size%REGISTER_BYTE_SIZE;
    if (!Remain)
        return Size;
    else
        return Size+REGISTER_BYTE_SIZE-Remain;// supposed to be aligned
}

//-----------------------------------------------------------------------------
// Name: CheckStringEnd
// Object: find if string representation should be stopped due to sufficient char number filling
// Parameters :
//      TCHAR* String : current string
//      SIZE_T NbRequieredChars : number of char requiered
// Return : TRUE if we should to continue string representation, FALSE if string size is enought
//-----------------------------------------------------------------------------
BOOL CUserDataTypeVar::CheckStringEnd(TCHAR* String, SIZE_T NbRequieredChars)
{
    if (NbRequieredChars == 0)
        return TRUE;
    // if string size is bigger than required char numbers
    if ( NbRequieredChars < _tcslen(String) )
    {
        // if possible, add "..." at the end of string to tell user string representation is not fully completed
        if (NbRequieredChars>3)
        {
            String[NbRequieredChars-3]=0;
            _tcscat(String,_T("..."));
        }
        else
        {
            String[NbRequieredChars]=0;
        }

        // stop parsing
        return FALSE;
    }
    // continue parsing
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsBitsFields
// Object: return TRUE if current type is seen has a bits fields
// Parameters :
// Return : TRUE if current type is seen has a bits fields
//-----------------------------------------------------------------------------
BOOL CUserDataTypeVar::IsBitsFields()
{
    return (this->pBitsFieldInfosList != 0);
}

//-----------------------------------------------------------------------------
// Name: GetBitsFieldRemainingSize
// Object: return the remaining available size for bits fields
// Parameters :
// Return : the remaining available size for bits fields
//-----------------------------------------------------------------------------
SIZE_T CUserDataTypeVar::GetBitsFieldRemainingSize()
{
    // assume we are a bitfield
    if (!this->pBitsFieldInfosList)
        return 0;

    return (this->GetSize()*8 - this->BitsFieldsUsedSize);// *8 from bytes count to bits count
}

//-----------------------------------------------------------------------------
// Name: AddBitsField
// Object: add a bits field to current var
//          WARNING if FALSE is returned, pBitsFieldInfos is not added
// Parameters :
//      IN BITS_FIELD_INFOS_LITE* const pBitsFieldInfos : bits field infos
// Return : TRUE if bitfield can be added to current var FALSE else
//-----------------------------------------------------------------------------
BOOL CUserDataTypeVar::AddBitsField(IN BITS_FIELD_INFOS_LITE* const pBitsFieldInfosLite)
{
    // if current var is not marked as a bits fields one, do it
    if (!this->pBitsFieldInfosList)
    {
        this->SetName(_T("Bits Field"));
        this->pBitsFieldInfosList = new CLinkList(sizeof(CUserDataTypeVar::BITS_FIELD_INFOS));
    }

    BITS_FIELD_INFOS BitsFieldInfos={0};

    // get current type remaining available size
    SIZE_T RemainingSize = this->GetBitsFieldRemainingSize();

    // check if bits field size can be put in current type
    if (pBitsFieldInfosLite->Size > RemainingSize)
        return FALSE;

    // TYPE Unused : 0; --> Force alignment to next boundary.
    if (pBitsFieldInfosLite->Size == 0)
        pBitsFieldInfosLite->Size = RemainingSize;

    // compute shifting and mask
    BitsFieldInfos.Shift = this->BitsFieldsUsedSize;
    BitsFieldInfos.MaskAfterShifting = 0;
    for (SIZE_T Cnt=0;Cnt<pBitsFieldInfosLite->Size;Cnt++)
        BitsFieldInfos.MaskAfterShifting += (1<<Cnt);
    // get name
    _tcscpy(BitsFieldInfos.VarName, pBitsFieldInfosLite->VarName);

    // /!\ this->BitsFieldsUsedSize is only incremented for struct
    // not for union
    if (!this->bParentIsUnion)
    {
        // increase used size
        this->BitsFieldsUsedSize += pBitsFieldInfosLite->Size;
    }

    // add bits field to list
    this->pBitsFieldInfosList->AddItem(&BitsFieldInfos);

    // force return to TRUE to avoid misalignment
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: MakeLogParamFromBuffer
// Object: fill a LogParam struct (winapioverride struct used for parameter parsing) from a buffer)
//          WARNING : do not call for top level (top level : NbPointedTimesFromRoot ==0 )
// Parameters :
//      CUserDataTypeVar* pUserDataTypeVar : user data type var from which make buffer ( not always current object )
//      PBYTE Buffer : current buffer from which extract data
//      SIZE_T BufferSize : current size of buffer containing data
//      OUT PARAMETER_LOG_INFOS* pLogInfos : filled struct
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDataTypeVar::MakeLogParamFromBuffer(CUserDataTypeVar* pUserDataTypeVar , PBYTE Buffer, SIZE_T BufferSize,OUT PARAMETER_LOG_INFOS* pLogInfos)
{
    // default output parameter
    memset(pLogInfos,0,sizeof(PARAMETER_LOG_INFOS));
    // copy var name in output parameter
    _tcscpy(pLogInfos->pszParameterName,pUserDataTypeVar->VarName);

    // get parameter informations from CSupportedParameters static table
    SUPPORTED_PARAMETERS_STRUCT* pTypeInfos = CSupportedParameters::GetParamInfos(pUserDataTypeVar->pUserDataType->BaseType);

    // if pointer value
    if (pTypeInfos->DataSize==0) 
    {

        // NOT FOR TOP LEVEL (NbPointedTimesFromRoot !=0 ) 
        // data are lost, we can only access pointer value (not data) 
        // --> act as a pointer to display only pointer value
        pLogInfos->dwType = PARAM_POINTER;
        if ( BufferSize!=GetRegisterSize() )
            return FALSE;

        // copy buffer informations to struct
        pLogInfos->Value = 0;
        memcpy(&pLogInfos->Value,Buffer,__min(BufferSize,sizeof(PBYTE))); // works for 32 or 64 bits due to little endian representation
        pLogInfos->dwSizeOfData = sizeof(PBYTE);

        return TRUE;
    }


    /////////////////////////////
    // value isn't passed as ref
    /////////////////////////////

    // copy var type
    pLogInfos->dwType=pUserDataType->BaseType;

    // copy data size
    pLogInfos->dwSizeOfData = BufferSize;

    // according to data size, store value in OutputStruct.Value or OutputStruct.pValue
    if (BufferSize <= sizeof(PBYTE) )
    {
        if (BufferSize < sizeof(PBYTE) )
        {
            switch (BufferSize)
            {
            default:
            case 1:
                {
                    BYTE b;
                    b=*( (BYTE*)Buffer );
                    pLogInfos->Value = (PBYTE)b;
                }
                break;
            case 2:
                {
                    WORD w;
                    w=*( (WORD*)Buffer );
                    pLogInfos->Value = (PBYTE)w;
                }
                break;
            case 4:
                {
                    DWORD dw;
                    dw=*( (DWORD*)Buffer );
                    pLogInfos->Value = (PBYTE)dw;
                }
                break;
            }
        }
        else
            pLogInfos->Value = *( (PBYTE*)Buffer );

        // apply mask and shifting (mask and shifting can be applied only in this case)
        if (pUserDataTypeVar->pBitsFieldInfosList)
        {
            BITS_FIELD_INFOS* pBitsFieldInfos;
            CLinkListItem* pItem;
            for (pItem = pUserDataTypeVar->pBitsFieldInfosList->Head; pItem ;pItem = pItem->NextItem )
            {
                pBitsFieldInfos = (BITS_FIELD_INFOS*)pItem->ItemData;

                // apply shift
                pBitsFieldInfos->Value = (((ULONG_PTR)pLogInfos->Value) >> pBitsFieldInfos->Shift);
                // apply mask
                pBitsFieldInfos->Value = (((ULONG_PTR)pBitsFieldInfos->Value) & pBitsFieldInfos->MaskAfterShifting);
            }
        }
    }
    else
        pLogInfos->pbValue = Buffer;

    // do a size check between provided buffer size and parameter required size
    if (BufferSize<pUserDataTypeVar->pUserDataType->GetSize())
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: MakeBufferFromLogParam
// Object: get data and data size from a PARAMETER_LOG_INFOS struct
// Parameters :
//          const IN PARAMETER_LOG_INFOS* pLogInfos : struct containing data informations
//          OUT PBYTE* pBuffer : pointer on data pointer
//          OUT SIZE_T* pBufferSize : pointer on size of data
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDataTypeVar::MakeBufferFromLogParam(const IN PARAMETER_LOG_INFOS* pLogInfos, OUT PBYTE* pBuffer,OUT SIZE_T* pBufferSize)
{
    BOOL bSuccess=TRUE;

    // if pointer
    if (pLogInfos->dwSizeOfPointedValue)
    {
        *pBufferSize = pLogInfos->dwSizeOfPointedValue;
        *pBuffer = pLogInfos->pbValue;
        if (pLogInfos->pbValue == 0)
            bSuccess = FALSE;
    }
    else
    {
        *pBufferSize = pLogInfos->dwSizeOfData;
        if ( pLogInfos->dwSizeOfData > GetRegisterSize() )
        {
            *pBuffer = pLogInfos->pbValue;
            if (pLogInfos->pbValue == 0)
                bSuccess = FALSE;
        }
        else
            *pBuffer = (PBYTE)&pLogInfos->Value;
    }
    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: ToString
// Object: get string representation for a PARAMETER_LOG_INFOS struct
// Parameters :
//          PARAMETER_LOG_INFOS* const pLogInfos : log infos from which getting string representation
//          TCHAR** pString : pointer to string (string will be allocated internally, use delete to free memory)
//          const SIZE_T NbRequieredChars : number of wanted chars (to stop string representation); 0 to get full representation
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDataTypeVar::ToString(PARAMETER_LOG_INFOS* const pLogInfos,TCHAR** pString,const SIZE_T NbRequieredChars)
{
    return this->ToString(pLogInfos,pString,NbRequieredChars,0);
}

//-----------------------------------------------------------------------------
// Name: ToString
// Object: get string representation for a PARAMETER_LOG_INFOS struct
// Parameters :
//          PARAMETER_LOG_INFOS* const pLogInfos : log infos from which getting string representation
//          TCHAR** pString : pointer to string (string will be allocated internally, use delete to free memory)
//          const SIZE_T NbRequieredChars : number of wanted chars (to stop string representation); 0 to get full representation
//          const SIZE_T NbPointedTimesFromRoot : number of pointed times from logged object
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDataTypeVar::ToString(PARAMETER_LOG_INFOS* const pLogInfos,TCHAR** pString,const SIZE_T NbRequieredChars,const SIZE_T NbPointedTimesFromRoot)
{
    SIZE_T Cnt;
    PBYTE pSubTypeBuffer;
    SIZE_T SubTypeBufferSize;
    PARAMETER_LOG_INFOS LogInfos;
    TCHAR* LocalString;
    SIZE_T StringMaxSize;
    PBYTE pTypeBuffer;
    SIZE_T TypeBufferSize;

    BOOL bSuccess;
    bSuccess=TRUE;

    // if we can only access pointers, not data value
    if ( (this->NbPointedTimes>1) 
            || (NbPointedTimesFromRoot>0) )
    {
        // display only pointer value
        memcpy(&LogInfos,pLogInfos,sizeof(PARAMETER_LOG_INFOS));
        LogInfos.dwType = PARAM_POINTER;
        LogInfos.dwSizeOfData=CSupportedParameters::GetParamStackSize(PARAM_POINTER);
        LogInfos.dwSizeOfPointedValue=CSupportedParameters::GetParamPointedSize(PARAM_POINTER);
        CSupportedParameters::ParameterToString(_T(""),&LogInfos,pString,NbRequieredChars,FALSE);// FALSE : basic type without define infos -> module name is useless
        return TRUE;
    }

    if ( this->pUserDataType->IsBaseType() )
    {
        SUPPORTED_PARAMETERS_STRUCT* pTypeInfos = CSupportedParameters::GetParamInfos(this->pUserDataType->BaseType);
        if ( 
                ( ( pTypeInfos->DataSize == 0) && (this->NbPointedTimes == 1) )// if pointer and pointed one time
                || ( this->NbPointedTimes == 0) // or if not pointed (only value)
                && (! this->pBitsFieldInfosList) && (!this->pUserDataType->IsEnum()) // not a bit field, and not an enum
                && ( this->NbItems == 1)// single item only
            )
        {
            // call original function
            memcpy(&LogInfos,pLogInfos,sizeof(PARAMETER_LOG_INFOS));
            LogInfos.dwType = this->pUserDataType->BaseType;
            CSupportedParameters::ParameterToString(_T(""),&LogInfos,pString,NbRequieredChars,FALSE);// FALSE : basic type without define infos -> module name is useless
            return TRUE;
        }
    }

    // allocate string and initialize it to "VarName:"
    StringMaxSize = 2048;
    LocalString = new TCHAR[StringMaxSize];
    _tcscpy(LocalString,this->VarName);
    _tcscat(LocalString,_T(":"));


    // if item is a pointer
    if ( (this->NbPointedTimes>0) || pLogInfos->dwSizeOfPointedValue>0)
    {
        // add "0xAddress:" to string
        TCHAR PointerValue[128];
        _stprintf(PointerValue,_T("0x%p:"),pLogInfos->Value);
        CSecureTcscat::Secure_tcscat(&LocalString,PointerValue,&StringMaxSize);
        // check if we get pointed data
        if ( (pLogInfos->dwSizeOfPointedValue == 0) || (pLogInfos->pbValue == 0) )
        {
            CSecureTcscat::Secure_tcscat(&LocalString,_T("Bad Pointer"),&StringMaxSize);
            bSuccess = FALSE;
        }
    }

    // get buffer from logged informations structure (pLogInfos)
    bSuccess = this->MakeBufferFromLogParam(pLogInfos,&pTypeBuffer,&TypeBufferSize);
    if ( TypeBufferSize < this->GetSize() )
    {
        CSecureTcscat::Secure_tcscat(&LocalString,_T("Bad Pointer"),&StringMaxSize);
        bSuccess = FALSE;
    }

    pSubTypeBuffer = pTypeBuffer;

    // if enum
    if (this->pUserDataType->IsEnum())
    {
        if (bSuccess)
        {
            // get enum names for each item
            TCHAR EnumName[CUserDataType_ENUM_NAME_MAX_SIZE];

            SubTypeBufferSize = this->pUserDataType->GetSize();

            // in case of array add "{" before content
            if (this->NbItems>1)
                CSecureTcscat::Secure_tcscat(&LocalString,_T("{"),&StringMaxSize);
            for (
                Cnt=0;
                (Cnt<this->NbItems) && bSuccess && ( (_tcslen(LocalString) < NbRequieredChars ) || (NbRequieredChars == 0) ) ;
                Cnt++
                )
            {
                // check NbRequieredChars
                if (!this->CheckStringEnd(LocalString,NbRequieredChars))
                    break;

                // if not first item add splitter "," to string representation
                if (Cnt)
                    CSecureTcscat::Secure_tcscat(&LocalString,_T(","),&StringMaxSize);

                // translate current buffer position into logparam
                if ( !this->MakeLogParamFromBuffer(this,pSubTypeBuffer,SubTypeBufferSize,&LogInfos))
                {
#ifdef _DEBUG
                    if (::IsDebuggerPresent())
                        ::DebugBreak();
#endif
                    bSuccess=FALSE;
                    break;
                }
                // if string representation of enum can't be found
                if ( !this->pUserDataType->EnumToString(LogInfos.Value,EnumName) )
                {
                    // in case of enum conversion error, display raw value
                    _stprintf(EnumName,_T("0x%p"),LogInfos.Value);
                }

                // add single enum string representation to full string representation
                CSecureTcscat::Secure_tcscat(&LocalString,EnumName,&StringMaxSize);

                // make buffer point to next item
                pSubTypeBuffer+=SubTypeBufferSize;
            }
            // in case of array add "}" after content
            if (this->NbItems>1)
                CSecureTcscat::Secure_tcscat(&LocalString,_T("}"),&StringMaxSize);
        }
    }
    // if base type (fully supported by CSupportedParameters::ParameterToString)
    else if (this->pUserDataType->IsBaseType())
    {
        LogInfos.dwType = this->pUserDataType->BaseType;
        if (bSuccess)
        {
            TCHAR* pszTmp = NULL;

            SubTypeBufferSize = this->pUserDataType->GetSize();

            if ( (this->NbItems>1)
                 && (this->NbPointedTimes == 0)
                 && ( (this->pUserDataType->BaseType == PARAM_CHAR) || (this->pUserDataType->BaseType == PARAM_WCHAR) )
                )
            {

                // default output parameter
                memset(&LogInfos,0,sizeof(PARAMETER_LOG_INFOS));
                // LogInfos.Value = 0; // let value to 0 to show it's not a pointer
                // copy var name in output parameter
                _tcscpy(LogInfos.pszParameterName,this->VarName);
                LogInfos.dwSizeOfPointedValue = SubTypeBufferSize*(this->NbItems+1);// +1 to assume \0 end
                LogInfos.pbValue = new BYTE[LogInfos.dwSizeOfPointedValue];
                memset(&LogInfos.pbValue[SubTypeBufferSize*this->NbItems],0,SubTypeBufferSize);// assume string is ended by 0
                memcpy(LogInfos.pbValue,pSubTypeBuffer,SubTypeBufferSize*this->NbItems);

                switch (this->pUserDataType->BaseType)
                {
                case PARAM_CHAR:
                    LogInfos.dwType = PARAM_PSTR;
                    break;
                case PARAM_WCHAR:
                    LogInfos.dwType = PARAM_PWSTR;
                    break;
                }

                // get string representation of value
                CSupportedParameters::ParameterToString(_T(""),&LogInfos,&pszTmp,NbRequieredChars,FALSE,FALSE);// FALSE : basic type without define infos -> module name is useless
                if (pszTmp)
                {
                    // add single value string representation to full string representation
                    CSecureTcscat::Secure_tcscat(&LocalString,pszTmp,&StringMaxSize);
                    delete pszTmp;
                }

                delete LogInfos.pbValue;
            }
            else
            {

                // in case of array add "{" before content
                if (this->NbItems>1)
                    CSecureTcscat::Secure_tcscat(&LocalString,_T("{"),&StringMaxSize);

                for (
                    Cnt=0;
                    (Cnt<this->NbItems) && bSuccess && ( (_tcslen(LocalString) < NbRequieredChars ) || (NbRequieredChars == 0) ) ;
                    Cnt++
                    )
                {
                    // check NbRequieredChars
                    if (!this->CheckStringEnd(LocalString,NbRequieredChars))
                        break;

                    // if not first item add splitter "," to string representation
                    if (Cnt)
                        CSecureTcscat::Secure_tcscat(&LocalString,_T(","),&StringMaxSize);

                    // translate current buffer position into logparam
                    if (!this->MakeLogParamFromBuffer(this,pSubTypeBuffer,SubTypeBufferSize,&LogInfos))
                    {
#ifdef _DEBUG
                        if (::IsDebuggerPresent())
                            ::DebugBreak();
#endif
                        bSuccess=FALSE;
                        break;
                    }
                    // if bit fields
                    if (this->pBitsFieldInfosList)
                    {
                        BITS_FIELD_INFOS* pBitsFieldInfos;
                        CLinkListItem* pItem;
                        PBYTE OriginalValue;

                        OriginalValue = LogInfos.Value;

                        // add "{" before bits fields content
                        CSecureTcscat::Secure_tcscat(&LocalString,_T("{"),&StringMaxSize);
                        for (pItem = this->pBitsFieldInfosList->Head; pItem ;pItem = pItem->NextItem )
                        {
                            pBitsFieldInfos = (BITS_FIELD_INFOS*)pItem->ItemData;

                            if (pItem != this->pBitsFieldInfosList->Head)
                                CSecureTcscat::Secure_tcscat(&LocalString,_T(","),&StringMaxSize);

                            // apply shift
                            pBitsFieldInfos->Value = (((ULONG_PTR)OriginalValue) >> pBitsFieldInfos->Shift);
                            pBitsFieldInfos->Value = (((ULONG_PTR)pBitsFieldInfos->Value) & pBitsFieldInfos->MaskAfterShifting);

                            // copy name and converted value into the log struct
                            // (this allow to keep existing other parameters)
                            // Warning : Original name and value are lost
                            _tcscpy(LogInfos.pszParameterName, pBitsFieldInfos->VarName);
                            LogInfos.Value = (PBYTE)pBitsFieldInfos->Value;

                            // get string representation of value
                            CSupportedParameters::ParameterToString(_T(""),&LogInfos,&pszTmp,NbRequieredChars,FALSE);// FALSE : basic type without define infos -> module name is useless

                            if (pszTmp)
                            {
                                // add single value string representation to full string representation
                                CSecureTcscat::Secure_tcscat(&LocalString,pszTmp,&StringMaxSize);
                                delete pszTmp;
                            }
                        }
                        // add "}" after bits fields content
                        CSecureTcscat::Secure_tcscat(&LocalString,_T("}"),&StringMaxSize);
                    }
                    else
                    {
                        // get string representation of value
                        CSupportedParameters::ParameterToString(_T(""),&LogInfos,&pszTmp,NbRequieredChars,FALSE,FALSE);// FALSE : basic type without define infos -> module name is useless
                        if (pszTmp)
                        {
                            // add single value string representation to full string representation
                            CSecureTcscat::Secure_tcscat(&LocalString,pszTmp,&StringMaxSize);
                            delete pszTmp;
                        }
                    }

                    // make buffer point to next item
                    pSubTypeBuffer+=SubTypeBufferSize;
                }
                // in case of array add "}" after content
                if (this->NbItems>1)
                    CSecureTcscat::Secure_tcscat(&LocalString,_T("}"),&StringMaxSize);
            }
        }
    }
    else // item is an union / struct item
    {
        CLinkListItem* pItem;
        CUserDataTypeVar* pUserDataTypeVar;
        TCHAR* SubTypeString;

        // in case of array add "{" before content
        if (this->NbItems>1)
            CSecureTcscat::Secure_tcscat(&LocalString, _T("{"), &StringMaxSize);

        // for each item
        for (
            Cnt=0;
            (Cnt<this->NbItems) && bSuccess && ( (_tcslen(LocalString) < NbRequieredChars ) || (NbRequieredChars == 0) );
            Cnt++
            )
        {
            // make buffer point to next item
            pSubTypeBuffer = &pTypeBuffer[Cnt * TypeBufferSize / this->NbItems];
            SubTypeBufferSize = 0;

            // if not first item, add splitter
            if (Cnt>0)
                CSecureTcscat::Secure_tcscat(&LocalString, _T(","), &StringMaxSize);

            // add "{" before struct content
            CSecureTcscat::Secure_tcscat(&LocalString, _T("{"), &StringMaxSize);
            for (pItem=this->pUserDataType->pSubType->Head;pItem;pItem=pItem->NextItem)
            {
                pUserDataTypeVar = (CUserDataTypeVar*)pItem->ItemData;

                // check NbRequieredChars
                if (!this->CheckStringEnd(LocalString,NbRequieredChars))
                    break;

                // if current type isn't a union
                if (!pUserDataTypeVar->bParentIsUnion)
                    // shift buffer with previous subtype size
                    pSubTypeBuffer+= SubTypeBufferSize;

                // get current subtype shift
                SubTypeBufferSize = pUserDataTypeVar->GetSize();

                // translate current buffer position into logparam
                if (!this->MakeLogParamFromBuffer(pUserDataTypeVar,pSubTypeBuffer,SubTypeBufferSize,&LogInfos))
                {
#ifdef _DEBUG
                    if (::IsDebuggerPresent())
                        ::DebugBreak();
#endif
                    bSuccess=FALSE;
                    break;
                }

                // get type content
                if (!pUserDataTypeVar->ToString(&LogInfos,&SubTypeString,NbRequieredChars ? (NbRequieredChars - _tcslen(LocalString)) : 0, NbPointedTimesFromRoot + pUserDataTypeVar->NbPointedTimes))
                {
                    bSuccess=FALSE;
                    break;
                }

                // if not first item, add splitter
                if (pItem!=this->pUserDataType->pSubType->Head)
                    CSecureTcscat::Secure_tcscat(&LocalString, _T(","), &StringMaxSize);

                // add "subtype_representation" to LocalString
                CSecureTcscat::Secure_tcscat(&LocalString, SubTypeString, &StringMaxSize);

                delete SubTypeString;
            } // end for each subtype

            // add "}" after struct content
            CSecureTcscat::Secure_tcscat(&LocalString, _T("}"), &StringMaxSize);

        } // end for each item

        // in case of array add "}" after content
        if (this->NbItems>1)
            CSecureTcscat::Secure_tcscat(&LocalString, _T("}"), &StringMaxSize);
    }
    *pString = LocalString;
    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: get string representation for a PARAMETER_LOG_INFOS struct
// Parameters :
//          TCHAR* UserDataTypePath : path containing struct definitions
//          TCHAR* ModuleName : module name (used to check subfolder existence)
//          TCHAR* TypeName : name of the type ("RECT", "POINT", ...)
//          TCHAR* VarName : name of the var ("MyRect", "MyPoint", ...
//          SIZE_T NbItems : number of items (for array)
//          SIZE_T NbPointedTimes : nb pointed times (ex for RECT*, use "RECT" as type name and 1 for NbPointedTimes)
//          CUserDataType::pfReportError ReportError : report error function
//          PBYTE ReportErrorUserParam : report error function user param
//          OUT DEFINITION_FOUND* pDefinitionFound : found/not found informations
// Return : TRUE on success
//-----------------------------------------------------------------------------
CUserDataTypeVar* CUserDataTypeVar::Parse(TCHAR* UserDataTypePath, TCHAR* ModuleName, TCHAR* TypeName, TCHAR* VarName, SIZE_T NbItems, SIZE_T NbPointedTimes,CUserDataType::pfReportError ReportError,PBYTE ReportErrorUserParam,CUserDataType::DEFINITION_FOUND* pDefinitionFound)
{
    CUserDataTypeVar* pUserDataTypeVar;
    CLinkListSimple* pParentUserDataTypeList;
    pParentUserDataTypeList = new CLinkListSimple();
    pUserDataTypeVar = CUserDataTypeVar::Parse(UserDataTypePath, ModuleName, TypeName, VarName, NbItems, NbPointedTimes,pParentUserDataTypeList, ReportError,ReportErrorUserParam,pDefinitionFound);
    delete pParentUserDataTypeList;
    return pUserDataTypeVar;
}

CUserDataTypeVar* CUserDataTypeVar::Parse(TCHAR* UserDataTypePath, TCHAR* ModuleName, TCHAR* TypeName, TCHAR* VarName, SIZE_T NbItems, SIZE_T NbPointedTimes,CLinkListSimple* pParentUserDataTypeList,CUserDataType::pfReportError ReportError,PBYTE ReportErrorUserParam,CUserDataType::DEFINITION_FOUND* pDefinitionFound)
{
    CUserDataTypeVar* pUserDataTypeVar = new CUserDataTypeVar();

    pUserDataTypeVar->NbItems = NbItems;
    pUserDataTypeVar->NbPointedTimes = NbPointedTimes;
    _tcsncpy(pUserDataTypeVar->VarName,VarName,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE);
    pUserDataTypeVar->VarName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;
    // parse user data type
    pUserDataTypeVar->pUserDataType = CUserDataType::Parse(UserDataTypePath,ModuleName,TypeName,&pUserDataTypeVar->NbPointedTimes,pParentUserDataTypeList,ReportError,ReportErrorUserParam,pDefinitionFound);

    return pUserDataTypeVar;
}