#ifndef _COMMON_H_
#define _COMMON_H_

#include <math.h>

#define FILE_COUNT 32
typedef struct _IOCP_FILE_INFO
{
	char version[32]; //更新版本
	char name[FILE_COUNT][MAX_PATH]; //更新文件的二维数组
	int size[FILE_COUNT]; //更新文件的大小
	WIN32_FILE_ATTRIBUTE_DATA data[FILE_COUNT]; //文件的属性信息
	int count; //更新文件的数量
	
	struct _IOCP_FILE_INFO ()
	{
		memset (version, 0, 32);
		for (int i = 0; i < FILE_COUNT; i++){
			memset (name[i], 0, MAX_PATH);
		}
		for (int j = 0; j < FILE_COUNT; j++){
			size[j] = 0;
		}

		memset (data, 0, sizeof (WIN32_FILE_ATTRIBUTE_DATA) * FILE_COUNT);

		count = 0;
	}
}IOCP_FILE_INFO, *PIOCP_FILE_INFO;

void 
OutputDebugPrintf(
	TCHAR *szFormat, ...
	)
{
	va_list args;
	va_start(args, szFormat);

	vprintf(szFormat, args );

	va_end(args);
}

void
GetCurrentPath (LPSTR Path)
{
	GetModuleFileName(NULL,Path,MAX_PATH);
	char *pch = Path; 
	while(strchr(pch,'\\'))
	{
		 pch = strchr(pch,'\\');
		 pch++;
	}		 
	*pch = 0;
}

BOOL  
Relative(
	double a, 
	double b)
{
	ULONG ua =  a * 100;
	ULONG ub =  b * 100;
 
	if (ua - ub){
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void trim (char *pch, char ch)
{
	bool bExits = true;
	while (bExits)
	{
		int len = strlen (pch);
		bExits = false;
		for (int i = 0; i < len; i++)
		{
			if (pch[i] == ch)
			{
				for (int j = i; j < len; j++)
				{
					if (j + 1 <= len) {
						pch[j] = pch[j + 1];
					}
				}
				pch[len - 1] = 0;
				bExits = true;
				break;
			}
		}
	}
}

PCHAR GetFileExtName (char *name)
{
	char *pch = strrchr (name, '.');
	if (strlen (pch) == 0)
		return pch;
	else 
		return ++pch;
}

#endif //_COMMON_H_