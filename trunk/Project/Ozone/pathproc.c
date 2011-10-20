/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		pathproc.c
 *
 * Abstract:
 *
 *		This module implements various pathname handling routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 19-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "pathproc.h"
#include "procname.h"
#include "policy.h"
#include "learn.h"
#include "log.h"


/*
 * ResolveFilename() XXX rewrite
 *
 * Description:
 *		Get canonical name for a file by resolving symbolic links.
 *
 * Parameters:
 *		szFileName - filename to resolve.
 *		szResult - output buffer.
 *		szResultSize - size of an output buffer.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
ResolveFilename(IN PCHAR szFileName, OUT PCHAR szResult, IN USHORT szResultSize)
{
	CHAR				*p, c;
	OBJECT_ATTRIBUTES	oa;
	ANSI_STRING			FileNameAnsi;
	UNICODE_STRING		FileNameUnicode;
	HANDLE				hLink;
	NTSTATUS			rc;
	int					NumberOfLinks = 0;
	WCHAR				buffer[chMAX_PATH];
	CHAR				buffer2[chMAX_PATH];


restart:

	*szResult = '\0';

	if (szFileName[0] == '\\' && szFileName[1] == '\\')
		szFileName++;

	/* move to the end of the object name */
	for (p = szFileName; *p != '\0'; p++)
		;

	/* process the object name from end to the beginning */
	while (p != szFileName)
	{
		/* find the last slash */
		if (*p != '\\' && *p != '\0')
		{
			p--;
			continue;
		}

		c = *p;
		*p = '\0';


		RtlInitAnsiString(&FileNameAnsi, szFileName);
		RtlAnsiStringToUnicodeString(&FileNameUnicode, &FileNameAnsi, TRUE);

		InitializeObjectAttributes(&oa, &FileNameUnicode, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

		rc = ZwOpenSymbolicLinkObject(&hLink, GENERIC_READ, &oa);

		if (NT_SUCCESS(rc))
		{
			UNICODE_STRING	target;
			ANSI_STRING		targeta;


			target.Buffer = buffer;
			target.MaximumLength = bMAX_PATH;
			target.Length = 0;

			rc = ZwQuerySymbolicLinkObject(hLink, &target, NULL);

			ZwClose(hLink);


			if (NT_SUCCESS(rc))
			{
				targeta.Length = 0;
				targeta.MaximumLength = szResultSize;
				targeta.Buffer = szResult;

				RtlUnicodeStringToAnsiString(&targeta, &target, FALSE);
				targeta.Buffer[targeta.Length] = '\0';

//XXX				szResultSize -= targeta.Length;

				RtlFreeUnicodeString(&FileNameUnicode);
				*p = c;

//XXX can we have circular links?
				if (NumberOfLinks++ < MAX_NUMBER_OF_LINKS)
				{
					strncat(szResult, p, szResultSize);

//					if (NumberOfLinks > 1)
//						LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("ResolveFilename: NumberOfLinks=%d. Resolved %s to %s. Restarting.\n", NumberOfLinks, szFileName, szResult));

					/*
					 * switch szFileName to a different buffer. we cannot reuse szFileName buffer
					 * since the resolved link might end up being longer than the original buffer
					 */

					szFileName = (PCHAR) buffer2;
					strcpy(szFileName, szResult);

					goto restart;
				}

				LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("ResolveFilename: NumberOfLinks=%d. bailing out. %s\n", NumberOfLinks, szResult));

				break;
			}
		}

		RtlFreeUnicodeString(&FileNameUnicode);

		*p-- = c;
	}

	strncat(szResult, p, szResultSize);


//	LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("ResolveFilename: name=%s number of links=%d\n", szResult, NumberOfLinks));


	return TRUE;
}



