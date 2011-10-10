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
#include "parameterinfo.h"

namespace NET
{

CParameterInfo::CParameterInfo(void)
{
    this->bPointedParameter=FALSE;
    this->bIsOutParameter=FALSE;
    this->PointedSize=0;
    this->StackSize=sizeof(PBYTE);
    this->WinAPIOverrideType=PARAM_UNKNOWN;
    *this->szName=0;
}

CParameterInfo::~CParameterInfo(void)
{
}

BOOL CParameterInfo::GetName(OUT TCHAR* pszName,DWORD NameMaxLen)
{
    // if enough buffer space
    if (_tcslen(this->szName)<NameMaxLen)
    {
        _tcscpy(pszName,this->szName);
    }
    else
    {
        // search for last '.'
        TCHAR* psz=_tcsrchr(this->szName,_T('.'));

        // if found
        if (psz)
            // point after '.'
            psz++;
        // if not found
        else
            // we will keep only first chars of original name
            psz=this->szName;

        // copy data
        _tcsncpy(pszName,psz,NameMaxLen-1);
        pszName[NameMaxLen-1]=0;

    }
    return TRUE;
}

BOOL CParameterInfo::IsOutParameter()
{
    // I found no informations specifying if parameter is an out one or not.
    // So I consider that each pointed type could be an out one
    // return (this->bIsOutParameter!=0);
    return (this->bPointedParameter!=0);
}

//-----------------------------------------------------------------------------
// Name: GetStackSize
// Object: get parameter required stack size
// Parameters :
//     in  : 
//     out : 
//     return : required stack size
//-----------------------------------------------------------------------------
DWORD CParameterInfo::GetStackSize()
{
    return this->StackSize;
}
//-----------------------------------------------------------------------------
// Name: GetPointedSize
// Object: get parameter pointed size
// Parameters :
//     in  : 
//     out : 
//     return : pointed size
//-----------------------------------------------------------------------------
DWORD CParameterInfo::GetPointedSize()
{
    return this->PointedSize;
}
//-----------------------------------------------------------------------------
// Name: GetWinAPIOverrideType
// Object: get WinAPIOverride type
// Parameters :
//     in  : 
//     out : 
//     return : parsing result
//-----------------------------------------------------------------------------
DWORD CParameterInfo::GetWinAPIOverrideType()
{
    return this->WinAPIOverrideType;
}

// see "MetaData Unmanaged API.doc" file available inside 
// Program Files\Microsoft Visual Studio .NET 2003\SDK\v1.1\Tool Developers Guide\docs\ 
// for meta data informations
CParameterInfo* CParameterInfo::Parse(IN IMetaDataImport *pMetaDataImport,IN PCOR_SIGNATURE pSigParam,OUT PCOR_SIGNATURE* pNextSig)
{	
    CParameterInfo* pParameterInfo=NULL;
    COR_SIGNATURE SigContent;

    *pNextSig=0;

    pParameterInfo=new CParameterInfo();
    if (!pParameterInfo)
        return NULL;

    //SigParam:
    //1) 0,N ELEMENT_TYPE_CMOD_REQD/ELEMENT_TYPE_CMOD_OPT
    //2) a) 0,1 TYPE_BYREF + 1 Type
    //   or
    //   b) 1 TYPE_TYPEDBYREF

    SigContent=*pSigParam;
    pSigParam++;

    switch (SigContent) 
    {	
    // 1) 0,N ELEMENT_TYPE_CMOD_REQD/ELEMENT_TYPE_CMOD_OPT
    case ELEMENT_TYPE_CMOD_REQD:
    case ELEMENT_TYPE_CMOD_OPT:
        {	
            // in these 2 case we don't get
            mdToken	Token;	
            TCHAR szClassName[MAX_LENGTH];

            *szClassName=0;
            pSigParam += CorSigUncompressToken(pSigParam,&Token); 
            if (TypeFromToken(Token)!=mdtTypeRef)
            {
                HRESULT	hr;
                WCHAR szName[MAX_LENGTH];
                DWORD szNameSize;
                *szName=0;
                hr = pMetaDataImport->GetTypeDefProps(Token, 
                                                szName,
                                                MAX_LENGTH,
                                                &szNameSize,
                                                NULL,
                                                NULL);
                szName[MAX_LENGTH-1]=0;
                if (FAILED(hr)
                    || (szName==0)
                    )
                {
                    wsprintfW(szName,L"ParamToken%u",Token);
                }

#if (defined(UNICODE)||defined(_UNICODE))
                _tcsncpy(szClassName,szName,MAX_LENGTH);
#else
                wcstombs(szClassName,szName,MAX_LENGTH);
#endif
                szClassName[MAX_LENGTH-1]=0;
            }

            _tcscpy(pParameterInfo->szName,szClassName);
            // get type
            pParameterInfo->WinAPIOverrideType=CSupportedParameters::GetParamType(pParameterInfo->szName);
            // get sizes
            pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
            pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        }
        break;
    // 2) b) 1 TYPE_TYPEDBYREF
    case ELEMENT_TYPE_TYPEDBYREF:
        //A TYPEDBYREF is a simple structure of two DWORDs – one indicates the type of the parameter, the other, its value.
        //This struct is pushed on the stack by the caller.  
        //So, only at runtime, is the type of the parameter actually provided.  
        //TYPEDBYREF was originally introduced to support VB’s "refany" argument-passing technique
        _tcscpy(pParameterInfo->szName,_T("ULONG64"));
        // get type
        pParameterInfo->WinAPIOverrideType=PARAM_INT64;
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;	
    // 2) a) 0,1 TYPE_BYREF + 1 Type
    case ELEMENT_TYPE_BYREF:   
        // delete current info
        delete pParameterInfo;

        // get type infos
        pParameterInfo=CParameterInfo::Parse(pMetaDataImport,pSigParam,&pSigParam); 
        if (!pParameterInfo)
            return NULL;
        _tcscat(pParameterInfo->szName,_T("*"));

        pParameterInfo->bPointedParameter=TRUE;

        // get type
        if (pParameterInfo->WinAPIOverrideType & EXTENDED_TYPE_FLAG_MASK)
            pParameterInfo->WinAPIOverrideType=PARAM_POINTER;
        else
            pParameterInfo->WinAPIOverrideType=CSupportedParameters::GetParamType(pParameterInfo->szName);

        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;

    //////////////////////////
    // from now only Type
    //////////////////////////

    //Type :=	VOID (for return type)
    //          | Intrinsic (BOOLEAN | CHAR | I1 | U1 | I2 | U2 | I4 | U4 | I8 | U8 | R4 | R8 | I  | U)
    //          | VALUETYPE			TypeDefOrRefEncoded
    //          | CLASS         	TypeDefOrRefEncoded
    //          | STRING
    //          | OBJECT
    //          | PTR				CustomMod*  VOID
    //          | PTR				CustomMod*  Type
    //          | FNPTR				MethodDefSig
    //          | FNPTR				MethodRefSig
    //          | ARRAY     		Type		ArrayShape
    //          | SZARRAY 			CustomMod*  Type

    case ELEMENT_TYPE_VOID:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_VOID;
        _tcscpy(pParameterInfo->szName,_T("VOID"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;					
    case ELEMENT_TYPE_BOOLEAN:	
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_BOOL;
        _tcscpy(pParameterInfo->szName,_T("BOOL"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;	
    case ELEMENT_TYPE_CHAR:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_WCHAR;
        _tcscpy(pParameterInfo->szName,_T("WCHAR"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_I1:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_CHAR;
        _tcscpy(pParameterInfo->szName,_T("SBYTE"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_U1:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_BYTE;
        _tcscpy(pParameterInfo->szName,_T("BYTE"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_I2:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_SHORT;
        _tcscpy(pParameterInfo->szName,_T("SHORT"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_U2:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_USHORT;
        _tcscpy(pParameterInfo->szName,_T("USHORT"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;			
    case ELEMENT_TYPE_I4:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_INT;
        _tcscpy(pParameterInfo->szName,_T("INT"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;
    case ELEMENT_TYPE_U4:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_UINT;
        _tcscpy(pParameterInfo->szName,_T("UINT"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_I8:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_INT64;
        _tcscpy(pParameterInfo->szName,_T("INT64"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_U8:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_INT64;
        _tcscpy(pParameterInfo->szName,_T("UINT64"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_R4:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_FLOAT;
        _tcscpy(pParameterInfo->szName,_T("float"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;			
    case ELEMENT_TYPE_R8:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_DOUBLE;
        _tcscpy(pParameterInfo->szName,_T("double"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		
    case ELEMENT_TYPE_U:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_UINT;
        _tcscpy(pParameterInfo->szName,_T("UINT"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		 
    case ELEMENT_TYPE_I:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_INT;
        _tcscpy(pParameterInfo->szName,_T("INT"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;			  

    case ELEMENT_TYPE_OBJECT:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_POINTER;
        _tcscpy(pParameterInfo->szName,_T("PVOID"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		 
    case ELEMENT_TYPE_STRING:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_NET_STRING;
        _tcscpy(pParameterInfo->szName,_T("string"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;		 

    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_CLASS:
        {
            HRESULT	hResult=E_FAIL;
            mdToken	Token;	
            TCHAR szClassName[MAX_LENGTH];
            WCHAR szName[MAX_LENGTH];
            DWORD szNameSize;
            *szClassName=0;
            *szName=0;
            pSigParam+=CorSigUncompressToken(pSigParam,&Token); 
            if ( TypeFromToken( Token ) == mdtTypeDef )
            {
                hResult = pMetaDataImport->GetTypeDefProps( Token, szName, MAX_LENGTH, &szNameSize, NULL, NULL );
            }
            else if ( TypeFromToken( Token ) == mdtTypeRef )
            {
                hResult = pMetaDataImport->GetTypeRefProps(Token, NULL,szName, MAX_LENGTH, &szNameSize);
            }

            szName[MAX_LENGTH-1]=0;
            if (FAILED(hResult)
                || (szName==0)
                || (*szName<32) || (*szName>126) // assume readable name
                )
            {
                wsprintfW(szName,L"ParamToken%u",Token);
            }
#if (defined(UNICODE)||defined(_UNICODE))
            _tcsncpy( szClassName,szName, MAX_LENGTH);
#else
            wcstombs( szClassName, szName, MAX_LENGTH );
#endif
            szClassName[MAX_LENGTH-1]=0;

            _tcscpy(pParameterInfo->szName,szClassName);

            // set default type
            pParameterInfo->WinAPIOverrideType=PARAM_POINTER;
            // get sizes
            pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
            pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
            break;
        }
    case ELEMENT_TYPE_FNPTR:
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_POINTER;
        _tcscpy(pParameterInfo->szName,_T("FunctionPointer"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;
    case ELEMENT_TYPE_SZARRAY:
        // The SZARRAY production describes a frequently-used, special-case of ARRAY 
        // that's to say, a single-dimension (rank 1) array, with a zero lower bound, and no specified size
        // delete current info
        delete pParameterInfo;

        // get type infos
        pParameterInfo=CParameterInfo::Parse(pMetaDataImport,pSigParam,&pSigParam); 
        if (!pParameterInfo)
            return NULL;
        _tcscat(pParameterInfo->szName,_T("*"));
        pParameterInfo->bPointedParameter=TRUE;

        // get type
        pParameterInfo->WinAPIOverrideType=CSupportedParameters::GetParamType(pParameterInfo->szName);
        pParameterInfo->WinAPIOverrideType|=EXTENDED_TYPE_FLAG_NET_SINGLE_DIM_ARRAY;
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);

        break;		
    case ELEMENT_TYPE_ARRAY:	
        {	
            //1) rank
            //2) num size
            //3) 0-N Sizes
            //4) nb low bounds
            //5) 0-N low bounds
            //
            //ex
            //                Type	Rank	NumSizes	    Size*	NumLoBounds	LoBound*
            //[0..2]	        I4	    1	        1	    3	        0	
            //[,,,,,,]	        I4	    6	        0			
            //[0..3, 0..2,,,,]  I4	    6	        2	    4,3	        0	
            //[1..2, 6..8]	    I4	    2	        2	    2,3	        2	    1,6
            //[5, 3..5, , ]	    I4	    3	        2	    5,3	        2	    0,3
            ULONG rank;

            // delete current info
            delete pParameterInfo;
            // parse element
            pParameterInfo=CParameterInfo::Parse(pMetaDataImport,pSigParam,&pSigParam);
            if (!pParameterInfo)
                return NULL;
            _tcscat(pParameterInfo->szName,_T("*"));
            pParameterInfo->bPointedParameter=TRUE;

            // get type
            pParameterInfo->WinAPIOverrideType=CSupportedParameters::GetParamType(pParameterInfo->szName);
            pParameterInfo->WinAPIOverrideType|=EXTENDED_TYPE_FLAG_NET_MULTIPLE_DIM_ARRAY;
            pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
            pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);

            // we have to do parsing even if not necessary to find next signature
            // 1) rank
            rank = CorSigUncompressData((PCCOR_SIGNATURE&)pSigParam);
            
            if ( rank > 0 ) 
            {
                ULONG *lower;	
                ULONG *sizes; 	
                ULONG numsizes; 
                ULONG arraysize=(sizeof(ULONG)*2*rank);

                lower=(ULONG*)_alloca(arraysize);                                                        
                memset(lower,0,arraysize); 
                sizes=&lower[rank];

                // 2) num size
                numsizes = CorSigUncompressData((PCCOR_SIGNATURE&)pSigParam);	
                if ( numsizes <= rank )
                {
                    ULONG numlower;
                    ULONG i;

                    // 3) sizes
                    for ( i = 0; i < numsizes; i++ )	
                        sizes[i] = CorSigUncompressData((PCCOR_SIGNATURE&)pSigParam);	


                    // 4) num lower
                    numlower = CorSigUncompressData((PCCOR_SIGNATURE&)pSigParam);	
                    if ( numlower <= rank )
                    {
                        // 5) low bounds
                        for ( i = 0; i < numlower; i++)	
                            lower[i] = CorSigUncompressData((PCCOR_SIGNATURE&)pSigParam); 

                        //// to display array size only
                        //_tcscat( pParameterInfo->szName, "[" );	
                        //for ( i = 0; i < rank; i++ )	
                        //{	
                        //    if ( (sizes[i] != 0) && (lower[i] != 0) )	
                        //    {	
                        //        if ( lower[i] == 0 )	
                        //            _stprintf ( pParameterInfo->szName, _T("%d"), sizes[i] );	

                        //        else	
                        //        {	
                        //            _stprintf( pParameterInfo->szName, _T("%d"), lower[i] );	
                        //            _tcscat( pParameterInfo->szName, _T("...") );	

                        //            if ( sizes[i] != 0 )	
                        //                _stprintf( pParameterInfo->szName, _T("%d"), (lower[i] + sizes[i] + 1) );	
                        //        }	
                        //    }

                        //    if ( i < (rank - 1) ) 
                        //        _tcscat( pParameterInfo->szName, _T(",") );	
                        //}	

                        //_tcscat( pParameterInfo->szName, _T("]") );  

                    }						
                }
            }

        } 
        break;	


    case ELEMENT_TYPE_PINNED:
        // delete current info
        delete pParameterInfo;
        // parse element
        pParameterInfo=CParameterInfo::Parse(pMetaDataImport,pSigParam,&pSigParam); 
        if (!pParameterInfo)
            return NULL;
        // _tcscat(pParameterInfo->szName,_T("pinned"));	
        break;	


    case ELEMENT_TYPE_PTR:  
        // delete current info
        delete pParameterInfo;

        // get type infos
        pParameterInfo=CParameterInfo::Parse(pMetaDataImport,pSigParam,&pSigParam); 
        if (!pParameterInfo)
            return NULL;
        _tcscat(pParameterInfo->szName,_T("*"));

        pParameterInfo->bPointedParameter=TRUE;

        // get type
        if (pParameterInfo->WinAPIOverrideType & EXTENDED_TYPE_FLAG_MASK)
            pParameterInfo->WinAPIOverrideType=PARAM_POINTER;
        else
            pParameterInfo->WinAPIOverrideType=CSupportedParameters::GetParamType(pParameterInfo->szName);

        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);

        break;   


    default:	
    case ELEMENT_TYPE_END:	
    case ELEMENT_TYPE_SENTINEL:	
        // set default type
        pParameterInfo->WinAPIOverrideType=PARAM_UNKNOWN;
        _tcscpy(pParameterInfo->szName,_T("Unknown"));
        // get sizes
        pParameterInfo->StackSize=CSupportedParameters::GetParamStackSize(pParameterInfo->WinAPIOverrideType);
        pParameterInfo->PointedSize=CSupportedParameters::GetParamPointedSize(pParameterInfo->WinAPIOverrideType);
        break;				                      				            

    } // switch	

    // fill output parameters
    *pNextSig=pSigParam;

    return pParameterInfo;
}

}