#include <windows.h>
#include <stdio.h>


typedef struct _SYMINFO
{
	ULONG NextEntryDelta;
	ULONG SymOffset;
	char SymName[1];
} SYMINFO, *PSYMINFO;

int main(int argc, char** argv)
{
	if (argc < 2)
		return printf("usage: symdump sym-file\n");

	HANDLE hFile = CreateFile (argv[1], GENERIC_READ,
		FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	HANDLE hMapping = CreateFileMapping (
		hFile, 0, PAGE_READONLY, 0, 0, 0);

	PVOID pMap = MapViewOfFile (hMapping, FILE_MAP_READ,
		0, 0, 0);

	ULONG Size = GetFileSize (hFile, 0);
	SYMINFO *info = (SYMINFO*) ((PUCHAR)pMap + 4);

	do
	{
		char name[128];
		int len = info->NextEntryDelta - FIELD_OFFSET(SYMINFO, SymName);

		memcpy (name, info->SymName, len);
		name[len] = 0;

		printf("%08X %s\n", info->SymOffset, name);

		*(ULONG*)&info += info->NextEntryDelta;

		if (info->NextEntryDelta == 0)
			break;
	}
	while (TRUE);

	UnmapViewOfFile (pMap);
	CloseHandle (hMapping);
	CloseHandle (hFile);
	return 0;
}