BOOLEAN
ResolveFilenameW(IN PUNICODE_STRING szFileName, OUT PCHAR szResult, IN USHORT szResultSize)
{
	WCHAR				*p, c;
	OBJECT_ATTRIBUTES	oa;
	ANSI_STRING			FileNameAnsi;
	UNICODE_STRING		FileNameUnicode;
	HANDLE				hLink;
	NTSTATUS			rc;
	int					NumberOfLinks = 0;
	WCHAR				buffer[chMAX_PATH];
	USHORT				OriginalLength;
	UNICODE_STRING		target;
	ANSI_STRING			targeta;



	ASSERT(szFileName);
	ASSERT(szResult);

	OriginalLength = szFileName->Length;


restart:

	*szResult = '\0';

	/* move to the end of the object name */
	p = (PWCHAR) ((PCHAR)szFileName->Buffer + szFileName->Length);

	/* process the object name from end to the beginning */
	while (p != szFileName->Buffer)
	{
		/* find the last slash */
//		p = wcsrchr(p, L'\\');
/*
		if (p == NULL)
		{
			p = szFileName->Buffer;
			break;
		}
*/
		if (*p != L'\\' && *p != L'\0')
		{
			p--;
			continue;
		}


		c = *p;
		*p = L'\0';
//		szFileName->Length = OriginalLength - (p - szFileName->Buffer);


//		RtlInitAnsiString(&FileNameAnsi, szFileName);
//		RtlAnsiStringToUnicodeString(&FileNameUnicode, &FileNameAnsi, TRUE);
//		InitializeObjectAttributes(&oa, &FileNameUnicode, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

		InitializeObjectAttributes(&oa, szFileName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

		rc = ZwOpenSymbolicLinkObject(&hLink, GENERIC_READ, &oa);

		if (! NT_SUCCESS(rc))
		{
			*p-- = c;
			continue;
		}

		target.Buffer = buffer;
		target.MaximumLength = bMAX_PATH;
		target.Length = 0;

		rc = ZwQuerySymbolicLinkObject(hLink, &target, NULL);

		ZwClose(hLink);


		if (! NT_SUCCESS(rc))
		{
			*p-- = c;
			continue;
		}


		*p = c;

		
//XXX can we have circular links?
		if (NumberOfLinks++ < MAX_NUMBER_OF_LINKS)
		{
			wcscat(buffer, p);
			RtlInitUnicodeString(szFileName, buffer);

			goto restart;
		}

		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("ResolveFilename: NumberOfLinks=%d. bailing out. %s\n", NumberOfLinks, szResult));

		break;
	}


//	wcscat(szResult, p);


//	LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("ResolveFilename: name=%s number of links=%d\n", szResult, NumberOfLinks));


	return TRUE;
}



