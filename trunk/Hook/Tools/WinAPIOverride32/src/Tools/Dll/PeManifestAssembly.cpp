#include "PeManifestAssembly.h"
#include "../APIError/APIError.h"
#include "../String/AnsiUnicodeConvert.h"
#include <stdio.h>

#define ASSEMBLY_DEPENDENT_ASSEMBLY_BEGIN_TAG  _T("<dependentAssembly>")
#define ASSEMBLY_DEPENDENT_ASSEMBLY_END_TAG  _T("</dependentAssembly>")
#define ASSEMBLY_IDENTITY_BEGIN_TAG _T("<assemblyIdentity")
#define ASSEMBLY_IDENTITY_END_TAG _T("</assemblyIdentity>")
#define ASSEMBLY_IDENTITY_TYPE _T("type=")
#define ASSEMBLY_IDENTITY_NAME _T("name=")
#define ASSEMBLY_IDENTITY_VERSION _T("version=")
#define ASSEMBLY_IDENTITY_PROCESSOR_ARCHITECTURE _T("processorArchitecture=")
#define ASSEMBLY_IDENTITY_PUBLIC_KEY_TOKEN _T("publicKeyToken=")
#define ASSEMBLY_IDENTITY_LANGUAGE _T("language=")

CPeManifestAssemblyIdentity::CPeManifestAssemblyIdentity()
{
    this->Type=NULL;
    this->Name=NULL;
    this->Version=NULL;
    this->Architecture=NULL;
    this->Token=NULL;
    this->Language=NULL;
    *this->DependencyFullPathWithBackSlash=NULL;
}
CPeManifestAssemblyIdentity::~CPeManifestAssemblyIdentity()
{
    DeleteIfNotNull(this->Type);
    DeleteIfNotNull(this->Name);
    DeleteIfNotNull(this->Version);
    DeleteIfNotNull(this->Architecture);
    DeleteIfNotNull(this->Token);
    DeleteIfNotNull(this->Language);
}

