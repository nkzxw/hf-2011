#ifndef _PEHANDLER_H_
#define _PEHANDLER_H_

// 判断 PE 文件格式是否是无效的
__forceinline BOOL IsInvalidPe(LPVOID pBase)
{
	if (NULL == pBase)
		return TRUE;

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;
	if (IMAGE_DOS_SIGNATURE != pDosHeader->e_magic)
		return TRUE;

	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((DWORD)pBase + pDosHeader->e_lfanew);
	if (IMAGE_NT_SIGNATURE != pNtHeaders->Signature)
		return TRUE;

	return FALSE;
}

__forceinline LPVOID MapPeFile(LPCWSTR pwzFile, PLARGE_INTEGER lpFileSize)
{
	if (StrSafeLen(pwzFile) <= 0) // 如果传入的文件路径为空
		return NULL;

	HANDLE hFile = CreateFileW(pwzFile,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if (INVALID_HANDLE_VALUE == hFile)
		return NULL;

	GetFileSizeEx(hFile, lpFileSize);
	HANDLE hMapFile = CreateFileMappingW(hFile,
							NULL,
							PAGE_READONLY,
							0,
							0,
							NULL);

	CloseHandle(hFile);

	if (NULL == hMapFile)
		return NULL;

	LPVOID pRet = MapViewOfFile(hMapFile,
						FILE_MAP_READ,
						0,
						0,
						0);

	CloseHandle(hMapFile);

	return pRet;
}

__forceinline void UnmapPeFile(LPVOID pBase)
{
	if (NULL != pBase)
		UnmapViewOfFile(pBase);
}

__forceinline BOOL GetPeSignOffsetAndSize(LPVOID pBase, LPDWORD lpOffset, LPDWORD lpSize)
{
	if (NULL == pBase || NULL == lpOffset || NULL == lpSize)
		return FALSE;

	*lpOffset = *lpSize = 0;

	if (IsInvalidPe(pBase))
		return FALSE;

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((DWORD)pBase + pDosHeader->e_lfanew);
	*lpOffset = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress;
	*lpSize = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size;

	if (0 == *lpOffset || 0 == *lpSize)
		return FALSE;

	return TRUE;
}

__forceinline BOOL RemovePeSign(LPVOID pBase, PLARGE_INTEGER lpFileSize, LPCWSTR pwzNewFile)
{
	BOOL bRet = FALSE;

	if (NULL == pBase || NULL == lpFileSize)
		return FALSE;

	if (IsInvalidPe(pBase))
		return FALSE;

	DWORD dwOffset;
	DWORD dwSize;

	GetPeSignOffsetAndSize(pBase, &dwOffset, &dwSize);

	if (0 == dwOffset || 0 == dwSize)
		return FALSE;

	LPVOID pNewBase = VirtualAlloc(NULL, lpFileSize->LowPart, MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pNewBase)
		return FALSE;

	DWORD dwRealFileSize = lpFileSize->LowPart - dwSize;
	memcpy(pNewBase, pBase, dwRealFileSize);
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pNewBase;
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((DWORD)pNewBase + pDosHeader->e_lfanew);
	pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0;
	pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size = 0;
	pNtHeaders->OptionalHeader.CheckSum = 0;
	// 重新计算校检和
	DWORD dwHeaderSum, dwCheckSum;
	CheckSumMappedFile(pNewBase, dwRealFileSize, &dwHeaderSum, &dwCheckSum);
	pNtHeaders->OptionalHeader.CheckSum = dwCheckSum;

	HANDLE hFile = CreateFile(pwzNewFile,
						GENERIC_WRITE,
						FILE_SHARE_READ,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if (INVALID_HANDLE_VALUE == hFile)
		goto _exit;

	DWORD dwBytes;
	bRet = WriteFile(hFile, pNewBase, dwRealFileSize, &dwBytes, NULL);
	CloseHandle(hFile);
_exit:
	VirtualFree(pNewBase, 0, MEM_RELEASE);

	return bRet;
}

#endif // _PEHANDLER_H_