/*
 * GetPathFromOA()
 *
 * Description:
 *		Resolve an object handle to an object name.
 *
 * Parameters:
 *		ObjectAttributes - opaque structure describing an object handle.
 *		OutBuffer - output buffer where an object name will be saved to.
 *		OutBufferSize - size of an output buffer.
 *		ResolveLinks - do symbolic links need to be resolved?
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

#define	FINISH_GetPathFromOA(msg)										\
	do {																\
		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, msg);					\
		return FALSE;													\
	} while(0)

BOOLEAN
GetPathFromOA(IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PCHAR OutBuffer, IN USHORT OutBufferSize, IN BOOLEAN ResolveLinks)
{
	NTSTATUS					rc;
	PVOID						Object = NULL;
	ULONG						len;
	CHAR						Buffer[sizeof(OBJECT_NAME_INFORMATION) + bMAX_PATH];
	POBJECT_NAME_INFORMATION	pONI = (POBJECT_NAME_INFORMATION) Buffer;
	BOOLEAN						ret = FALSE;
    UNICODE_STRING				ObjectName;

	PUNICODE_STRING				pusFilename = NULL;
	ANSI_STRING					name;


	if (! ARGUMENT_PRESENT(ObjectAttributes) || OutBuffer == NULL)

		return(FALSE);


	try
	{
		if (KeGetPreviousMode() != KernelMode)

			ProbeForRead(ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), sizeof(ULONG));


		if (ObjectAttributes->Length != sizeof(OBJECT_ATTRIBUTES))

			FINISH_GetPathFromOA(("GetPathFromOA: Invalid ObjectAttributes length %d\n", ObjectAttributes->Length));


		if (! ARGUMENT_PRESENT(ObjectAttributes->ObjectName) )

			return FALSE;


		if (KeGetPreviousMode() != KernelMode)
		{
			ProbeForRead(ObjectAttributes->ObjectName, sizeof(UNICODE_STRING), sizeof(ULONG));

			ObjectName = ProbeAndReadUnicodeString(ObjectAttributes->ObjectName);
		}
		else
		{
			ObjectName = *ObjectAttributes->ObjectName;
		}


		if (ObjectName.Length == 0)

			return FALSE;


		if (((ObjectName.Length & (sizeof(WCHAR) - 1)) != 0) ||
			(ObjectName.Length > bMAX_PATH - sizeof(WCHAR)) )

			FINISH_GetPathFromOA(("GetPathFromOA: invalid wchar string length = %d\n", ObjectName.Length));


		if (KeGetPreviousMode() != KernelMode)

			ProbeForRead(ObjectName.Buffer, ObjectName.Length, sizeof(WCHAR));
	}

	except(EXCEPTION_EXECUTE_HANDLER)
	{
		NTSTATUS status = GetExceptionCode();

		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("GetPathFromOA(): caught an exception. status = 0x%x\n", status));

		return FALSE;
	}


	pusFilename = &ObjectName;


	/*
	 * is the filename referenced in relation to some directory?
	 * if so, append the filename to a specified directory name
	 */

	if (ARGUMENT_PRESENT(ObjectAttributes->RootDirectory))
	{
		if (! NT_SUCCESS( ObReferenceObjectByHandle(ObjectAttributes->RootDirectory, 0, 0,
													KernelMode, &Object, NULL) ))
		{
			FINISH_GetPathFromOA(("GetPathFromOA(): ObReferenceObjectByHandle() failed. Object = %x\n", Object));
		}

		if (Object == NULL)
		{
			FINISH_GetPathFromOA(("GetPathFromOA(): Object = NULL\n"));
		}


		if (! NT_SUCCESS( ObQueryNameString(Object, pONI, bMAX_PATH, &len) ))
		{
			ObDereferenceObject(Object);
			FINISH_GetPathFromOA(("GetPathFromOA(): ObQueryNameString() failed\n"));
		}


		ObDereferenceObject(Object);
		Object = NULL;


		/* extracted directory name */
		pusFilename = &pONI->Name;


		/* is the directory name too long? */

		if (pusFilename->Length >= bMAX_PATH - sizeof(WCHAR))
			FINISH_GetPathFromOA(("GetPathFromOA(): directory name is too long\n"));


		/*
		 * pusFilename points to a buffer of MAX_PATH size, ObQueryNameString() sets MaximumLength to the length
		 * of the directory name, we need to reset this back to MAX_PATH to be able to append a filename
		 * (reusing the same buffer)
		 */
		pusFilename->MaximumLength = bMAX_PATH;


		if (pusFilename->Buffer[ (pusFilename->Length / sizeof(WCHAR)) - 1 ] != L'\\')
		{
			pusFilename->Buffer[ pusFilename->Length / sizeof(WCHAR) ] = L'\\';
			pusFilename->Length += sizeof(WCHAR);
		}

		if (RtlAppendUnicodeStringToString(pusFilename, ObjectAttributes->ObjectName) == STATUS_BUFFER_TOO_SMALL)
		{
			LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("GetPathFromOA: 1 %S\n", pusFilename->Buffer));
			LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("GetPathFromOA: 2 %S\n", ObjectAttributes->ObjectName->Buffer));
			FINISH_GetPathFromOA(("GetPathFromOA(): RtlAppendUnicodeStringToString() = STATUS_BUFFER_TOO_SMALL\n"));
		}
	}


	if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&name, pusFilename, TRUE)))
	{	
		if (ResolveLinks == TRUE)
		{
			ret = ResolveFilename(name.Buffer, OutBuffer, OutBufferSize);
		}
		else
		{
			if (name.Length >= OutBufferSize - 1)
			{
				LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("GetPathFromOA: Pathname too long %d\n", name.Length));

				OutBuffer[0] = 0;

				ret = FALSE;
			}
			else
			{
				strcpy(OutBuffer, name.Buffer);

				ret = TRUE;
			}
		}

		RtlFreeAnsiString(&name);
	}


