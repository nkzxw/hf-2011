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
// Object: generate COM auto monitoring files from type library files (.tlb)
//-----------------------------------------------------------------------------
#include "COMMonitoringFileGenerator.h"

//-----------------------------------------------------------------------------
// Name: CCOMMonitoringFileGenerator
// Object: Constructor.
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CCOMMonitoringFileGenerator::CCOMMonitoringFileGenerator()
{
    this->PercentCompletedCallBack=NULL;
    this->UserMessageInformationCallBack=NULL;
    this->UserMessageInformationCallBackUserParam=NULL;
    this->PercentCompletedCallBackUserParam=NULL;
    *this->CurrentTypeLibrary=NULL;
    this->bOperationCanceled=FALSE;
}

//-----------------------------------------------------------------------------
// Name: CCOMMonitoringFileGenerator
// Object: Constructor with callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CCOMMonitoringFileGenerator::CCOMMonitoringFileGenerator(tagProgressCallBack PercentCompletedCallBack,LPVOID PercentCompletedCallBackUserParam,tagUserMessageInformationCallBack UserMessageInformationCallBack,LPVOID UserMessageInformationCallBackUserParam)
{
    this->PercentCompletedCallBack=PercentCompletedCallBack;
    this->UserMessageInformationCallBack=UserMessageInformationCallBack;
    this->UserMessageInformationCallBackUserParam=UserMessageInformationCallBackUserParam;
    this->PercentCompletedCallBackUserParam=PercentCompletedCallBackUserParam;
    *this->CurrentTypeLibrary=NULL;
    this->bOperationCanceled=FALSE;
}

//-----------------------------------------------------------------------------
// Name: CCOMMonitoringFileGenerator
// Object: Destructor
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CCOMMonitoringFileGenerator::~CCOMMonitoringFileGenerator(void)
{

}

//-----------------------------------------------------------------------------
// Name: UserMessage
// Object: Translate user message to callback if any
// Parameters :
//     in  : TCHAR* ErrorMsg
//           tagUserMessagesTypes MessageType
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCOMMonitoringFileGenerator::UserMessage(TCHAR* ErrorMsg,tagUserMessagesTypes MessageType)
{
    if (this->UserMessageInformationCallBack)
    {
        this->UserMessageInformationCallBack(ErrorMsg,MessageType,this->UserMessageInformationCallBackUserParam);
    }
}

//-----------------------------------------------------------------------------
// Name: ReportProgress
// Object: Translate progress report to callback if any
// Parameters :
//     in  : BYTE PercentCompleted
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCOMMonitoringFileGenerator::ReportProgress(BYTE PercentCompleted)
{
    if (this->PercentCompletedCallBack)
    {
        this->PercentCompletedCallBack(PercentCompleted,this->PercentCompletedCallBackUserParam);
    }
}

