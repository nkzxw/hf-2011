#if !defined(AFX_STDAFX_H__8C802E54_A019_4C55_8DD6_CA0B02DCA0BF__INCLUDED_)
#define AFX_STDAFX_H__8C802E54_A019_4C55_8DD6_CA0B02DCA0BF__INCLUDED_

BOOL CreateDbgProc();
void SetDbgNamePath(char *szDbgName, char *szDbgPath);
char *GetErr();
void SetBreakPointInfo(DWORD BreakAddress, char *Comment);
BOOL AddSoftBreak();
void SetBreakNum(int BreakNum);
void SetOpCode(char *OpCode);
char *GetAsmStr();
BOOL GetIsPase();
BOOL AddHeadBreak();
BOOL SetMemBreakInfo(int BreakLends, char *Protect);
BOOL AddMemBreak();
void SetIsPase(BOOL isPase);
char *GetRegisterStr();
char *GetMemStr();
char *GetStackStr();
char *GetBreakStr();
BOOL OpCommend();
void SetPrProcess(char *Proprocess);
PTHREAD_START_ROUTINE GetOpCommend();
HMODULE GetR3DbgHandle();
HANDLE GetR3ProcHandle();



#endif //AFX_STDAFX_H__8C802E54_A019_4C55_8DD6_CA0B02DCA0BF__INCLUDED_