//	LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("%d GetPathFromOA: %s (%S)\n", (ULONG) PsGetCurrentProcessId(), OutBuffer, pusFilename->Buffer));


	return ret;
}




BOOLEAN
GetPathFromOAW(IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PCHAR OutBuffer, IN USHORT OutBufferSize, IN BOOLEAN ResolveLinks)
{
	NTSTATUS					rc;
	PVOID						Object = NULL;
	ULONG						len;
	CHAR						Buffer[sizeof(OBJECT_NAME_INFORMATION) + bMAX_PATH];
	POBJECT_NAME_INFORMATION	pONI = (POBJECT_NAME_INFORMATION) Buffer;
	BOOLEAN						ret = FALSE;
    UNICODE_STRING				ObjectName;

	PUNICODE_STRING				pusFilename = NULL;
	ANSI_STRING					name;


	if (! ARGUMENT_PRESENT(ObjectAttributes) || OutBuffer == NULL)

		return(FALSE);


	try
	{
		if (KeGetPreviousMode() != KernelMode)

			ProbeForRead(ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), sizeof(ULONG));


		if (ObjectAttributes->Length != sizeof(OBJECT_ATTRIBUTES))

			FINISH_GetPathFromOA(("GetPathFromOA: Invalid ObjectAttributes length %d\n", ObjectAttributes->Length));


		if (! ARGUMENT_PRESENT(ObjectAttributes->ObjectName) )

			return FALSE;


		if (KeGetPreviousMode() != KernelMode)
		{
			ProbeForRead(ObjectAttributes->ObjectName, sizeof(UNICODE_STRING), sizeof(ULONG));

			ObjectName = ProbeAndReadUnicodeString(ObjectAttributes->ObjectName);
		}
		else
		{
			ObjectName = *ObjectAttributes->ObjectName;
		}


		if (ObjectName.Length == 0)

			return FALSE;


		if (((ObjectName.Length & (sizeof(WCHAR) - 1)) != 0) ||
			(ObjectName.Length > bMAX_PATH - sizeof(WCHAR)) )

			FINISH_GetPathFromOA(("GetPathFromOA: invalid wchar string length = %d\n", ObjectName.Length));


		if (KeGetPreviousMode() != KernelMode)

			ProbeForRead(ObjectName.Buffer, ObjectName.Length, sizeof(WCHAR));
	}

	except(EXCEPTION_EXECUTE_HANDLER)
	{
		NTSTATUS status = GetExceptionCode();

		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("GetPathFromOA(): caught an exception. status = 0x%x\n", status));

		return FALSE;
	}


	pusFilename = &ObjectName;


	/*
	 * is the filename referenced in relation to some directory?
	 * if so, append the filename to a specified directory name
	 */

	if (ARGUMENT_PRESENT(ObjectAttributes->RootDirectory))
	{
		if (! NT_SUCCESS( ObReferenceObjectByHandle(ObjectAttributes->RootDirectory, 0, 0,
													KernelMode, &Object, NULL) ))
		{
			ObDereferenceObject(Object);
			FINISH_GetPathFromOA(("GetPathFromOA(): ObReferenceObjectByHandle() failed\n"));
		}


		if (Object == NULL)
		{
			ObDereferenceObject(Object);
			FINISH_GetPathFromOA(("GetPathFromOA(): Object = NULL\n"));
		}


		if (! NT_SUCCESS( ObQueryNameString(Object, pONI, bMAX_PATH, &len) ))
		{
			ObDereferenceObject(Object);
			FINISH_GetPathFromOA(("GetPathFromOA(): ObQueryNameString() failed\n"));
		}


		ObDereferenceObject(Object);
		Object = NULL;


		/* extracted directory name */
		pusFilename = &pONI->Name;


		/* is the directory name too long? */

		if (pusFilename->Length >= bMAX_PATH - sizeof(WCHAR))
			FINISH_GetPathFromOA(("GetPathFromOA(): directory name is too long\n"));


		/*
		 * pusFilename points to a buffer of MAX_PATH size, ObQueryNameString() sets MaximumLength to the length
		 * of the directory name, we need to reset this back to MAX_PATH to be able to append a filename
		 * (reusing the same buffer)
		 */
		pusFilename->MaximumLength = bMAX_PATH;


		pusFilename->Buffer[ pusFilename->Length / sizeof(WCHAR) ] = L'\\';
		pusFilename->Length += sizeof(WCHAR);


		if (RtlAppendUnicodeStringToString(pusFilename, ObjectAttributes->ObjectName) == STATUS_BUFFER_TOO_SMALL)
		{
			LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("GetPathFromOA: 1 %S\n", pusFilename->Buffer));
			LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("GetPathFromOA: 2 %S\n", ObjectAttributes->ObjectName->Buffer));
			FINISH_GetPathFromOA(("GetPathFromOA(): RtlAppendUnicodeStringToString() = STATUS_BUFFER_TOO_SMALL\n"));
		}
	}


	if (ResolveLinks == TRUE)
	{
		ret = ResolveFilenameW(pusFilename, OutBuffer, OutBufferSize);
	}

