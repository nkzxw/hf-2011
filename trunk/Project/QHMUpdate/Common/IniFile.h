#ifndef _INI_FILE_H_
#define _INI_FILE_H_

#include <windows.h>

#define	MAX_SECTION	260			//Section最大长度   
#define	MAX_KEY	260				//KeyValues最大长度   
#define	MAX_ALLSECTIONS	128	//所有Section的最大长度   
#define	MAX_ALLKEYS	512		//所有KeyValue的最大长度

typedef struct _INI_SECTION
{
	char chSection[MAX_SECTION];
}INI_SECTION, *PINI_SECTION;

typedef struct _INI_KEY
{
	char chKey[MAX_KEY];
}INI_KEY, *PINI_KEY;

class CIniFile   
{
public:
	
	CIniFile();

	virtual ~CIniFile();
public:

	BOOL SetPath(LPCSTR strPath);
	BOOL SectionExist(LPCSTR strSection); 

	/*
	* Key 增,删,获得操作。
	*/ 
	DWORD GetKeyValue(LPCSTR strSection, LPCSTR strKey, LPSTR lpKeyValue, DWORD size);  
	BOOL SetKeyValue(LPCSTR strSection, LPCSTR strKey, LPCSTR strKeyValue); 
	BOOL DeleteKey(LPCSTR strSection, LPCSTR strKey); 

	/*
	* Section 增,删,获得操作。
	*/
	DWORD GetSectionValue (LPCTSTR lpSecName, LPTSTR lpReturnedString, DWORD nSize);
	BOOL SetSectionValue (LPCSTR lpSecName, LPCSTR lpString);
	BOOL DeleteSection(LPCSTR strSection); 

	PINI_SECTION GetAllSections(int *pNum);   
	int GetAllKeysOfSection(LPCSTR strSection,PINI_KEY pKey); 
	void DeleteAllSections(); 

private:
	LPCSTR m_strPath;
	INI_SECTION m_Sections[MAX_ALLSECTIONS];
}; 

#endif //_INI_FILE_H_