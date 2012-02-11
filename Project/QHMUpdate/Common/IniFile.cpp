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
	memset (m_Sections, 0, MAX_ALLSECTIONS * sizeof (INI_SECTION));

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
	GetAllSections(&nSecNum);    
	for(int i=0; i<nSecNum; i++)    
	{    
		WritePrivateProfileString(m_Sections[i].chSection,   
								NULL,   
								NULL,   
								(LPCTSTR)m_strPath);          
	}      
}  

BOOL
CIniFile::SetSectionValue(
	LPCSTR lpSecName, 
	LPCSTR lpString)
{
	return WritePrivateProfileSection (lpSecName, 
									lpString, 
									m_strPath);
}


DWORD 
CIniFile::GetSectionValue (
	LPCTSTR lpSecName,
	LPTSTR lpReturnedString,
	DWORD nSize
	)
{
	return GetPrivateProfileSection(lpSecName,
							  lpReturnedString,
							  nSize,
							  m_strPath);
}


void TestIni (LPCSTR path)
{
#define BUF_SIZE 65536

	CIniFile IniFile;
	if ( FALSE == IniFile.SetPath (path)){
		return;
	}
	
	//Test Function GetAllSections;
	int num;
	PINI_SECTION pSection = IniFile.GetAllSections (&num);

	//Test Funciton GetAllKeysOfSection.
	for (int i = 0; i < num; i++)
	{
		char *pch = pSection[i].chSection;
		
		char buf[BUF_SIZE];
		memset (buf, 0, BUF_SIZE);
		BOOL bRet = IniFile.GetSectionValue (pch, buf, BUF_SIZE);
		
		INI_KEY keys[MAX_ALLKEYS];
		memset (keys, 0, sizeof (INI_KEY) * MAX_ALLKEYS);
		int iNum = IniFile.GetAllKeysOfSection (pch, keys);
	}

	//Test Function SetSectionValue 
	BOOL bRet = IniFile.SetSectionValue ("New Section", "2222");
	if (!bRet){
		printf ("Error = %d.\r\n", GetLastError ());
	}

	//Test Function DeleteSection
	bRet = IniFile.DeleteSection ("New Section");
	if (!bRet){
		printf ("Error = %d.\r\n", GetLastError ());
	}

}