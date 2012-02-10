#include "stdafx.h"   
#include "IniFile.h"   
   
CIniFile::CIniFile()   
{   
   
}   
   
CIniFile::~CIniFile()   
{   
   
}   

BOOL 
CIniFile::SetPath(
	LPCSTR strPath
	)   
{   
	 m_strPath = strPath;   

	 // 检查文件是否存在   
	 DWORD dwFlag = GetFileAttributes((LPCTSTR)m_strPath);   
	 if( 0xFFFFFFFF == dwFlag )   
		 return FALSE;   

	 // 检查是否是路径
	 if (FILE_ATTRIBUTE_DIRECTORY & dwFlag )   
		return FALSE;   

	 return TRUE;   
}   

BOOL 
CIniFile::SectionExist(
	LPCSTR strSection
	)   
{   
	TCHAR chSection[MAX_SECTION];   
	DWORD dwRetValue;   
	dwRetValue = GetPrivateProfileString((LPCTSTR)strSection,NULL,"",
							chSection, sizeof(chSection)/sizeof(TCHAR),   
							(LPCTSTR)m_strPath);   
	return (dwRetValue > 0);   
}   

   
DWORD  
CIniFile::GetKeyValue(
	LPCSTR strSection, 
	LPCSTR strKey,
	LPSTR lpKeyValue)   
{   
	return GetPrivateProfileString((LPCTSTR)strSection,   
					  (LPCTSTR)strKey,   
					   "",
					   lpKeyValue,   
					   strlen(lpKeyValue),   
					   (LPCTSTR)m_strPath);
}   
   
BOOL
CIniFile::SetKeyValue(
	LPCSTR strSection,
	LPCSTR strKey,
	LPCSTR strKeyValue)   
{   
	return WritePrivateProfileString((LPCTSTR)strSection,    
							  (LPCTSTR)strKey,    
							  (LPCTSTR)strKeyValue,    
							  (LPCTSTR)m_strPath);   
}   
   
BOOL
CIniFile::DeleteKey(
	LPCSTR strSection, 
	LPCSTR strKey
	)   
{   
	return WritePrivateProfileString((LPCTSTR)strSection,   
						  (LPCTSTR)strKey,    
						   NULL,                 
						  (LPCTSTR)m_strPath);      
}   
   
BOOL
CIniFile::DeleteSection(
	LPCSTR strSection)   
{   
	return WritePrivateProfileSection (strSection, 
									NULL, 
									m_strPath);
}   
   
PINI_SECTION 
CIniFile::GetAllSections(
	int *pNum)   
{   
#define BUF_SIZE 65536

	char buf[BUF_SIZE];   
	memset(buf, 0, BUF_SIZE);   
	GetPrivateProfileSectionNames(buf,BUF_SIZE,m_strPath);
	char *pch = buf;
	int i = 0;
	while (strlen (pch))
	{
		memcpy (m_Sections[i].chSection, pch, strlen(pch));
		pch += strlen (pch) + 1;
		i++;
	}

	*pNum = i;

	return m_Sections;
}   
   
int  
CIniFile::GetAllKeysOfSection(
	LPCSTR strSection,
	PINI_KEY pKey)   
{   
#define BUF_SIZE 65536

	int dwRetValue, iPos = 0;   
	char buf[BUF_SIZE];
	memset (buf, 0, BUF_SIZE);

	dwRetValue = GetPrivateProfileSection(strSection,   
									  buf,   
									  BUF_SIZE,   
									  m_strPath);   
	char *pch = buf;
	int i = 0;
	while (strlen(pch))
	{
		memcpy (pKey[i].chKey, pch, strlen (pch));
		pch += strlen (pch) + 1;
		i++;
	}
	
	return i;
}   
   
void 
CIniFile::DeleteAllSections()   
{   
	int nSecNum;    
	LPCSTR strArrSection;    
	GetAllSections(&nSecNum);    
	for(int i=0; i<nSecNum; i++)    
	{    
		WritePrivateProfileString((LPCTSTR)strArrSection[i],   
								NULL,   
								NULL,   
								(LPCTSTR)m_strPath);          
	}      
}  

BOOL
CIniFile::SetSectionValue(
	LPCSTR lpAppName, 
	LPCSTR lpString)
{
	return WritePrivateProfileSection (lpAppName, 
									lpString, 
									m_strPath);
}


DWORD 
CIniFile::GetSectionValue (
	LPCTSTR lpAppName,
	LPTSTR lpReturnedString,
	DWORD nSize,
	LPCTSTR lpFileName
	)
{
	return GetPrivateProfileSection(lpAppName,
							  lpReturnedString,
							  nSize,
							  lpFileName);
}