//XXX
	if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&name, pusFilename, TRUE)))
	{	
		if (ResolveLinks == TRUE)
		{
			ret = ResolveFilename(name.Buffer, OutBuffer, OutBufferSize);
		}
		else
		{
			if (name.Length >= OutBufferSize - 1)
			{
				LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("GetPathFromOA: Pathname too long %d\n", name.Length));

				OutBuffer[0] = 0;

				ret = FALSE;
			}
			else
			{
				strcpy(OutBuffer, name.Buffer);

				ret = TRUE;
			}
		}

		RtlFreeAnsiString(&name);
	}


//	LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("%d GetPathFromOA: %s (%S)\n", (ULONG) PsGetCurrentProcessId(), OutBuffer, pusFilename->Buffer));


	return ret;
}


/*
 * ConvertLongFileNameToShort()
 *
 * Description:
 *		Converts long windows filenames to their DOS short equivalent filenames
 *		(i.e. c:\program files to c:\progra~1).
 *
 * Parameters:
 *		LongFileName - long filename buffer.
 *		ShortFileName - output buffer where a short filename is written to.
 *		ShortFileNameSize - size of an output buffer (in bytes).
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

#if 0

BOOLEAN
ConvertLongFileNameToShort(IN PCHAR LongFileName, OUT PCHAR ShortFileName, IN USHORT ShortFileNameSize)
{
	int			LongFileNameIndex = 0, ShortFileNameIndex = 0, CurrentFileNameLength, TotalLength, ExtensionLength, NumberOfSpaces;
	BOOLEAN		ProcessingExtension = FALSE;
	CHAR		ch, Extension[3];


	if (LongFileName == NULL)
		return FALSE;

	TotalLength = strlen(LongFileName);


	/* if the filename does not start with X:\ then assume \device\blah\ format and skip over the first 2 slashes */
	if (LongFileName[0] == '\\')
	{
		int Slashes = 0;

		do
		{
			if ( (ch = ShortFileName[ShortFileNameIndex++] = LongFileName[LongFileNameIndex++]) == '\0') return TRUE;
			if (ch == '\\') ++Slashes;
		} while (Slashes != 3);
	}

	for (NumberOfSpaces = ExtensionLength = CurrentFileNameLength = 0; ; LongFileNameIndex++)
	{
		/* if we finished traversing the entire directory path or reached a '\' then process the filename (append the extension if necessary) */
		if (LongFileNameIndex == TotalLength || LongFileName[LongFileNameIndex] == '\\')
		{
			/*
			 * if the filename is longer than 8 chars or extension is longer than 3 chars then we need
			 * to create a 6 char filename followed by a '~1' and the first 3 chars of the last extension
			 */

			if (CurrentFileNameLength > 8 || ExtensionLength > 3 || NumberOfSpaces > 0)
			{
				CurrentFileNameLength -= NumberOfSpaces;

				if (CurrentFileNameLength > 7)
				{
					ShortFileName[ShortFileNameIndex - 2] = '~';
					ShortFileName[ShortFileNameIndex - 1] = '1';
				}
				else if (CurrentFileNameLength == 7)
				{
					ShortFileName[ShortFileNameIndex - 1] = '~';
					ShortFileName[ShortFileNameIndex++] = '1';
				}
				else
				{
					ShortFileName[ShortFileNameIndex++] = '~';
					ShortFileName[ShortFileNameIndex++] = '1';
				}
			}

			if (ExtensionLength > 0)
			{
				ShortFileName[ShortFileNameIndex++] = '.';
				ShortFileName[ShortFileNameIndex++] = Extension[0];

				if (ExtensionLength > 1)
				{
					ShortFileName[ShortFileNameIndex++] = Extension[1];

					if (ExtensionLength > 2)
						ShortFileName[ShortFileNameIndex++] = Extension[2];
				}

				ExtensionLength = 0;
				ProcessingExtension = FALSE;
			}

			/* if we are done traversing the entire path than we can bail */
			if (LongFileNameIndex == TotalLength)
				break;

			ShortFileName[ShortFileNameIndex++] = '\\';
			NumberOfSpaces = CurrentFileNameLength = 0;

			continue;
		}

		if (LongFileName[LongFileNameIndex] == '.')
		{
			ProcessingExtension = TRUE;
			ExtensionLength = 0;
			continue;
		}

		if (ProcessingExtension == TRUE)
		{
			if (ExtensionLength++ < 3)
				Extension[ExtensionLength - 1] = LongFileName[LongFileNameIndex];

			continue;
		}

		if (((CurrentFileNameLength++) - NumberOfSpaces) < 8)
		{
			if (LongFileName[LongFileNameIndex] != ' ')
				ShortFileName[ShortFileNameIndex++] = LongFileName[LongFileNameIndex];
			else
				++NumberOfSpaces;
		}
	}


	ShortFileName[ShortFileNameIndex++] = 0;


	return TRUE;
}

