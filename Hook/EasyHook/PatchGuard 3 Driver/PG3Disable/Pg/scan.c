/*
Released under MIT License

Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Visit http://www.codeplex.com/easyhook for more information.
*/

#include "EasyHookDriver.h"

#ifndef _M_X64
	#error "This driver part is intended for 64-bit builds only."
#endif


/************************************************************************
*************************************** PgDumpFingerprints
*************************************************************************

Description:

	This will work equally to PgDisablePatchGuard, just that it does
	only output the addresses that will be patched.

*/
NTSTATUS PgDumpFingerprints()
{
	ULONG					Index;
	CHAR					LogEntryText[200];
	NTSTATUS				Result = STATUS_SUCCESS;
	HANDLE					hLogFile;
	UNICODE_STRING			LogFileName;
	OBJECT_ATTRIBUTES		ObjAttr;
	IO_STATUS_BLOCK			IOStatus;
	ULONG					LogEntryTextLen;
	ULONG					iPatch;
	PMEMORY_PATCH_INFO		Patch;
	MEMORY_PATCH_INFO*		PatchArray = (MEMORY_PATCH_INFO*)ExAllocatePool(PagedPool, sizeof(MEMORY_PATCH_INFO) * 9);
	CHAR*					PatchToStr[9] = 
	{
		"VistaSp1:KiTimerListExpire",
		"VistaSp1:KiRetireDpcList",
		"VistaSp0:KiTimerListExpire_01",
		"VistaSp0:KiTimerListExpire_02",
		"VistaSp0:KiRetireDpcList",
		"VistaAll:KiCustomAccessRoutine",
		"VistaAll:Fingerprint",
		"VistaAll:ExpWorkerThread",
		"VistaAll:ExpWorkerThread",
	};

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);


	/*
		Open log file...
	*/
	RtlInitUnicodeString(&LogFileName, L"\\??\\C:\\patchguard.log");

	InitializeObjectAttributes(
		&ObjAttr, 
		&LogFileName, 
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL, NULL)

	if(!NT_SUCCESS(Result = ZwCreateFile(
			&hLogFile,
			GENERIC_WRITE,
			&ObjAttr,
			&IOStatus,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
			NULL, 0)))
	{
		KdPrint(("\r\n" "ERROR: Unable to open file \"\\??\\C:\\patchguard.log\". (NTSTATUS: 0x%p)\r\n", (void*)Result));

		return Result;
	}

	__try
	{
		/*
			Enumerate PatchGuard fingerprint locations...
		*/
		PatchArray[0] = VistaSp1_KiTimerListExpirePatch();
		PatchArray[1] = VistaSp1_KiRetireDpcListPatch();
		PatchArray[2] = VistaSp0_KiTimerListExpire_01_Patch();
		PatchArray[3] = VistaSp0_KiTimerListExpire_02_Patch();
		PatchArray[4] = VistaSp0_KiRetireDpcListPatch();
		PatchArray[5] = VistaAll_KiCustomAccessRoutinePatch();
		PatchArray[6] = VistaAll_FingerprintPatch(FALSE);
		PatchArray[7] = VistaSp0_ExpWorkerThreadPatch();
		PatchArray[8] = VistaSp1_ExpWorkerThreadPatch();

		if(!NT_SUCCESS(Result = PgProcessMemoryPatchList(PatchArray, 9)))
			return Result;

		for(iPatch = 0; iPatch < 9; iPatch++)
		{
			Patch = &PatchArray[iPatch];

			for(Index = 0; Index < Patch->LocCount; Index++)
			{
				LogEntryTextLen = _snprintf(
					LogEntryText, 
					sizeof(LogEntryText) - 1, 
					"<fingerprint patch=\"%s\">0x%p</fingerprint>\r\n",
					PatchToStr[iPatch],
					Patch->LocArray[Index]);

				if(NT_SUCCESS(Result))
				{
					Result = ZwWriteFile(
						hLogFile,
						NULL, NULL, NULL,
						&IOStatus,
						LogEntryText,
						LogEntryTextLen,
						NULL, NULL);
				}
			}
		}
	}
	__finally
	{
		ZwClose(hLogFile);

		ExFreePool(PatchArray);
	}

	return Result;
}