//-----------------------------------------------------------------------------
// Name: GenerateCOMAllRegisteredTypeLibrariesMonitoring
// Object: Parse registry to find all registered Type libraries, and create 
//          COM auto monitoring files for each type library
// Parameters :
//     in  : TCHAR* OutputDirectory : output directory that will contain generated monitoring files
//           HANDLE hCancelEvent : cancellation event
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCOMMonitoringFileGenerator::GenerateCOMAllRegisteredTypeLibrariesMonitoring(TCHAR* OutputDirectory,HANDLE hCancelEvent)
{

    LONG lResult;
    WCHAR* TypeLibraryPathW;
    TCHAR TypeCLSID[128];
    TCHAR VersionInformation[32];
    DWORD SubKeyIndex;
    DWORD CLSIDSubKeyIndex;
    TCHAR Msg[2*MAX_PATH];
    DWORD NbTypeLibs=0;
    UINT  MajorVersion;
    UINT  MinorVersion;
    CLSID CLSIDTypeLib;
    BOOL  bRet;

    BOOL bResult = TRUE;
    // save percent callback
    tagProgressCallBack LocalPercentCompletedCallBack=this->PercentCompletedCallBack;
    
    // reset progress to 0
    this->ReportProgress(0);

    // open HKCR\TypeLib registry key
    HKEY hKeyAllTypeLibs = NULL;
    lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT,CCOMMonitoringFileGenerator_TYPE_LIB_KEY,0,KEY_READ,&hKeyAllTypeLibs);
    if ( ( lResult != ERROR_SUCCESS) || (hKeyAllTypeLibs==NULL) )
    {
        _stprintf (Msg,_T("Error opening HKEY_CLASSES_ROOT\\%s\r\n"),CCOMMonitoringFileGenerator_TYPE_LIB_KEY);
        this->UserMessage(Msg,USER_MESSAGE_ERROR);
        return FALSE;
    }

    // Get the class name and the value count. 
    RegQueryInfoKey(hKeyAllTypeLibs,// key handle 
                    NULL,      // buffer for class name 
                    NULL,      // size of class string 
                    NULL,      // reserved 
                    &NbTypeLibs,// number of subkeys 
                    NULL,      // longest subkey size 
                    NULL,      // longest class string 
                    NULL,      // number of values for this key 
                    NULL,      // longest value name 
                    NULL,      // longest value data 
                    NULL,      // security descriptor 
                    NULL);     // last write time 
    
    HKEY hKeyTypeLib;
    //Enumerate through all the typelib CLSID entries
    for (SubKeyIndex = 0; 
        RegEnumKey(hKeyAllTypeLibs,SubKeyIndex,TypeCLSID,sizeof(TypeCLSID)) == ERROR_SUCCESS; 
        SubKeyIndex++)
    {
        this->ReportProgress((BYTE)((SubKeyIndex*100)/NbTypeLibs));

        //Open each CLSID Subkey  under the TypeLib key	
        hKeyTypeLib = NULL;
        lResult = RegOpenKeyEx(hKeyAllTypeLibs,TypeCLSID,0, KEY_READ, &hKeyTypeLib ) ;
        if ( ( lResult != ERROR_SUCCESS) || (hKeyAllTypeLibs==NULL) )
        {
            _stprintf (Msg,_T("Error opening HKEY_CLASSES_ROOT\\%s\\%s\r\n"),CCOMMonitoringFileGenerator_TYPE_LIB_KEY,TypeCLSID);
            this->UserMessage(Msg,USER_MESSAGE_ERROR);
            continue;
        }

        //Get each Subkey under the CLSID subkey
        // We should be able to get version information 
        for(CLSIDSubKeyIndex=0;
            RegEnumKey(hKeyTypeLib,CLSIDSubKeyIndex , VersionInformation, sizeof(VersionInformation)) == ERROR_SUCCESS;
            CLSIDSubKeyIndex++
            )
        {
            // check cancel event
            if (WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)    
            {
                this->bOperationCanceled=TRUE;
                RegCloseKey(hKeyTypeLib);
                RegCloseKey(hKeyAllTypeLibs);
                return FALSE;
            }

            // assume version is Major.Minor
            if (_stscanf(VersionInformation,_T("%u.%u"),&MajorVersion,&MinorVersion)!=2)
                continue;

            // get CLSID from it string representation
            if (!CGUIDStringConvert::CLSIDFromTchar(TypeCLSID,&CLSIDTypeLib))
                continue;

            // get Type library path associated to CLSID
            if ( QueryPathOfRegTypeLib(CLSIDTypeLib,(USHORT)MajorVersion,(USHORT)MinorVersion,GetUserDefaultLangID(),&TypeLibraryPathW) != ERROR_SUCCESS )
            {
                _stprintf (Msg,_T("Error getting type library path for CLSID %s\r\n"),TypeCLSID);
                this->UserMessage(Msg,USER_MESSAGE_ERROR);
                continue;
            }

            // remove percent completed callback to avoid GenerateCOMTypeLibraryMonitoring percent changes
            this->PercentCompletedCallBack=NULL;

            // generate info for typelib
            bRet = this->GenerateCOMTypeLibraryMonitoringW(TypeLibraryPathW,OutputDirectory,hCancelEvent);
            bResult = bResult && bRet;

            // restore percent completed callback
            this->PercentCompletedCallBack=LocalPercentCompletedCallBack;

            // add an empty line between 2 TypeLibrary generation
            this->UserMessage(_T("\r\n"),USER_MESSAGE_INFORMATION);

            // free memory
            SysFreeString(TypeLibraryPathW);

            // check operation cancellation
            if (this->bOperationCanceled)
                break;
        }

        // close type lib key
        RegCloseKey(hKeyTypeLib);

        // check operation cancellation
        if (this->bOperationCanceled)
            break;
    }

    // close HKCR\TypeLib key
    RegCloseKey(hKeyAllTypeLibs);

    // update percent completed if operation hasn't been canceled
    if (!this->bOperationCanceled)
        this->ReportProgress(100);

    return bResult;
}


