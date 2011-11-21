// Shell.h
//
// copyright (c) 2002 Passeck Technology
// 
// all right reserved.
//
// author: tony
// 
// create date: 2002/11/05
//
// email: xstudio@xfilt.com
//
//
//

#ifndef _SHELL_H
#define _SHELL_H

#define SHELL_INSTALL_KEY		"SOFTWARE\\FilSecLab"
#define SHELL_REG_PROGRAM		"RegProgram"
#define SHELL_UPDATE_PROGRAM	"UpdateProgram"
#define SHELL_USER_REG_PROGRAM  "UserRegProgram"

typedef enum{
	SHELL_ERROR_SUCCESS = 0,			// �ɹ�
	SHELL_ERROR_CANT_GET_PATH,			// ���ܴ�ע���õ�����·��
	SHELL_ERROR_CANT_RUN,				// �������г��򣬳��򲻴��ڻ��߱���
	SHELL_ERROR_INVALID_PROGRAM_TYPE,	// ��Ч������
} SHELL_ERROR;

typedef enum{
	PROGRAM_REGISTER,
	PROGRAM_UPDATE,
	PROGRAM_USERREG,
} PROGRAM_TYPE;

class CShell
{
public:
	CShell();
	virtual ~CShell();
public:
	int RunProgram(BYTE bProgramType, BOOL bIsWait = FALSE);
	LPCTSTR GetCmdLine(){return m_CmdLine;}
	LPCTSTR GetProgram(){return m_Program;}
private:
	char m_Program[MAX_PATH];
	char m_CmdLine[MAX_PATH];
public:
	static BOOL GetUpdateProgram(char* szProgram, int nMaxLen);
	static BOOL GetRegProgram(char* szProgram, int nMaxLen);
	static BOOL GetUserReg(char* szProgram, int nMaxLen);
	static HANDLE Pg_CreateProcess(LPCTSTR szCommandLine, BOOL bIsWait = FALSE);
	static BOOL ReadReg(
		TCHAR	*sKey,
		void	*pBuffer,
		DWORD	dwBufSize, 
		HKEY	hkey	 = HKEY_LOCAL_MACHINE, 
		TCHAR	*sSubKey = SHELL_INSTALL_KEY,
		DWORD	ulType	 = REG_SZ
		);
};

#endif //_SHELL_H