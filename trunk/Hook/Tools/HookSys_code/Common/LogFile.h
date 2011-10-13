//---------------------------------------------------------------------------
//
// LogFile.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      
//				
// DESCRIPTION: Common utilities. 
//              Provides an interface for logging facilities
// 
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------
#if !defined(_LOGFILE_H_)
#define _LOGFILE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <share.h>
#include <stdio.h>
//---------------------------------------------------------------------------
//
// class CLogFile 
//
//---------------------------------------------------------------------------
class CLogFile
{
public:
	CLogFile(BOOL bTraceEnabled):
	  m_bTraceEnabled(bTraceEnabled)
	{
		m_hMutex = ::CreateMutex(
			NULL, 
			FALSE, 
			"{45428D53-A5DB-4168-BD3C-658419B60279}"
			); 
	}
	virtual ~CLogFile()
	{
		::CloseHandle(m_hMutex);
	}

	void InitializeFileName(char* pszFileName)
	{
		strcpy(m_szFileName, pszFileName);
	}

	void DoLogMessage(char* pszMessage)
	{
		::WaitForSingleObject(m_hMutex, INFINITE);
		__try
		{
			if (m_bTraceEnabled)
			{
				FILE* pOutFile;
				char  szFlags[2];
				strcpy(szFlags, "a");

				pOutFile = _fsopen(m_szFileName, szFlags, _SH_DENYNO);

				if (pOutFile != NULL)
				{
					char szPrintMessage[MAX_PATH];
					fseek(pOutFile, 0L, SEEK_END);
					sprintf(szPrintMessage, "%s\n", pszMessage);
					fputs(szPrintMessage, pOutFile);            
					fflush(pOutFile);

					fclose(pOutFile);
				} // if
			} // if
		}
		__finally
		{
			::ReleaseMutex(m_hMutex);
		}
	}
	
private:
	//
	// Determines whether to use a log file management
	//
	BOOL             m_bTraceEnabled;
	//
	// Name of the log file
	//
	char             m_szFileName[MAX_PATH];
	//
	// Handle to the mutex used as guard
	//
	HANDLE           m_hMutex;
};

#endif // !defined(_LOGFILE_H_)

//--------------------- End of the file -------------------------------------