//-----------------------------------------------------------------------------
// Name: GenerateCOMTypeLibraryMonitoring
// Object: Parse registry to find all registered Type libraries, and create 
//          COM auto monitoring files for each type library
// Parameters :
//     in  : TCHAR* TypeLibraryPath : path of Type library
//           TCHAR* OutputDirectory : output directory that will contain generated monitoring files
//           HANDLE hCancelEvent : cancellation event
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCOMMonitoringFileGenerator::GenerateCOMTypeLibraryMonitoring(TCHAR* TypeLibraryPath,TCHAR* OutputDirectory,HANDLE hCancelEvent)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return this->GenerateCOMTypeLibraryMonitoringW(TypeLibraryPath,OutputDirectory,hCancelEvent);
#else
    WCHAR TypeLibraryPathW[MAX_PATH];
    CAnsiUnicodeConvert::AnsiToUnicode(TypeLibraryPath,TypeLibraryPathW,MAX_PATH);
    return this->GenerateCOMTypeLibraryMonitoringW(TypeLibraryPathW,OutputDirectory,hCancelEvent);
#endif
}

//-----------------------------------------------------------------------------
// Name: GenerateCOMTypeLibraryMonitoringW
// Object: Parse registry to find all registered Type libraries, and create 
//          COM auto monitoring files for each type library
// Parameters :
//     in  : WCHAR* TypeLibraryPathW : unicode path of Type library
//           TCHAR* OutputDirectory : output directory that will contain generated monitoring files
//           HANDLE hCancelEvent : cancellation event
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCOMMonitoringFileGenerator::GenerateCOMTypeLibraryMonitoringW(WCHAR* TypeLibraryPathW,TCHAR* OutputDirectory,HANDLE hCancelEvent)
{
    BOOL bResult = TRUE;
    BOOL bFunctionRet; 
    ITypeLib*  pTypeLib=NULL;
    ITypeInfo* pTypeInfo=NULL;
    SIZE_T InheritedInterfaceMaxVTBLIndex = 2;// always inherit from IUnkown
    CInterfaceInfo* pInterfaceInfo;
    TYPEKIND TypeKind;
    TCHAR Msg[4*MAX_PATH];
    HRESULT hResult;
    IID InheritedInterface;
    BSTR InheritedInterfaceName;
    UINT Cnt;
    UINT NbTypeInfo;

    // assume output directory exists
    if (!CStdFileOperations::CreateDirectory(OutputDirectory))
    {
        _stprintf(Msg,_T("Error can't create directory %s\r\n"),OutputDirectory);
        this->UserMessage(Msg,USER_MESSAGE_ERROR);
        return FALSE;
    }

#if (defined(UNICODE)||defined(_UNICODE))
    _tcscpy(this->CurrentTypeLibrary,TypeLibraryPathW);
#else
    CAnsiUnicodeConvert::UnicodeToAnsi(TypeLibraryPathW,this->CurrentTypeLibrary,MAX_PATH);
#endif

    // reset progress to 0
    this->ReportProgress(0);

    // display user information
    _stprintf(Msg,_T("Generating monitoring file for %s\r\n"),this->CurrentTypeLibrary);
    this->UserMessage(Msg,USER_MESSAGE_INFORMATION);

    // load type library
    hResult = ::LoadTypeLibEx( TypeLibraryPathW,REGKIND_NONE,&pTypeLib);
    if (FAILED(hResult) || (pTypeLib==NULL))
    {
        _stprintf(Msg,_T("Error Loading Type Library %s (Error code : 0x%.8X)\r\n"),this->CurrentTypeLibrary,hResult);
        this->UserMessage(Msg,USER_MESSAGE_ERROR);
        bResult = FALSE;
        goto CleanUp;
    }

    // get number of type infos
    NbTypeInfo = pTypeLib->GetTypeInfoCount();

    // for each type info
    for (Cnt=0;Cnt<NbTypeInfo;Cnt++)
    {
        // report progress
        this->ReportProgress((BYTE)((Cnt*100)/NbTypeInfo));

        // check cancel event
        if (WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)    
        {
            this->bOperationCanceled=TRUE;
            bResult = FALSE;
            goto CleanUp;
        }

        // get kind of type
        pTypeLib->GetTypeInfoType(Cnt,&TypeKind);

        // if type is not an interface (remember we generate COM auto monitoring files for interface)
        // TKIND_COCLASS is not required as object makes a query interface on wanted interfaces
        // TKIND_DISPATCH use same IID as associated TKIND_INTERFACE but sometimes only types using TKIND_DISPATCH are returned
        if ( (TypeKind != TKIND_INTERFACE)
             && (TypeKind != TKIND_DISPATCH)
           )
            continue;

        // get type
        pTypeInfo=NULL;
        hResult = pTypeLib->GetTypeInfo(Cnt,&pTypeInfo);
        if (pTypeInfo==NULL)
            continue;
        if (FAILED(hResult))
        {
            pTypeInfo->Release();
            continue;
        }

        // get inherited interface infos (iid and name)
        InheritedInterfaceName = 0;
        InheritedInterface = this->GetInheritedInterface(pTypeInfo,&InheritedInterfaceName, &InheritedInterfaceMaxVTBLIndex);

        // get interface infos
        pInterfaceInfo=new CInterfaceInfo();
        // someone report case that CInterfaceInfo::Parse can crash (coming from pTypeInfo->GetFuncDesc)
        // This can be due to bad install or dll provider internal error
        // 1) As this error belongs to dll we are not owner, we have to do a try catch
        // 2) we do it here instead of CInterfaceInfo::Parse method for performance reasons
        try
        {
            CExceptionHardware::RegisterTry();
            hResult = pInterfaceInfo->Parse(NULL,NULL,pTypeInfo,TRUE,TRUE);
        }
        catch (CExceptionHardware e)
        {
            _stprintf(Msg,_T("%s parsing %s\r\n"),e.ExceptionText,this->CurrentTypeLibrary);
            this->UserMessage(Msg,USER_MESSAGE_ERROR);
            hResult = E_FAIL;
            // reset pTypeInfo to avoid to access it again (we don't know it's state)
            pTypeInfo = NULL;
        }
        
        if (SUCCEEDED(hResult))
        {
            // generate monitoring file for interface
            bFunctionRet = CCOMMonitoringFileGenerator::GenerateCOMMonitoringFromInterface(pInterfaceInfo,&InheritedInterface,&InheritedInterfaceName,InheritedInterfaceMaxVTBLIndex,OutputDirectory);
            bResult = bResult && bFunctionRet;
        }

        // free allocated memory
        if (InheritedInterfaceName)
            SysFreeString(InheritedInterfaceName);

        delete pInterfaceInfo;
        if (pTypeInfo) // see exception
            pTypeInfo->Release();
    }
CleanUp:
    if (pTypeLib)
        pTypeLib->Release();
   
    // report progress if operation not canceled
    if (!this->bOperationCanceled)
        this->ReportProgress(100);

    return bResult;
}

