#if !defined(AFX_UNTIL_H_INCLUDED)
#define AFX_UNTIL_H_INCLUDED
#include "RegEditEx.h"
typedef struct 
{
	unsigned ( __stdcall *start_address )( void * );
	void	*arglist;
	bool	bInteractive; // �Ƿ�֧�ֽ�������
	HANDLE	hEventTransferArg;
}THREAD_ARGLIST, *LPTHREAD_ARGLIST;

unsigned int __stdcall ThreadLoader(LPVOID param);

HANDLE MyCreateThread (LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
					   SIZE_T dwStackSize,                       // initial stack size
					   LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
					   LPVOID lpParameter,                       // thread argument
					   DWORD dwCreationFlags,                    // creation option
  LPDWORD lpThreadId, bool bInteractive = false);
	
DWORD GetProcessID(LPCTSTR lpProcessName);
char *GetLogUserXP();
char *GetLogUser2K();
char *GetCurrentLoginUser();

bool SwitchInputDesktop();

BOOL SelectHDESK(HDESK new_desktop);
BOOL SelectDesktop(char *name);
BOOL SimulateCtrlAltDel();
bool http_get(LPCTSTR szURL, LPCTSTR szFileName);
#endif // !defined(AFX_UNTIL_H_INCLUDED)