TCHAR* __fastcall CPeManifestAssemblyIdentity::ExtractField(TCHAR* const StrIdentityContent,TCHAR* const FieldName)
{
    TCHAR* FieldStart;
    TCHAR* FieldEnd;
    FieldStart = _tcsstr(StrIdentityContent,FieldName);
    if (!FieldStart)
        return NULL;

    // find beginning "
    FieldStart = _tcschr(FieldStart,'"');
    if (!FieldStart)
        return NULL;

    FieldStart++;
    // find ending "
    // FIXE ME : Works only if field don't have " inside
    FieldEnd = _tcschr(FieldStart,'"');
    if (!FieldEnd)
        return NULL;

    SIZE_T StringSize = FieldEnd-FieldStart;//(FieldEnd-1-FieldStart)+1; // -1 because FieldEnd points on "

    TCHAR* Ret;
    // allocate memory
    Ret = new TCHAR[StringSize+1];
    // copy content
    _tcsncpy(Ret,FieldStart,StringSize);
    // assume string ending
    Ret[StringSize]=0;

    return Ret;
}
BOOL __fastcall CPeManifestAssemblyIdentity::Parse(TCHAR* const Content)
{
    // content is like  type="win32" name="Microsoft.VC90.MFC" version="9.0.21022.8" processorArchitecture="x86" publicKeyToken="1fc8b3b9a1e18e3b">
    this->Type=this->ExtractField(Content,ASSEMBLY_IDENTITY_TYPE);
    this->Name=this->ExtractField(Content,ASSEMBLY_IDENTITY_NAME);
    this->Version=this->ExtractField(Content,ASSEMBLY_IDENTITY_VERSION);
    this->Architecture=this->ExtractField(Content,ASSEMBLY_IDENTITY_PROCESSOR_ARCHITECTURE);
    this->Token=this->ExtractField(Content,ASSEMBLY_IDENTITY_PUBLIC_KEY_TOKEN);
    this->Language=this->ExtractField(Content,ASSEMBLY_IDENTITY_LANGUAGE);

    return TRUE;
}
const TCHAR* CPeManifestAssemblyIdentity::GetType()
{
    return this->GetSecureContent(this->Type);
}
const TCHAR* CPeManifestAssemblyIdentity::GetName()
{
    return this->GetSecureContent(this->Name);
}
const TCHAR* CPeManifestAssemblyIdentity::GetVersion()
{
    return this->GetSecureContent(this->Version);
}
const TCHAR* CPeManifestAssemblyIdentity::GetProcessorArchitecture()
{
    return this->GetSecureContent(this->Architecture);
}
const TCHAR* CPeManifestAssemblyIdentity::GetPublicKeyToken()
{
    return this->GetSecureContent(this->Token);
}
const TCHAR* CPeManifestAssemblyIdentity::GetPublicLanguage()
{
    return this->GetSecureContent(this->Language);
}
const TCHAR* CPeManifestAssemblyIdentity::GetDependencyFullPathWithBackSlash()
{
    // do an on demand forging (only once) to avoid to loose time if not needed by user
    if (*this->DependencyFullPathWithBackSlash==NULL)
    {
        ///////////////////
        // forge DependencyFullPathWithBackSlash
        ///////////////////

        // get windows dir
        TCHAR WindowsDir[MAX_PATH];
        ::GetWindowsDirectory(WindowsDir, MAX_PATH);

        // get language
        TCHAR* UsedLanguageName;
        UsedLanguageName = (TCHAR*)this->GetPublicLanguage();
        if (*UsedLanguageName==0)
            UsedLanguageName = _T("none");


// FIX ME XXX to find
        // create DependencyFullPathWithBackSlash
//       _sntprintf(this->DependencyFullPathWithBackSlash,
//                   MAX_PATH,
//                   _T("%s\\winsxs\\%s_%s_%s_%s_%s_%s\\"),
//                   WindowsDir,
//                   this->GetProcessorArchitecture(),
//                   this->GetName(),
//                   this->GetPublicKeyToken(),
//                   this->GetVersion(),
//                   UsedLanguageName,
//                   XXX
//                   );

        _sntprintf(this->DependencyFullPathWithBackSlash,
                    MAX_PATH,
                    _T("%s\\winsxs\\%s_%s_%s_%s_%s_%s"),
                    WindowsDir,
                    this->GetProcessorArchitecture(),
                    this->GetName(),
                    this->GetPublicKeyToken(),
                    this->GetVersion(),
                    UsedLanguageName,
                    _T("*")
                    );

        WIN32_FIND_DATA DirectoryInfos;
        HANDLE hFind = ::FindFirstFile(this->DependencyFullPathWithBackSlash,&DirectoryInfos);
        if ( hFind != INVALID_HANDLE_VALUE)
        {
            ::FindClose(hFind);
            _sntprintf(this->DependencyFullPathWithBackSlash,
                        MAX_PATH,
                        _T("%s\\winsxs\\%s\\"),
                        WindowsDir,
                        DirectoryInfos.cFileName
                        );

        }
// end of FIX ME XXX to find
    }
    return this->DependencyFullPathWithBackSlash;
}