#endif



/*
 * GetNameFromHandle()
 *
 * Description:
 *		Resolve an object handle to an object name.
 *
 * Parameters:
 *		ObjectHandle - handle of an object whose name we are trying to obtain.
 *		OutBuffer - output buffer where an object name will be saved to.
 *		OutBufferSize - size of an output buffer (in bytes).
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

PWSTR
GetNameFromHandle(IN HANDLE ObjectHandle, OUT PWSTR OutBuffer, IN USHORT OutBufferSize)
{
	PVOID						Object = NULL;
	NTSTATUS					rc;
	POBJECT_NAME_INFORMATION	pONI = (POBJECT_NAME_INFORMATION) OutBuffer;
	ULONG						len;


	rc = ObReferenceObjectByHandle(ObjectHandle, GENERIC_READ, NULL, KernelMode, &Object, NULL);
	if (! NT_SUCCESS(rc))
	{
		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("%d GetNameFromHandle: ObReferenceObjectByHandle failed\n", (ULONG) PsGetCurrentProcessId()));
		return NULL;
	}


	rc = ObQueryNameString(Object, pONI, OutBufferSize - sizeof(OBJECT_NAME_INFORMATION)*sizeof(WCHAR), &len);
	if (! NT_SUCCESS(rc))
	{
		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_VERBOSE, ("%d GetNameFromHandle: ObQueryNameString failed\n", (ULONG) PsGetCurrentProcessId()));
		return NULL;
	}

	
//	_snprintf(OutBuffer, OutBufferSize, "%S", pONI->Name.Buffer);
//	OutBuffer[OutBufferSize - 1] = 0;

//	LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("%S (%s)\n", pONI->Name.Buffer, OutBuffer));


	ObDereferenceObject(Object);


	return pONI->Name.Buffer;
//	return TRUE;
}



/*
 * StripFileMacros()
 *
 * Description:
 *		Strip file names of %SystemRoot% and %SystemDrive% macros as well as any specifications.
 *
 * Parameters:
 *		Path - ASCII file path to parse.
 *		Buffer - pointer to an Object where the final result will be saved.
 *		BufferSize - size of the output Buffer.
 *
 * Returns:
 *		Pointer to a stripped ASCII path.
 */

