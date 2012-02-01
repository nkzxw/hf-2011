
#ifndef __ApiParam
#   define __ApiParam

    BOOL   MapSSFiles();
	VOID   UnmapSSFiles();
	CHAR*  GetApiParam(CHAR* szDll,CHAR* szApi,HANDLE hProc,VOID* pStack);
	
#   endif