//-----------------------------------------------------------------------------
// Name: GetInheritedInterface
// Object: get inherited interface for an interface
//          COM auto monitoring files for each type library
// Parameters :
//     in  : ITypeInfo* pTypeInfo : interface type info
//     out : BSTR* pInheritedInterFaceName : inherited interface name
//     return : inherited interface ID
//-----------------------------------------------------------------------------
IID CCOMMonitoringFileGenerator::GetInheritedInterface(ITypeInfo* pTypeInfo,OUT BSTR* pInheritedInterFaceName, OUT SIZE_T* pInheritedInterfaceMaxVTBLIndex)
{
    HRESULT hResult;
    ITypeInfo* pInheritedTypeInfo=NULL;
    HREFTYPE hRefType;
    IID Iid = IID_IUnknown;
    *pInheritedInterfaceMaxVTBLIndex = 2;// always inherit from IUnkown

    // get inherited type
    hResult = pTypeInfo->GetRefTypeOfImplType(0,&hRefType);
    if( FAILED(hResult))
        return Iid;

    // get inherited type info
    TYPEATTR* pTypeAttr;
    hResult = pTypeInfo->GetRefTypeInfo(hRefType,&pInheritedTypeInfo);
    if (!pInheritedTypeInfo)
        return Iid;
    if (FAILED(hResult))
    {
        pInheritedTypeInfo->Release();
        return Iid;
    }

    // get interface name
    hResult=pInheritedTypeInfo->GetDocumentation(-1,pInheritedInterFaceName,0,0,0);

    // get inherited interface max vtbl index
    CInterfaceInfo* pInterfaceInfo=new CInterfaceInfo();
    CLinkListItem* pItem;
    CMethodInfo* pMethodInfo;
    // someone report case that CInterfaceInfo::Parse can crash (coming from pTypeInfo->GetFuncDesc)
    // This can be due to bad install or dll provider internal error
    // 1) As this error belongs to dll we are not owner, we have to do a try catch
    // 2) we do it here instead of CInterfaceInfo::Parse method for performance reasons
    try
    {
        CExceptionHardware::RegisterTry();
        hResult = pInterfaceInfo->Parse(NULL,NULL,pInheritedTypeInfo,TRUE,TRUE);

        // free each CMethodInfo object
        for (pItem=pInterfaceInfo->pMethodInfoList->Head;pItem;pItem=pItem->NextItem)
        {
            pMethodInfo=(CMethodInfo*) pItem->ItemData;
            *pInheritedInterfaceMaxVTBLIndex = __max(*pInheritedInterfaceMaxVTBLIndex,(SIZE_T)pMethodInfo->VTBLIndex);
        }
    }
    catch (CExceptionHardware e)
    {

    }
    delete pInterfaceInfo;

    // get type Attr
    hResult = pInheritedTypeInfo->GetTypeAttr(&pTypeAttr);
    if( FAILED(hResult))
    {
        pInheritedTypeInfo->Release();
        return Iid;
    }

    // fill iid field
    Iid = pTypeAttr->guid;

    // free memory
    pInheritedTypeInfo->ReleaseTypeAttr(pTypeAttr);
    pInheritedTypeInfo->Release();

    return Iid;
}