PCHAR
StripFileMacros(IN PCHAR Path, OUT PCHAR Buffer, IN USHORT BufferSize)
{
	if (_strnicmp(Path, "%systemdrive%:", 14) == 0)
	{
		return Path + 14;
	}


	if (_strnicmp(Path, "%systemroot%\\", 13) == 0)
	{
		if (_snprintf(Buffer, MAX_PATH, "%s%s", SystemRootUnresolved, Path + 12) < 0)
			return NULL;

		Path = Buffer;
	}


	if (Path[1] == ':' && Path[2] == '\\' && (isalpha(Path[0]) || Path[0] == '?' || Path[0] == '*'))
	{
		Path += 2;
	}


	return Path;
}



/*
 * FixupFilename()
 *
 * Description:
 *		Get canonical name for a file (without the drive specification, i.e. \windows\blah.exe)
 *
 * Parameters:
 *		szFileName - filename to resolve.
 *		szResult - output buffer.
 *		szResultSize - size of an output buffer.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
FixupFilename(IN PCHAR szFileName, OUT PCHAR szResult, IN USHORT szResultSize)
{
	/* skip over \??\ */
	if (_strnicmp(szFileName, "\\??\\", 4) == 0)
	{
		szFileName += 4;
	}


	/* replace "\SystemRoot" references with the actual system root directory */
	if (_strnicmp(szFileName, "\\SystemRoot\\", 12) == 0)
	{
		_snprintf(szResult, szResultSize, "%s\\%s", SystemRootDirectory, szFileName + 12);
		szResult[ szResultSize - 1 ] = 0;

		return TRUE;
	}


	/* skip over X: drive specifications */
	if (isalpha(szFileName[0]) && szFileName[1] == ':' && szFileName[2] == '\\')
	{
		szFileName += 2;
	}


	strncpy(szResult, szFileName, szResultSize);
	szResult[ szResultSize - 1 ] = 0;


	return TRUE;
}



/*
 * AreMalformedExtensionsAllowed()
 *
 * Description:
 *		Check whether the current process is allowed to run binaries with malformed file extensions.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		FALSE if binaries with malformed extensions are not allowed to run. TRUE otherwise.
 */

BOOLEAN
AreMalformedExtensionsAllowed()
{
	PIMAGE_PID_ENTRY	CurrentProcess;
	BOOLEAN				MalformedExtensionsAllowed = FALSE;


	/* check the global policy first */
	if (! IS_EXTENSION_PROTECTION_ON(gSecPolicy))
		return TRUE;


	/* now check the process specific policy */
	CurrentProcess = FindImagePidEntry(CURRENT_PROCESS_PID, 0);

	if (CurrentProcess != NULL)
	{
		MalformedExtensionsAllowed = ! IS_EXTENSION_PROTECTION_ON(CurrentProcess->SecPolicy);
	}
	else
	{
		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("%d AreMalformedExtensionsAllowed: CurrentProcess = NULL!\n", (ULONG) PsGetCurrentProcessId()));
	}


	return MalformedExtensionsAllowed;
}



