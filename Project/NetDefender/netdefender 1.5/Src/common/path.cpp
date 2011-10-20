//////////////////////////////////////////////////////////////////////
//	Implemented by Samuel Gonzalo 
//
//	You may freely use or modify this code 
//////////////////////////////////////////////////////////////////////
//
// Path.cpp: implementation of the CPath class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Path.h"
#include <io.h>
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPath::CPath()
{
	_bIsRelative = FALSE;
}
/*---------------------------------------------------------------------------------------------
Name				:	CPath(LPCTSTR szPath, BOOL bIsFolderPath = 0)
Purpose				:	Construct the object with given path
Parameters			:
LPCTSTR  szPath -	null terminated string containing the path.
BOOL  bIsFolderPath -	must be set to TRUE if we know szPath represents 
a folder path but we are not sure if it's terminated with a 
slash or back slash character ('/' or '\').  imagine you 
pass the string "c:\\temp\\subfolder" to this method. 
If bIsFolderPath is not set to TRUE, the class interprets that 
"subfolder" is a file name with no extension.
Return				:	Void.
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/

CPath::CPath(LPCTSTR szPath, BOOL bIsFolderPath)
{
	SetPath(szPath, bIsFolderPath);
}

CPath::~CPath()
{

}
/*---------------------------------------------------------------------------------------------
Name				:	SetPath(LPCTSTR szPath, BOOL bIsFolderPath = FALSE)
Purpose				:	// Parses a path 
Parameters			:
LPCTSTR  szPath - a null terminated string containing the path. 
BOOL  bIsFolderPath - 	must be set to TRUE if we know szPath represents 
a folder path but we are not sure if it's terminated with a 
slash or back slash character ('/' or '\').  imagine you 
pass the string "c:\\temp\\subfolder" to this method. 
If bIsFolderPath is not set to TRUE, the class interprets that 
"subfolder" is a file name with no extension.
Return				:	Void.
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
void CPath::SetPath(LPCTSTR szPath, BOOL bIsFolderPath)
{
	TCHAR	szParamPath[_MAX_PATH];
	TCHAR	szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
	TCHAR	szName[_MAX_FNAME], szExt[_MAX_EXT];

	// Reset
	_strOriginalPath.Empty();
	_strDriveLabel.Empty();
	_bIsRelative = FALSE;
	_strExtName.Empty();

	// Original path
	_strOriginalPath = szPath;

	// Get args and remove them from path
	szParamPath[0] = 0x0;
	_tcscpy(szParamPath, szPath);

	if(szParamPath[0] == 0x0)
	{
		return;
	}

	_tsplitpath(szParamPath, szDrive, szDir, szName, szExt);

	// Drive
	_strDrive = szDrive;

	// Directory
	_strDir = szDir;
	_strDir.Replace(_T("/"), _T("\\"));
	if(!_strDir.IsEmpty())
	{
		_bIsRelative = (_strDir[0] != _T('\\'));
	}

	// FileTitle
	if(bIsFolderPath)
	{
		_strDir = CPath::AddBackSlash(_strDir);
		_strDir += szName;
		_strDir = CPath::AddBackSlash(_strDir);
	}
	else
	{
		_strFileTitle = szName;
	}

	// Get extension name (e.g.: "txt")
	if(IsFilePath())
	{
		_strExtName = szExt;
		_strExtName.Remove(_T('.'));
	}
}
/*---------------------------------------------------------------------------------------------
Name				:	IsLocalPath(void)
Purpose				:	Return TRUE if the path is a local path. FALSE for network and relative paths.
Parameters			:	None.
Return				:	Return TRUE if the path is a local path. FALSE for network and relative paths.
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
BOOL CPath::IsLocalPath()
{
	return !_strDrive.IsEmpty() && !_bIsRelative;
}
/*---------------------------------------------------------------------------------------------
Name				:	IsRelativePath(void)
Purpose				:	Return TRUE for relative paths.
Parameters			:	None.
Return				:	<RetReturn TRUE for relative paths.
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
BOOL CPath::IsRelativePath()
{
	return _bIsRelative;
}
/*---------------------------------------------------------------------------------------------
Name				:	IsFilePath(void)
Purpose				:	Return TRUE for paths including a file name.
Parameters			:	None.
Return				:	RetReturn TRUE for paths including a file name.
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
BOOL CPath::IsFilePath()
{
	return !_strFileTitle.IsEmpty();
}
/*---------------------------------------------------------------------------------------------
Name				:	ExistFile(void)
Purpose				:	Return TRUE if the file exists
Parameters			:	None.
Return				:	Return TRUE if the file exists
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
BOOL CPath::ExistFile()
{
	if (!IsFilePath()) return FALSE;

	if((_taccess(GetPath(), 0)) == -1)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}
/*---------------------------------------------------------------------------------------------
Name				:	ExistLocation(void)
Purpose				:	Return TRUE if the location exists
Parameters			:	None.
Return				:	Return TRUE if the location exists
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
BOOL CPath::ExistLocation()
{
	if((_taccess(GetFolderPath(), 0)) == -1)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}
/*---------------------------------------------------------------------------------------------
Name				:	GetPath(BOOL bOriginal = FALSE)
Purpose				:	Return the path contained in the object
Parameters			:
BOOL  bOriginal -	 If bOriginal is TRUE, the same string that was passed to SetPath or the constructor is returned.
Return				:	CString - Path Object
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CPath::GetPath(BOOL /*bAppendArgs*/, BOOL bOriginal)
{
	CString sPath;

	if (bOriginal)
		sPath = _strOriginalPath;
	else
		sPath = GetFolderPath() + GetFileName();

	return sPath;
}
/*---------------------------------------------------------------------------------------------
Name				:	GetDrive(void)
Purpose				:	Get drive string (empty for a network path) [e.g.: "c:"]
Parameters			:	None.
Return				:	CString - Return the drive latter
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CPath::GetDrive()
{
	return _strDrive;
}
int CPath::GetDirCount()
{
	FillDirArray();
	return((int)_arrDir.GetSize());
}
/*---------------------------------------------------------------------------------------------
Name				:	GetDir(int nIndex = -1)
Purpose				:	Get 0 based nIndex folder string 
Parameters			:
int  nIndex -	If nIndex = -1 the return string is the full dir string 
[e.g.: "\folder\subfolder\" or "\\pcname\folder\" for non-local]
Return				:	CString - Directory
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CPath::GetDir(int nIndex)
{
	if (nIndex < 0)
		return _strDir;
	else if (nIndex < GetDirCount())
	{
		FillDirArray();
		return _arrDir[nIndex];
	}

	return sCEmptyString;
}
/*---------------------------------------------------------------------------------------------
Name				:	GetFolderPath(void)
Purpose				:	File location or directory path [e.g.: "c:\folder\subfolder\"]
Parameters			:	None.
Return				:	CString -   File location or directory path
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString	CPath::GetFolderPath()
{
	return _strDrive + GetDir();
}
/*---------------------------------------------------------------------------------------------
Name				:	GetFileTitle(void)
Purpose				:	File title string (without extension) [e.g.: "file" for "..\file.ext"]
Parameters			:	None.
Return				:	CString - File title string (without extension)
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CPath::GetFileTitle()
{
	return _strFileTitle;
}
/*---------------------------------------------------------------------------------------------
Name				:	GetFileName(void)
Purpose				:	Filename = File title + extension [e.g.: "file.ext"]
Parameters			:	None.
Return				:	CString - Filename = File title + extension 
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CPath::GetFileName()
{
	return _strFileTitle + GetExtension();
}
/*---------------------------------------------------------------------------------------------
Name				:	GetExtension(void)
Purpose				:	Extension string (dot included) [e.g.: ".ext"]
Parameters			:	None.
Return				:	CString - Extension string (dot included)
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CPath::GetExtension()
{
	if (_strExtName.IsEmpty()) return sCEmptyString;

	return CString(".") + _strExtName;
}
CString CPath::GetExtName()
{
	return _strExtName.MakeLower();
}
BOOL CPath::GetFileSize(__int64 &nSize)
{
	BOOL bResult;

	bResult = FillFileInfoStruct();
	nSize = ((__int64)_fis.nFileSizeHigh * (__int64)MAXDWORD) + (__int64)_fis.nFileSizeLow;
	return bResult;
}

BOOL CPath::GetFileTime(CTime &time, DWORD dwType)
{
	BOOL bResult;
	FILETIME *pTime = NULL;

	bResult = FillFileInfoStruct();
	switch (dwType)
	{
	case FILE_CREATION:	pTime = &_fis.ftCreationTime;	break;
	case FILE_WRITE:	pTime = &_fis.ftLastWriteTime;	break;
	case FILE_ACCESS:	
	default:			pTime = &_fis.ftLastAccessTime;	break;
	}

	if (pTime != NULL) time = CTime(*pTime);
	return bResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Private methods

// This function must be called whenever _arrDir array is needed, since this
// method is the one which parses _strDir and fill _arrDir
void CPath::FillDirArray()
{
	if (_strDir.IsEmpty() || (_arrDir.GetSize() > 0)) return;

	int nFrom, nTo;

	// nFrom: 0 - relative / 1 - local / 2 - network
	nFrom = IsLocalPath() ? 1 : (IsRelativePath() ? 0 : 2);

	while ((nTo = _strDir.Find('\\', nFrom)) != -1)
	{
		_arrDir.Add(_strDir.Mid(nFrom, nTo - nFrom));
		nFrom = nTo + 1;
	}
}
CString CPath::AddBackSlash(LPCTSTR szFolderPath, BOOL bInverted)
{
	CString sResult(szFolderPath);
	int		nLastChar = sResult.GetLength() - 1;

	if(nLastChar >= 0)
	{
		if((sResult[nLastChar] != _T('\\')) && (sResult[nLastChar] != _T('/')))
		{
			sResult += bInverted ? _T('/') : _T('\\');
		}
	}
	return(sResult);
}
/*---------------------------------------------------------------------------------------------
Name				:	RemoveBackSlash(LPCTSTR szFolderPath)
Purpose				:	Removes a trailing back slash (or normal slash) if found at the end of szFolderPath

Parameters			:
LPCTSTR  szFolderPath - Source path
Return				:	CString - new path
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CString CPath::RemoveBackSlash(LPCTSTR szFolderPath)
{
	CString sResult(szFolderPath);
	int		nLastChar = sResult.GetLength() - 1;

	if(nLastChar >= 0)
	{
		if((sResult[nLastChar] == _T('\\')) || (sResult[nLastChar] == _T('/')))
		{
			sResult = sResult.Left(nLastChar);
		}
	}

	return(sResult);
}
/*---------------------------------------------------------------------------------------------
Name				:	operator LPCTSTR(void)
Purpose				:	Return a temporary character pointer to the path data. 
Parameters			:	None.
Return				:	CString -	Return a temporary character pointer to the path data. 
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
CPath::operator LPCTSTR ()
{
	_sLPCTSTRPath = GetPath();
	return _sLPCTSTRPath;
}
/*---------------------------------------------------------------------------------------------
Name				:	operator =(LPCTSTR szPath)
Purpose				:	The same as SetPath(szPath
Parameters			:
LPCTSTR  szPath - Add a path to Path obj
Return				:	CPath - CPath obj
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
const CPath& CPath::operator = (LPCTSTR szPath)
{
	SetPath(szPath);
	return *this;
}
/*---------------------------------------------------------------------------------------------
Name				:	operator =(CPath &ref)
Purpose				:	Make a copy of the ref object in the left one.
Parameters			:
CPath & ref -	file obj
Return				:	 CPath - new obj
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
const CPath& CPath::operator = (CPath &ref)
{
	_arrDir.RemoveAll();
	_arrDir.Copy(ref._arrDir);
	
	_bIsRelative = ref._bIsRelative;

	_fis = ref._fis;

	_strDir = ref._strDir;
	_strDrive = ref._strDrive;
	_strDriveLabel = ref._strDriveLabel;
	_strExtName = ref._strExtName;
	_strFileTitle = ref._strFileTitle;
	_strOriginalPath = ref._strOriginalPath;
	return *this;
}
BOOL CPath::FillFileInfoStruct()
{
	HANDLE	hFile;
	BOOL	bResult;

	::memset(&_fis, 0, sizeof(_fis));

	hFile = CreateFile(GetPath(), GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN |
		FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM, NULL);

	if (hFile == INVALID_HANDLE_VALUE) return FALSE;

	bResult = GetFileInformationByHandle(hFile, &_fis);

	CloseHandle(hFile);

	return bResult;
}
CString CPath::CastItoXBytes(double count,  UINT decimal)
{
	CString strBuffer;

	if (count < 1024.0)
		strBuffer.Format(_T("%.0f %s"), count, _T("Bytes"));
	else if (count < 1024000.0)
		strBuffer.Format(_T("%.*f %s"), decimal, count/1024.0, _T("KB"));
	else if (count < 1048576000.0)
		strBuffer.Format(_T("%.*f %s"), decimal, count/1048576.0, _T("MB"));
	else if (count < 1073741824000.0)
		strBuffer.Format(_T("%.*f %s"), decimal, count/1073741824.0, _T("GB"));
	else 
		strBuffer.Format(_T("%.*f %s"), decimal, count/1099511627776.0, _T("TB"));
	return strBuffer;
}