CPeManifestAssembly::CPeManifestAssembly()
{
    this->pDependenciesList = new CLinkListSimpleSingleThreaded();
    this->pCurrentEnumerationItem = this->pDependenciesList->Head;
}
CPeManifestAssembly::~CPeManifestAssembly()
{
    this->FreeMemory();
    delete this->pDependenciesList;
}
void CPeManifestAssembly::FreeMemory()
{
    CLinkListItem* pItem;
    CPeManifestAssemblyIdentity* pPeManifestAssemblyIdentity;
    for (pItem = this->pDependenciesList->Head ; pItem; pItem=pItem->NextItem )
    {
        pPeManifestAssemblyIdentity = (CPeManifestAssemblyIdentity*)pItem->ItemData;
        if (pPeManifestAssemblyIdentity)
            delete pPeManifestAssemblyIdentity;
    }
    this->pDependenciesList->RemoveAllItems();
}
BOOL CPeManifestAssembly::Parse(TCHAR* FileName)
{
    // free previous parsing if any
    this->FreeMemory();

    // reset iterator
    this->pCurrentEnumerationItem = this->pDependenciesList->Head;
    
    BOOL bRet = FALSE;
    HMODULE hModule = ::LoadLibraryEx(FileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(hModule!=NULL)
    {
        bRet = ::EnumResourceNames(hModule, RT_MANIFEST, CPeManifestAssembly::EnumResNameProc, (LONG_PTR)this);
        ::FreeLibrary(hModule);
    }
    return bRet;

}

BOOL CALLBACK CPeManifestAssembly::EnumResNameProc(HMODULE hModule,LPCTSTR lpszType,LPTSTR lpszName,LONG_PTR lParam)
{
    CPeManifestAssembly* pManifestAssembly = (CPeManifestAssembly*)lParam;
    if( lpszType==RT_MANIFEST)
    {
        HRSRC hResource = ::FindResource(hModule, lpszName, lpszType);
        if( hResource )
        {
            // DWORD dwResource = ::SizeofResource(hModule, hResource);

            HGLOBAL hResData = ::LoadResource(hModule, hResource); 
            if(hResData)
            {
                const BYTE *pResource = (const BYTE *)::LockResource(hResData); 
                if(pResource)
                {
                    // example of manifest content
                    //<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
                    //<dependency>
                    //  <dependentAssembly>
                    //    <assemblyIdentity type="win32" name="Microsoft.VC90.CRT" version="9.0.21022.8" processorArchitecture="x86" publicKeyToken="1fc8b3b9a1e18e3b"></assemblyIdentity>
                    //  </dependentAssembly>
                    //</dependency>
                    //<dependency>
                    //  <dependentAssembly>
                    //    <assemblyIdentity type="win32" name="Microsoft.VC90.MFC" version="9.0.21022.8" processorArchitecture="x86" publicKeyToken="1fc8b3b9a1e18e3b"></assemblyIdentity>
                    //  </dependentAssembly>
                    //</dependency>
                    //<dependency>
                    //  <dependentAssembly>
                    //    <assemblyIdentity type="win32" name="Microsoft.VC90.ATL" version="9.0.21022.8" processorArchitecture="x86" publicKeyToken="1fc8b3b9a1e18e3b"></assemblyIdentity>
                    //  </dependentAssembly>
                    //</dependency>
                    //</assembly>

                    TCHAR* ResourceContent;
                    // manifest seems to be only in ansi encoding --> convert to TCHAR
                    CAnsiUnicodeConvert::AnsiToTchar((char*)pResource,&ResourceContent);
                    if (ResourceContent)
                    {
                        TCHAR* LocalResourceContent;
                        // assume to get local copy to put \0 for a quicker parsing
                        LocalResourceContent = _tcsdup(ResourceContent);
                        free(ResourceContent);
                        if (LocalResourceContent)
                        {
                            // Do a very light parsing
                            TCHAR* dependentAssemblyBeginTag;
                            TCHAR* dependentAssemblyEndTag;
                            TCHAR* assemblyIdentityBeginTag;
                            TCHAR* assemblyIdentityEndTag;
                            TCHAR* StrIdentityContent;
                            CPeManifestAssemblyIdentity* pIdentity;
                            TCHAR* CurrentPos;
                            SIZE_T assemblyIdentityBeginTagSize;
                            SIZE_T assemblyIdentityEndTagSize;
                            SIZE_T dependentAssemblyBeginTagSize;
                            SIZE_T dependentAssemblyEndTagSize;

                            CurrentPos = LocalResourceContent;
                            dependentAssemblyBeginTagSize = _tcslen(ASSEMBLY_DEPENDENT_ASSEMBLY_BEGIN_TAG);
                            dependentAssemblyEndTagSize = _tcslen(ASSEMBLY_DEPENDENT_ASSEMBLY_END_TAG);
                            assemblyIdentityBeginTagSize = _tcslen(ASSEMBLY_IDENTITY_BEGIN_TAG);
                            assemblyIdentityEndTagSize = _tcslen(ASSEMBLY_IDENTITY_END_TAG);

                            // find <dependentAssembly> and </dependentAssembly>
                            dependentAssemblyBeginTag = _tcsstr(CurrentPos, ASSEMBLY_DEPENDENT_ASSEMBLY_BEGIN_TAG );
                            dependentAssemblyEndTag = _tcsstr(CurrentPos, ASSEMBLY_DEPENDENT_ASSEMBLY_END_TAG );
                            while (dependentAssemblyBeginTag && dependentAssemblyEndTag)
                            {
                                CurrentPos = dependentAssemblyBeginTag+dependentAssemblyBeginTagSize;
                                *dependentAssemblyEndTag=0; // we can do this as we use a local buffer

                                // find <assemblyIdentity
                                assemblyIdentityBeginTag = _tcsstr(CurrentPos,ASSEMBLY_IDENTITY_BEGIN_TAG);
                                assemblyIdentityEndTag = _tcsstr(CurrentPos,ASSEMBLY_IDENTITY_END_TAG);
                                while (assemblyIdentityBeginTag && assemblyIdentityEndTag)
                                {
                                    // point after <assemblyIdentity
                                    StrIdentityContent = assemblyIdentityBeginTag + assemblyIdentityBeginTagSize;

                                    pIdentity = new CPeManifestAssemblyIdentity();
                                    // parse identity content
                                    if (pIdentity->Parse(StrIdentityContent))
                                    {
                                        //	on success, add the dependency to the collection
                                        pManifestAssembly->AddDependency(pIdentity);
                                    }
                                    else
                                        delete pIdentity;

                                    // point after </assemblyIdentity>
                                    CurrentPos = assemblyIdentityEndTag + assemblyIdentityEndTagSize;

                                    // find next <assemblyIdentity
                                    assemblyIdentityBeginTag = _tcsstr(CurrentPos,ASSEMBLY_IDENTITY_BEGIN_TAG);
                                }

                                // point after </dependentAssembly>
                                CurrentPos = dependentAssemblyEndTag + dependentAssemblyEndTagSize;

                                // find next <dependentAssembly> and </dependentAssembly>
                                dependentAssemblyBeginTag = _tcsstr(CurrentPos, ASSEMBLY_DEPENDENT_ASSEMBLY_BEGIN_TAG );
                                dependentAssemblyEndTag = _tcsstr(CurrentPos, ASSEMBLY_DEPENDENT_ASSEMBLY_END_TAG );
                            }
                            free(LocalResourceContent);
                        }
                    }

                }

                UnlockResource(hResData); 
                ::FreeResource(hResData); 
            }
        }
    }

    //	Keep going.
    return TRUE;
}

void CPeManifestAssembly::AddDependency(CPeManifestAssemblyIdentity* pAssemblyIdentity)
{
    this->pDependenciesList->AddItem(pAssemblyIdentity);
}

SIZE_T CPeManifestAssembly::GetDependenciesCount()
{
    return this->pDependenciesList->GetItemsCount();
}
CPeManifestAssemblyIdentity* CPeManifestAssembly::GetFirstDependency()
{
    this->pCurrentEnumerationItem = this->pDependenciesList->Head;

    if (!this->pCurrentEnumerationItem)
        return NULL;

    return (CPeManifestAssemblyIdentity*)this->pCurrentEnumerationItem->ItemData;
}
CPeManifestAssemblyIdentity* CPeManifestAssembly::GetNextDependency()
{
    if (!this->pCurrentEnumerationItem)
        return NULL;

    this->pCurrentEnumerationItem = this->pCurrentEnumerationItem->NextItem;
    if (!this->pCurrentEnumerationItem)
        return NULL;

    return (CPeManifestAssemblyIdentity*)this->pCurrentEnumerationItem->ItemData;
}