/*
 * VerifyExecutableName()
 *
 * Description:
 *		Make sure the executed binary does not have a funny filename.
 *		Look out for non-standard extensions (.exe, etc) and double
 *		extensions that are commonly "ab"used by malware.
 *
 * Parameters:
 *		szFileName - filename to verify.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

#define	CHECK_LEARNING_MODE()								\
	if (LearningMode == TRUE)								\
	{														\
		TURN_EXTENSION_PROTECTION_OFF(NewPolicy);			\
		return TRUE;										\
	}

BOOLEAN
VerifyExecutableName(IN PCHAR szFileName)
{
	SHORT		i, len;
	BOOLEAN		FirstExtension = TRUE;


	if (LearningMode == FALSE && AreMalformedExtensionsAllowed() == TRUE)
	{
		/* no need to check anything further, malformed extensions are allowed */
		return TRUE;
	}


	if ((len = (SHORT) strlen(szFileName)) == 0)
		return TRUE;


	for (i = len - 1; i >= 0; i--)
	{
		/* bail out once we reach the end of the filename */
		if (szFileName[i] == '\\')
			break;

		if (szFileName[i] == '.')
		{
			if (FirstExtension == FALSE)
			{
				CHECK_LEARNING_MODE();

				LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("VerifyExecutableName: Executing a binary with more than one extension '%s'\n", szFileName));

				LogAlert(ALERT_SS_PROCESS, OP_PROC_EXECUTE, ALERT_RULE_PROCESS_EXEC_2EXTS, ACTION_LOG, ALERT_PRIORITY_HIGH, NULL, 0, szFileName);

				return FALSE;
			}

			if (len - i != 4)
			{
				CHECK_LEARNING_MODE();

				LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("VerifyExecutableName: Executing a binary with an unknown extension '%s'\n", szFileName));

				LogAlert(ALERT_SS_PROCESS, OP_PROC_EXECUTE, ALERT_RULE_PROCESS_EXEC_UNKNOWN, ACTION_LOG, ALERT_PRIORITY_HIGH, NULL, 0, szFileName);

				return FALSE;
			}
			else
			{
				if (_stricmp(szFileName + i + 1, "exe") != 0 &&
					_stricmp(szFileName + i + 1, "com") != 0 &&
					_stricmp(szFileName + i + 1, "bat") != 0 &&
					_stricmp(szFileName + i + 1, "scr") != 0 &&
					_stricmp(szFileName + i + 1, "dir") != 0 &&
					_stricmp(szFileName + i + 1, "tmp") != 0 &&
					_stricmp(szFileName + i + 1, "_mp") != 0)
				{
					CHECK_LEARNING_MODE();

					LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("VerifyExecutableName: Executing a binary with an unknown extension '%s'\n", szFileName));

					LogAlert(ALERT_SS_PROCESS, OP_PROC_EXECUTE, ALERT_RULE_PROCESS_EXEC_UNKNOWN, ACTION_LOG, ALERT_PRIORITY_HIGH, NULL, 0, szFileName);

					return FALSE;
				}
			}

			FirstExtension = FALSE;
		}
	}


	if (FirstExtension == TRUE)
	{
		CHECK_LEARNING_MODE();

		LOG(LOG_SS_PATHPROC, LOG_PRIORITY_DEBUG, ("VerifyExecutableName: Executing binary without an extension '%s'\n", szFileName));

		LogAlert(ALERT_SS_PROCESS, OP_PROC_EXECUTE, ALERT_RULE_PROCESS_EXEC_NOEXT, ACTION_LOG, ALERT_PRIORITY_HIGH, NULL, 0, szFileName);

		return FALSE;
	}


	return TRUE;
}