//-----------------------------------------------------------------------------
// Name: GenerateCOMMonitoringFromInterface
// Object: Do COM auto monitoring file generation
// Parameters :
//     in  : CInterfaceInfo* pInterfaceInfo : Interface definition
//           IID* pInheritedInterface : inherited interface id
//           BSTR* pInheritedInterFaceName : inherited interface name
//           SIZE_T InheritedInterFaceMaxVTBLIndex : inherited interface max vtbl index
//           TCHAR* OutputDirectory : directory where monitoring file will be generated
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCOMMonitoringFileGenerator::GenerateCOMMonitoringFromInterface(CInterfaceInfo* pInterfaceInfo,IID* pInheritedInterface,BSTR* pInheritedInterFaceName,SIZE_T InheritedInterFaceMaxVTBLIndex,TCHAR* OutputDirectory)
{
    HANDLE hFile=INVALID_HANDLE_VALUE;

    TCHAR pszMsg[2*MAX_PATH];
    TCHAR pszMonitoringFileName[MAX_PATH];
    TCHAR pszIID[MAX_PATH];
    CGUIDStringConvert::TcharFromIID(&pInterfaceInfo->Iid,pszIID);
    TCHAR InterfaceName[MAX_PATH];
    BOOL InterfaceNameRetrievalSuccess;

    if (!OutputDirectory)
        return FALSE;

    // if interface name retrieval was successful
    if (pInterfaceInfo->Name)
    {
        InterfaceNameRetrievalSuccess=TRUE;

#if (defined(UNICODE)||defined(_UNICODE))
        _tcsncpy(InterfaceName,pInterfaceInfo->Name,MAX_PATH-1);
        InterfaceName[MAX_PATH-1]=0;
#else
        CAnsiUnicodeConvert::UnicodeToAnsi(pInterfaceInfo->Name,InterfaceName,MAX_PATH);
#endif
    }
    else
    {
        InterfaceNameRetrievalSuccess=FALSE;
        // try to get interface name from another way
        if (CGUIDStringConvert::GetInterfaceName(pszIID,InterfaceName,MAX_PATH))
            InterfaceNameRetrievalSuccess=TRUE;
    }

    // forge output file path
    _tcscpy(pszMonitoringFileName,OutputDirectory);
    _tcscat(pszMonitoringFileName,_T("\\"));
    _tcscat(pszMonitoringFileName,pszIID);
    _tcscat(pszMonitoringFileName,_T(".txt"));

    // report generation to user
    if (InterfaceNameRetrievalSuccess)
        _stprintf(pszMsg,_T("Generating monitoring file for Interface %s\r\n"),InterfaceName);
    else
        _stprintf(pszMsg,_T("Generating monitoring file for Interface %s\r\n"),pszIID);
    this->UserMessage(pszMsg,USER_MESSAGE_INFORMATION);

    // if monitoring file is already existing
    if (CStdFileOperations::DoesFileExists(pszMonitoringFileName))
    {
        // report message to user
        _stprintf(pszMsg,_T("Monitoring file for iid %s already exists. Existing file kept (not replaced).\r\n"),pszIID);
        this->UserMessage(pszMsg,USER_MESSAGE_INFORMATION);
        return TRUE;
    }
 
    // create a new monitoring file
    if (!CTextFile::CreateTextFile(pszMonitoringFileName,&hFile))
    {
        // report message to user
        _stprintf(pszMsg,_T("Error creating file %s\r\n"),pszMonitoringFileName);
        this->UserMessage(pszMsg,USER_MESSAGE_ERROR);
        return FALSE;
    }

    CMethodInfo* pMethodInfo;
    CLinkListItem* pItem;
    TCHAR pszFuncDescr[2048];
    TCHAR psz[50];
    
    // check IUnknown interface
    BOOL bIUnknownInterface = IsEqualIID(pInterfaceInfo->Iid,IID_IUnknown);

    if (hFile!=INVALID_HANDLE_VALUE)
    {
        // write monitoring file header
        CTextFile::WriteText(hFile,COM_INTERFACE_MONITORING_FILE_HEADER);
        CTextFile::WriteText(hFile,_T(";Generated from file "));
        CTextFile::WriteText(hFile,this->CurrentTypeLibrary);
        CTextFile::WriteText(hFile,_T("\r\n\r\n"));

        // write interface name
        if (InterfaceNameRetrievalSuccess)
        {
            CTextFile::WriteText(hFile,_T(";@InterfaceName="));
            CTextFile::WriteText(hFile,InterfaceName);
            CTextFile::WriteText(hFile,_T("\r\n\r\n"));
        }

        // if not IUnknown interface
        if(!bIUnknownInterface)
        {
            TCHAR pszInheritedIIDName[MAX_PATH];
            TCHAR pszInheritedIID[128];

            // add Derived from
            CTextFile::WriteText(hFile,_T(";Derived from "));
            CGUIDStringConvert::TcharFromIID(pInheritedInterface,pszInheritedIID);

            // if inherited interface name was successful
            if (*pInheritedInterFaceName)
            {
                // convert name from WCHAR* to TCHAR*
#if (defined(UNICODE)||defined(_UNICODE))
                _tcsncpy(pszInheritedIIDName,*pInheritedInterFaceName,MAX_PATH-1);
                InterfaceName[MAX_PATH-1]=0;
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(*pInheritedInterFaceName,pszInheritedIIDName,MAX_PATH);
#endif

                // write inherited interface name
                CTextFile::WriteText(hFile,pszInheritedIIDName);
            }
            else
            {
                // try to get inherited interface name from another way
                if (CGUIDStringConvert::GetInterfaceName(pszInheritedIID,pszInheritedIIDName,MAX_PATH))
                {
                    // write inherited interface name
                    CTextFile::WriteText(hFile,pszInheritedIIDName);
                }
                else
                {
                    // write inherited interface ID
                    CTextFile::WriteText(hFile,pszInheritedIID);
                }
            }

            // add line break
            CTextFile::WriteText(hFile,_T("\r\n"));

            // add inherited interface id
            CTextFile::WriteText(hFile,_T("BaseIID="));
            CTextFile::WriteText(hFile,pszInheritedIID);
            CTextFile::WriteText(hFile,_T("\r\n\r\n"));
        }
    }

    // for each methods in list
    for (pItem=pInterfaceInfo->pMethodInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        pMethodInfo = (CMethodInfo*)pItem->ItemData;

        // check pointer validity
        if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
            continue;

        if(!bIUnknownInterface)
        {
            if (pMethodInfo->VTBLIndex<=2) // IUnknown methods, they are already included by BaseIID=
                                           // Notice : this check is quit useless as description first index is ("inherited last index" + 1)
                continue;
        }

        if ((SIZE_T)pMethodInfo->VTBLIndex<=InheritedInterFaceMaxVTBLIndex) // Inherited methods, they are already included by BaseIID=
            continue;

        // if user wants to create a monitoring file
        if (hFile!=INVALID_HANDLE_VALUE)
        {
            // write VTBL_Index=
            CTextFile::WriteText(hFile,COM_DEFINITION_CURRENT_IID_VTBL_INDEX);

            // write VTBL Index
            _stprintf(psz,_T("%u"),pMethodInfo->VTBLIndex);
            CTextFile::WriteText(hFile,psz);
            CTextFile::WriteText(hFile,_T("|"));

            // return value
            if (pMethodInfo->Return.IsHRESULT())
                CTextFile::WriteText(hFile,_T("HRESULT")); // avoid to disturb user with WinApiOverride types for a standard COM HRESULT
            else
                // here some types can be replaced by there equivalent (HRESULT will appear as a LONG, which is less understandable for the |FailureIfNegativeRet option
                CTextFile::WriteText(hFile,CSupportedParameters::GetParamName(pMethodInfo->Return.GetWinAPIOverrideType()));
            
            // calling convention
            switch(pMethodInfo->callconv)
            {
            case CC_FASTCALL:
                CTextFile::WriteText(hFile,_T(" __fastcall "));
                break;
            case CC_CDECL:
                CTextFile::WriteText(hFile,_T(" __cdecl "));
                break;
            case CC_STDCALL:
                CTextFile::WriteText(hFile,_T(" __stdcall "));
                break;
            //default:
            // no longer supported calling convention --> let WinApiOverride use the default calling convention
            //    break;
            }

            // InterfaceName::
            if (InterfaceNameRetrievalSuccess)
            {
                CTextFile::WriteText(hFile,InterfaceName);
                CTextFile::WriteText(hFile,_T("::"));
            }

            // write func desc
            pMethodInfo->ToString(FALSE,pszFuncDescr);
            CTextFile::WriteText(hFile,pszFuncDescr);

            // add |Out key word if method has an out parameter
            if (pMethodInfo->HasAnOutParameter())
                CTextFile::WriteText(hFile,_T("|Out"));

            // if return value is HRESULT
            if (pMethodInfo->Return.IsHRESULT())
                CTextFile::WriteText(hFile,_T("|FailureIfNegativeRet"));

            // add line break
            CTextFile::WriteText(hFile,_T("\r\n"));
        }
    }

    if (hFile!=INVALID_HANDLE_VALUE)
        // close file
        CloseHandle(hFile);

    return TRUE;
}