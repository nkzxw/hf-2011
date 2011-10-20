#include <windows.h>

typedef ULONG ULONG_PTR, *PULONG_PTR;
#include <dbghelp.h>
#include <stdio.h>

BOOL CALLBACK EnumSymProc( 
    PSYMBOL_INFO pSymInfo,   
    ULONG SymbolSize,      
    PVOID UserContext)
{
    printf("[%d] type %d %08X %4u %s\n", 
           pSymInfo->Index, pSymInfo->TypeIndex, pSymInfo->Address, SymbolSize, pSymInfo->Name);
    return TRUE;
}

HANDLE hFile;

typedef struct _SYMINFO
{
	ULONG NextEntryDelta;
	ULONG SymOffset;
	char SymName[1];
} SYMINFO, *PSYMINFO;

ULONG Base = 0x20000000;
ULONG Size = 0;
ULONG LastLen = 0;
ULONG wr;

BOOL _stdcall Callback(
	PCSTR SymbolName,
	ULONG Address,
	ULONG Size,
	PVOID
	)
{
	printf("%08X %4u %s\n", Address, Size, SymbolName);

	char buffer[512] = {0};
	SYMINFO *info = (SYMINFO*) buffer;
	int SymLen = strlen(SymbolName);

	info->NextEntryDelta = FIELD_OFFSET (SYMINFO, SymName) + SymLen;
	info->SymOffset = Address - Base;
	strcpy (info->SymName, SymbolName);

	WriteFile (hFile, info, info->NextEntryDelta, &wr, 0);

	LastLen = info->NextEntryDelta;
	Size += LastLen;

	return TRUE;
}

ULONG QueryExeTimeStamp (char *exe, BOOLEAN InSystem)
{
	char fullname[512] = "";
	ULONG CapturedTimeStamp = 0;

	if (InSystem)
	{
		GetSystemDirectory (fullname, sizeof(fullname));
		strcat (fullname, "\\");
	}
	strcat (fullname, exe);

	HANDLE hexe = CreateFile (fullname, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hexe == INVALID_HANDLE_VALUE)
	{
		printf("not found in spcified path, searching system\n");
		char *relname = strrchr (exe, '\\');
		if (relname)
			relname++;
		else
			relname = exe;
		return QueryExeTimeStamp (relname, TRUE);
	}
	else
	{
		HANDLE hMapping = CreateFileMapping (hexe, 0, PAGE_READONLY, 0, 0, 0);
		if (hMapping)
		{
			LPVOID pMap = MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, 0);

			if (pMap)
			{
				PIMAGE_DOS_HEADER doshdr = (PIMAGE_DOS_HEADER) pMap;
				PIMAGE_NT_HEADERS nthdrs = (PIMAGE_NT_HEADERS)((PUCHAR)pMap + doshdr->e_lfanew);

				CapturedTimeStamp = nthdrs->FileHeader.TimeDateStamp;

				UnmapViewOfFile (pMap);
			}

			CloseHandle (hMapping);
		}

		CloseHandle (hexe);
	}

	return CapturedTimeStamp;
}


int main(int argc, char **argv)
{
	if (argc < 3)
		return printf ("usage: symconv exe-name sym-name local-symbol-storage\n");

	char *imagename = argv[1];
	char *storage = argv[3];

	hFile = CreateFile (argv[2], GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

	ULONG time = QueryExeTimeStamp(argv[1], FALSE);
	WriteFile (hFile, &time, sizeof(time), &wr, 0);

	printf("Initializing\n");

	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

	if(!SymInitialize (GetCurrentProcess(), storage, FALSE))
		return printf("Initialization failed\n");

	if (SymLoadModule (GetCurrentProcess(),
		NULL,
		imagename,
		0,
		Base,
		0))
	{
		printf("Module loaded\n");

		if(!SymEnumerateSymbols (GetCurrentProcess(),
			Base, Callback, 0))
		{
			return printf("could not enumerate symbols\n");
		}

		SetFilePointer (hFile, Size-LastLen, NULL, FILE_BEGIN);

		WriteFile (hFile, "\0\0\0\0", 4, &wr, 0);

		printf("Completed\n");

		return 0;
	}
	else
	{
		printf("SymLoadModule failed\n");
	}

	return 0;
}