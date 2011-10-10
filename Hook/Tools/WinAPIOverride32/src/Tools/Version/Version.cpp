#include "version.h"

CVersion::CVersion(void)
{
    this->pVersionInfo = NULL;
}

CVersion::~CVersion(void)
{
    if (this->pVersionInfo)
        delete [] this->pVersionInfo;
}

BOOL CVersion::Read(LPCTSTR FileName)
{
    DWORD dwRet;
    UINT iLen;
    DWORD dwHandle;
    LPVOID lpvi;

    // read file version info
    dwRet = GetFileVersionInfoSize(FileName, &dwHandle);
    if (dwRet <= 0)
        return FALSE;

    // allocate version info
    if (this->pVersionInfo)
        delete[] this->pVersionInfo;
    this->pVersionInfo = new BYTE[dwRet]; 

    if (!GetFileVersionInfo(FileName, 0, dwRet, this->pVersionInfo))
        return FALSE;
    memset(&this->FixedFileInfo, 0, sizeof(VS_FIXEDFILEINFO));



	if (!VerQueryValue(this->pVersionInfo, _TEXT("\\"), &lpvi, &iLen))
        return FALSE;
    memcpy(&this->FixedFileInfo,lpvi,sizeof(VS_FIXEDFILEINFO));


    
#if (defined(UNICODE)||defined(_UNICODE))
    this->LangAndCodePage.wCodePage = 1200;// code page unicode
#else
    this->LangAndCodePage.wCodePage = 1252;// code page us ASCII
#endif
    // Get translation info
    if (!VerQueryValue(this->pVersionInfo,_TEXT("\\VarFileInfo\\Translation"),&lpvi, &iLen))
        return FALSE;

    // memcpy(&this->LangAndCodePage,lpvi,iLen);
    memcpy(&this->LangAndCodePage,lpvi,sizeof(LANGANDCODEPAGE));

    this->GetValue(_TEXT("CompanyName"),this->CompanyName);
    this->GetValue(_TEXT("FileDescription"),this->FileDescription);
    this->GetValue(_TEXT("FileVersion"),this->FileVersion);
    this->GetValue(_TEXT("InternalName"),this->InternalName);
    this->GetValue(_TEXT("LegalCopyright"),this->LegalCopyright);
    this->GetValue(_TEXT("OriginalFilename"),this->OriginalFilename);
    this->GetValue(_TEXT("ProductName"),this->ProductName);
    this->GetValue(_TEXT("ProductVersion"),this->ProductVersion);

    // check signature (must be VS_FFI_SIGNATURE)
    return this->FixedFileInfo.dwSignature == VS_FFI_SIGNATURE;
}


// Get string file info.
// Key name is something like "CompanyName".
BOOL CVersion::GetValue(LPCTSTR lpKeyName,LPTSTR szValue)
{
    TCHAR sz[MAX_PATH];
    UINT iLenVal;
    LPVOID lpvi;

    if (!this->pVersionInfo)
        return FALSE;

    if (IsBadWritePtr(szValue,1))
        return FALSE;

    if (IsBadReadPtr(lpKeyName,1))
        return FALSE;

    *szValue=0;

    // To get a string value must pass query in the form
    //
    //    "\StringFileInfo\<langID><codepage>\keyname"
    //
    // where <lang-codepage> is the languageID concatenated with the
    // code page, in hex. Wow.
    //

    _stprintf(sz,_TEXT("\\StringFileInfo\\%04x%04x\\%s"),
        this->LangAndCodePage.wLanguage,
        this->LangAndCodePage.wCodePage,
        lpKeyName);

    if(!VerQueryValue(this->pVersionInfo,sz,&lpvi,&iLenVal))
        return FALSE;
    if (iLenVal>MAX_PATH)
        iLenVal=MAX_PATH;
    memcpy(szValue,lpvi,iLenVal*sizeof(TCHAR));

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetPrettyVersion
// Object: translate something like "1 ,2 ,3 ,4" to "1.2.3.4" notice :  according to windows version, version delimiter can be " ," or "."
// Parameters :
//     in  : LPCTSTR lpVersion : ProductVersion or FileVersion
//           DWORD NbDigits : number of wanted digits
//           SIZE_T PrettyVersionMaxSize : max size of translated string version
//     out : LPTSTR PrettyVersion : translated string version
//     return : 
//-----------------------------------------------------------------------------
BOOL CVersion::GetPrettyVersion(IN LPCTSTR lpVersion,IN const DWORD NbDigits,OUT LPTSTR PrettyVersion,IN const SIZE_T PrettyVersionMaxSize)
{
    LPTSTR pszBegin;
    LPTSTR pszEnd;
    DWORD NbDigitsAdded;
    SIZE_T RemainingMaxSize;

    
    RemainingMaxSize=PrettyVersionMaxSize-1;// -1 for \0
    *PrettyVersion=0;
    pszBegin=(LPTSTR)lpVersion;

    pszEnd=_tcschr(pszBegin,',');
    if (!pszEnd)// according to windows version, version delimiter can be " ," or "."
    {
        
        _tcscpy(PrettyVersion,pszBegin);
        pszEnd = PrettyVersion;
        for (NbDigitsAdded=0;
            NbDigitsAdded<NbDigits;
            NbDigitsAdded++)
        {
            pszEnd=_tcschr(pszEnd+1,'.');
        }
        if (!pszEnd)
            return FALSE;
        // else
        *pszEnd = 0;
        return TRUE;
    }
    // else

    for (NbDigitsAdded=0;
         NbDigitsAdded<NbDigits;
         NbDigitsAdded++)
    {
        // find next ','
        pszEnd=_tcschr(pszBegin,',');

        // if not found
        if (!pszEnd)
        {
            // if remaining content can't be inserted in buffer
            if (_tcslen(pszBegin)+1>RemainingMaxSize)// +1 for '.' splitter
                return FALSE;

            // if not first digit (main version number)
            if (NbDigitsAdded)
            {
                // add digit delimiter
                _tcscat(PrettyVersion,_T("."));
            }

            // add remaining content to buffer
            _tcscat(PrettyVersion,pszBegin);

            // remaining content is supposed to contains a digit
            // so increase NbDigitsAdded
            NbDigitsAdded++;

            // if NbDigitsAdded match the wanted digit number
            if (NbDigitsAdded==NbDigits)
                return TRUE;
            else
                return FALSE;
        }

        // if next digit content can't be inserted in buffer
        if ((SIZE_T)(pszEnd-pszBegin)+1>RemainingMaxSize) // +1 for '.' splitter
            return FALSE;

        // if not first digit (main version number)
        if (NbDigitsAdded)
        {
            // add digit delimiter
            _tcscat(PrettyVersion,_T("."));
        }

        // add next digit to buffer
        _tcsncat(PrettyVersion,pszBegin,pszEnd-pszBegin);

        // update remaining size
        RemainingMaxSize-=(SIZE_T)(pszEnd-pszBegin);

        // remove ',' and space
        pszBegin=pszEnd+2;
    }
    return TRUE;
}