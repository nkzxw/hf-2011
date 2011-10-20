#include "stdafx.h"
#include "commonFn.h"
#include <share.h>
#include <atlrx.h>

WORD DetectWinVersion()
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if(!GetVersionEx((OSVERSIONINFO *) &osvi))
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if(!GetVersionEx((OSVERSIONINFO *) &osvi))
        {
            return(FALSE);
        }
    }

    switch(osvi.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_NT:
            if(osvi.dwMajorVersion <= 4)
            {
                return(_WINVER_NT4_);
            }

            if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
            {
                return(_WINVER_2K_);
            }

            if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
            {
                return(_WINVER_XP_);
            }

            return(_WINVER_XP_);                // never return Win95 if we get the info about a NT system

        case VER_PLATFORM_WIN32_WINDOWS:
            if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
            {
                return(_WINVER_95_);
            }

            if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
            {
                return(_WINVER_98_);
            }

            if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
            {
                return(_WINVER_ME_);
            }
            break;

        default:
            break;
    }

    return(_WINVER_95_);                        // there should'nt be anything lower than this
}

BOOL IsRunningXPSP2()
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if(!GetVersionEx((OSVERSIONINFO *) &osvi))
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if(!GetVersionEx((OSVERSIONINFO *) &osvi))
        {
            return(FALSE);
        }
    }

    if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
    {
        if(osvi.wServicePackMajor >= 2)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

int GetSystemErrorString(DWORD dwError, CString &rstrError)
{
    // FormatMessage language flags:
    //
    // - MFC uses: MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)
    //				SUBLANG_SYS_DEFAULT = 0x02 (system default)
    //
    // - SDK uses: MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
    //				SUBLANG_DEFAULT		= 0x01 (user default)
    //
    //
    // Found in "winnt.h"
    // ------------------
    //  Language IDs.
    //
    //  The following two combinations of primary language ID and
    //  sublanguage ID have special semantics:
    //
    //    Primary Language ID   Sublanguage ID      Result
    //    -------------------   ---------------     ------------------------
    //    LANG_NEUTRAL          SUBLANG_NEUTRAL     Language neutral
    //    LANG_NEUTRAL          SUBLANG_DEFAULT     User default language
    //    LANG_NEUTRAL          SUBLANG_SYS_DEFAULT System default language
    //
    // *** SDK notes also:
    // If you pass in zero, 'FormatMessage' looks for a message for LANGIDs in
    // the following order:
    //
    //	1) Language neutral
    //	2) Thread LANGID, based on the thread's locale value
    //  3) User default LANGID, based on the user's default locale value
    //	4) System default LANGID, based on the system default locale value
    //	5) US English
    LPTSTR  pszSysMsg = NULL;
    DWORD   dwLength = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL,
                                     dwError,
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                                     (LPTSTR) & pszSysMsg,
                                     0,
                                     NULL);
    if(pszSysMsg != NULL && dwLength != 0)
    {
        if(dwLength >= 2 && pszSysMsg[dwLength - 2] == _T('\r'))
        {
            pszSysMsg[dwLength - 2] = _T('\0');
        }

        rstrError = pszSysMsg;
        rstrError.Replace(_T("\r\n"), _T(" ")); // some messages contain CRLF within the message!?
    }
    else
    {
        rstrError.Empty();
    }

    if(pszSysMsg)
    {
        LocalFree(pszSysMsg);
    }

    return(rstrError.GetLength());
}

int GetModuleErrorString(DWORD dwError, CString &rstrError, LPCTSTR pszModule)
{
    LPTSTR  pszSysMsg = NULL;
    DWORD   dwLength = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     GetModuleHandle(pszModule),
                                     dwError,
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                                     (LPTSTR) & pszSysMsg,
                                     0,
                                     NULL);
    if(pszSysMsg != NULL && dwLength != 0)
    {
        if(dwLength >= 2 && pszSysMsg[dwLength - 2] == _T('\r'))
        {
            pszSysMsg[dwLength - 2] = _T('\0');
        }

        rstrError = pszSysMsg;
        rstrError.Replace(_T("\r\n"), _T(" ")); // some messages contain CRLF within the message!?
    }
    else
    {
        rstrError.Empty();
    }

    if(pszSysMsg)
    {
        LocalFree(pszSysMsg);
    }

    return(rstrError.GetLength());
}

