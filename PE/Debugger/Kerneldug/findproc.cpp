#include <ntifs.h>

PVOID 
GetSystemInformation (
	SYSTEM_INFORMATION_CLASS InfoClass
	)
/**
	Get system information by class.
	ZwQuerySystemInformation wrapper
*/
{
	NTSTATUS Status;
	PVOID Buffer;
	ULONG Size = PAGE_SIZE;
	
	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	do
	{
		Buffer = ExAllocatePool (PagedPool, Size);

		Status = ZwQuerySystemInformation ( InfoClass,
											Buffer,
											Size,
											&Size );

		if (Status == STATUS_INFO_LENGTH_MISMATCH)
			ExFreePool (Buffer);

	}
	while (Status == STATUS_INFO_LENGTH_MISMATCH);

	if (!NT_SUCCESS(Status))
	{
		ExFreePool (Buffer);
		return NULL;
	}

	return Buffer;
}

typedef struct SYSTEM_MODULES_INFORMATION
{
	ULONG ModuleCount;
	SYSTEM_MODULE_INFORMATION Modules[1];
} *PSYSTEM_MODULES_INFORMATION;

PEPROCESS
GetProcessByName(
	PWCHAR wszProcessName,
	PETHREAD *FirstThread OPTIONAL
	)
{
	PSYSTEM_PROCESSES_INFORMATION Processes = (PSYSTEM_PROCESSES_INFORMATION) GetSystemInformation (SystemProcessesAndThreadsInformation);
	NTSTATUS Status = STATUS_NOT_FOUND;

	PEPROCESS Process;

	if (Processes)
	{
		for (PSYSTEM_PROCESSES_INFORMATION Proc=Processes; ; *(ULONG*)&Proc += Proc->NextEntryDelta)
		{
			if (Proc->ProcessName.Buffer && 
				!_wcsicmp (Proc->ProcessName.Buffer, wszProcessName))
			{
				KdPrint(("Found process, ID=%d\n", Proc->ProcessId));
				
				Status = PsLookupProcessByProcessId ((PVOID) Proc->ProcessId, (&Process));

				if(NT_SUCCESS(Status))
				{
					if (ARGUMENT_PRESENT(FirstThread))
					{
						*FirstThread = NULL;
						Status = PsLookupThreadByThreadId ( (PVOID)Proc->Threads[0].ClientId.UniqueThread, (FirstThread) );

						if (!NT_SUCCESS(Status))
						{
							KdPrint(("Could not reference first thread of %S\n", wszProcessName));
						}
					}

					ExFreePool (Processes);
					return Process;
				}

				break;
			}

			if (!Proc->NextEntryDelta) break;
		}

		ExFreePool (Processes);

		return NULL;
	}

	KdPrint(("ZwQuerySystemInformation failed\n"));
	return NULL;
}

PVOID FindImage (PWSTR ImageName)
{
	UNICODE_STRING UnicodeImageName;
	ANSI_STRING AnsiImageName;
	NTSTATUS Status;
	PWSTR ImageRelativeName;

	ImageRelativeName = wcsrchr (ImageName, L'\\');
	if (ImageRelativeName)
		ImageRelativeName ++;
	else
		ImageRelativeName = ImageName;

	KdPrint(("Searching image '%S'\n", ImageRelativeName));

	RtlInitUnicodeString (&UnicodeImageName, ImageRelativeName);
	Status = RtlUnicodeStringToAnsiString (&AnsiImageName, &UnicodeImageName, TRUE);

	if (!NT_SUCCESS(Status))
	{
		KdPrint (("RtlUnicodeStringToAnsiString failed with status %X\n", Status));
		return NULL;
	}

	PSYSTEM_MODULES_INFORMATION Modules = (PSYSTEM_MODULES_INFORMATION) GetSystemInformation (SystemModuleInformation);

	if (Modules)
	{
		ULONG i;

		if (!_wcsicmp(ImageName, L"hal.dll"))
		{
			// hal is always the second in the list.
			i = 1;
			goto _found;
		}

		for (i=0; i<Modules->ModuleCount; i++)
		{
			char *relative_name = strrchr (Modules->Modules[i].ImageName, '\\');
			if (relative_name)
				relative_name ++;
			else
				relative_name = Modules->Modules[i].ImageName;

//			KdPrint((" ~ image : '%s' base at %X\n", relative_name, Modules->Modules[i].Base));

			if (!_strnicmp (relative_name, AnsiImageName.Buffer, AnsiImageName.Length))
			{
_found:
				PVOID CapturedBase = Modules->Modules[i].Base;

				KdPrint(("Found image : '%s' base at %X\n", relative_name, Modules->Modules[i].Base));

				ExFreePool (Modules);
				RtlFreeAnsiString (&AnsiImageName);

				return CapturedBase;
			}
		}
		ExFreePool (Modules);
		RtlFreeAnsiString (&AnsiImageName);

		return NULL;
	}
	
	RtlFreeAnsiString (&AnsiImageName);

	KdPrint(("ZwQuerySystemInformation failed\n"));

	return NULL;
}