int GetErrorMessage(DWORD dwError, CString &rstrErrorMsg, DWORD dwFlags)
{
    int iMsgLen = GetSystemErrorString(dwError, rstrErrorMsg);
    if(iMsgLen == 0)
    {
        if((long) dwError >= 0)
        {
            rstrErrorMsg.Format(_T("Error %u"), dwError);
        }
        else
        {
            rstrErrorMsg.Format(_T("Error 0x%08x"), dwError);
        }
    }
    else if(dwFlags & 1)
    {
        CString strFullErrorMsg;
        if((long) dwError >= 0)
        {
            strFullErrorMsg.Format(_T("Error %u: %s"), dwError, rstrErrorMsg);
        }
        else
        {
            strFullErrorMsg.Format(_T("Error 0x%08x: %s"), dwError, rstrErrorMsg);
        }

        rstrErrorMsg = strFullErrorMsg;
    }

    return(rstrErrorMsg.GetLength());
}

CString GetErrorMessage(DWORD dwError, DWORD dwFlags)
{
    CString strError;
    GetErrorMessage(dwError, strError, dwFlags);
    return(strError);
}

BOOL RegularExpressionMatch(CString regexpr, CString teststring)
{
    CAtlRegExp<>    reFN;

    REParseError    status = reFN.Parse(regexpr);
    if(REPARSE_ERROR_OK != status)
    {
        // Unexpected error.
        return(FALSE);
    }

    CAtlREMatchContext<>    mcUrl;
    if(!reFN.Match(teststring, &mcUrl))
    {
        // Unexpected error.
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

BOOL IsUnicodeFile(LPCTSTR pszFilePath)
{
    BOOL    bResult = FALSE;
    FILE    *fp = _tfsopen(pszFilePath, _T("rb"), _SH_DENYWR);
    if(fp != NULL)
    {
        WORD    wBOM = 0;
        bResult = (fread(&wBOM, sizeof(wBOM), 1, fp) == 1 && wBOM == 0xFEFF);
        fclose(fp);
    }

    return(bResult);
}

BOOL HaveAdminAccess()
{
    // make sure this is NT first
    OSVERSIONINFO   ver;
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    // if this fails, we want to default to being enabled
    if(!(GetVersionEx(&ver) && ver.dwPlatformId == VER_PLATFORM_WIN32_NT))
    {
        return(TRUE);
    }

    // now check with the security manager
    HANDLE  hHandleToken;

    if(! :: OpenProcessToken( :: GetCurrentProcess(), TOKEN_READ, &hHandleToken))
    {
        return(FALSE);
    }

    UCHAR   TokenInformation[1024];
    DWORD   dwTokenInformationSize;

    BOOL    bSuccess = :: GetTokenInformation(hHandleToken,
                                              TokenGroups,
                                              TokenInformation,
                                              sizeof(TokenInformation),
                                              &dwTokenInformationSize);

    :: CloseHandle(hHandleToken);

    if(!bSuccess)
    {
        return(FALSE);
    }

    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID                        psidAdministrators;

    if(! :: AllocateAndInitializeSid(&siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0,
       0, 0, 0, &psidAdministrators))
    {
        return(FALSE);
    }

    // assume that we don't find the admin SID.
    bSuccess = FALSE;

    PTOKEN_GROUPS   ptgGroups = (PTOKEN_GROUPS) TokenInformation;

    for(UINT x = 0; x < ptgGroups->GroupCount; x++)
    {
        if( :: EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid))
        {
            bSuccess = TRUE;
            break;
        }
    }

    :: FreeSid(psidAdministrators);

    return(bSuccess);
}
/*---------------------------------------------------------------------------------------------
Name				:	CryptString(const CString &strNormal)
Purpose				:	Encrypt a String
Parameters			:
						const CString & strNormal - String to be Encrypted
Return				:	Encrypted string
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CryptString(const CString &strNormal)
{
    uint32  dwStrLen = strNormal.GetLength();
    BYTE    *pBuf = new BYTE[dwStrLen * 2 + 1];
    
	_tcscpy((TCHAR *) pBuf, strNormal);

    for(uint32 dwBufSize = dwStrLen; dwBufSize--;)
    {
        ((PSTR) pBuf)[2 * dwBufSize + 1] = (((PSTR) pBuf)[dwBufSize] % 0x10) + 'A';
        ((PSTR) pBuf)[2 * dwBufSize] = (((PSTR) pBuf)[dwBufSize] / 0x10) + 'A';
    }

    pBuf[dwStrLen * 2] = 0;

    CString str(pBuf);
    delete[] pBuf;

    return(str);
}
/*---------------------------------------------------------------------------------------------
Name				:	DecryptString(const CString &strCrypted)
Purpose				:	decrypt a String
Parameters			:
						const CString & strCrypted - String to be Decrypted
Return				:	Decrypted string
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString DecryptString(const CString &strCrypted)
{
	uint32	dwStrLen = strCrypted.GetLength() / 2;
	BYTE *pBuf = new BYTE[dwStrLen + 1];

	for (uint32 dwPos = 0; dwPos < dwStrLen; dwPos++)
	{
		((PSTR) pBuf)[dwPos] = (((strCrypted.GetAt(dwPos * 2) - _T('A')) << 4) & 0xFF) |
			((strCrypted.GetAt(dwPos * 2 + 1) - _T('A')) & 0xFF);
	}

	pBuf[dwStrLen] = 0;

	CString str(pBuf);
	delete[] pBuf;

	return str;
}

IMPLEMENT_SERIAL(CNetDefenderRules,CObject,1)
void CNetDefenderRules::Serialize( CArchive& ar )
{// call base class function first
	// base class is CObject in this case
	CObject::Serialize( ar );

	// now do the stuff for our specific class
	if( ar.IsStoring() )
		ar << m_nProtocol << m_nSourceIp << m_nDestinationIp << m_nSourceMask 
		<< m_nDestinationMask << m_nSourcePort << m_nDestinationPort << m_bDrop;
	else
		ar >> m_nProtocol >> m_nSourceIp >> m_nDestinationIp >> m_nSourceMask 
		>> m_nDestinationMask >> m_nSourcePort >> m_nDestinationPort >> m_bDrop;

}
void CNetDefenderRules::SetIpFilter(IPFilter& filterObj)
{
	m_nProtocol = filterObj.protocol;
	m_nSourceIp = filterObj.sourceIp;
	m_nDestinationIp = filterObj.destinationIp;
	m_nSourceMask = filterObj.sourceMask;
	m_nDestinationMask = filterObj.destinationMask;
	m_nSourcePort = filterObj.sourcePort;
	m_nDestinationPort = filterObj.destinationPort;
	m_bDrop = filterObj.drop;
}

IPFilter CNetDefenderRules::GetIpFilter()
{
	IPFilter filterObj;
	 filterObj.protocol = static_cast<USHORT>(m_nProtocol);
	 filterObj.sourceIp = m_nSourceIp;
	 filterObj.destinationIp = m_nDestinationIp;
	 filterObj.sourceMask =	m_nSourceMask;
	 filterObj.destinationMask =	m_nDestinationMask;
	 filterObj.sourcePort = static_cast<USHORT>(m_nSourcePort);
	 filterObj.destinationPort = static_cast<USHORT>(m_nDestinationPort);
	 filterObj.drop = m_bDrop;
	 return filterObj;

}
CString LongToIPStr(UINT nIpAddress)
{
	in_addr stIpAddr;
	CStringA strIp;
	stIpAddr.s_addr = nIpAddress;
	if(nIpAddress == 0)
	{
		strIp = _T("All");
	}
	else
	{
		strIp = inet_ntoa(stIpAddr);
	}
	
	return CString(strIp);
}
CString GiveDescIp(UINT nIpAddress)
{
	in_addr stIpAddr;
	CStringA strIp;
	CString strUIp ;
	stIpAddr.s_addr = nIpAddress;
	if(nIpAddress == 0)
	{
		strUIp = _T("Any");
	}
	else
	{
		strIp = inet_ntoa(stIpAddr);
		strUIp = strIp;
		CString strMyIp = GetCurrentIp();
		if(strUIp.CompareNoCase(strMyIp)==0)
		{
			strUIp.Format(_T("My machine"));
		}
	}

	return strUIp;

}
CString PortNoToStr(USHORT nPortNo)
{
	USHORT nDestport = ntohs(nPortNo);
	CString strTemp;
	if(nPortNo == 0)
	{
		strTemp = _T("Any");
	}
	else
	{
		strTemp.Format(_T("%d"),nDestport);
	}
	
	return strTemp;
}
CString  intToPotocol(int nPortNo)
{
	CString strProtocol;
	if(nPortNo == 0)
	{
		strProtocol = _T("ANY");
	}

	if(nPortNo == 1)
	{
		strProtocol = _T("ICMP");
	}

	if(nPortNo == 6)
	{
		strProtocol = _T("TCP");
	}

	if(nPortNo == 17)
	{
		strProtocol = _T("UDP");
	}
	return strProtocol;
}
CString BoolToAction(BOOL bAction)
{
	CString strAcion;
	if(bAction == 0)
	{
		strAcion =  _T("ALLOW");
	}

	if(bAction == 1)
	{
		strAcion =  _T("DENY");
	}
	return strAcion;
}
CString GetCurrentIp()
{
	WSADATA wsaData;
	struct hostent * hostinfo;
	CString strIp;
	WSAStartup( MAKEWORD(2,1), &wsaData );
	char name[MAX_PATH];
	if( gethostname ( name, sizeof(name)) == 0)
	{
		if((hostinfo = gethostbyname(name)) != NULL)
		{
			for( int i = 0;  hostinfo->h_addr_list[i]!= NULL; i++ )
			{
				strIp = inet_ntoa (*(struct in_addr *)hostinfo->h_addr_list[i]);
			}
		}
	}
	
	WSACleanup( );
	return strIp;
}
