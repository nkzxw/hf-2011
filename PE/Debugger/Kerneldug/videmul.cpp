#include <windows.h>


/*
 * Загрузчик PE
 */

//#define WIN32_NO_STATUS
#include <windows.h>
//#undef WIN32_NO_STATUS

#include <stdio.h>

typedef ULONG NTSTATUS;
ULONG (WINAPI *RtlNtStatusToDosError)(NTSTATUS);

//#pragma warning(disable:4005)
//#include <ntstatus.h>

// Фиксапы
typedef struct 
{
	WORD	Offset:12;
	WORD	Type:4;
} IMAGE_FIXUP_ENTRY, *PIMAGE_FIXUP_ENTRY;

// Быстро получаем хидеры
inline void GetHeaders(PCHAR ibase, PIMAGE_FILE_HEADER *pfh, PIMAGE_OPTIONAL_HEADER *poh, PIMAGE_SECTION_HEADER *psh)
{
	PIMAGE_DOS_HEADER mzhead = (PIMAGE_DOS_HEADER)ibase;
	*pfh = (PIMAGE_FILE_HEADER)&ibase[mzhead->e_lfanew];
	*pfh = (PIMAGE_FILE_HEADER)((PBYTE)*pfh + sizeof(IMAGE_NT_SIGNATURE));
	*poh = (PIMAGE_OPTIONAL_HEADER)((PBYTE)*pfh + sizeof(IMAGE_FILE_HEADER));
	*psh = (PIMAGE_SECTION_HEADER)((PBYTE)*poh + sizeof(IMAGE_OPTIONAL_HEADER));
}

// Смещение + База = Виртуальный Адрес
#define RVATOVA( base, offset )(((DWORD)(base) + (DWORD)(offset))) 

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

extern "C" {
	VOID
	WINAPI
	RtlInitUnicodeString(
	  IN PUNICODE_STRING UnicodeString,
	  IN PWSTR Buffer
	  );

	VOID
	WINAPI
	RtlEnterCriticalSection(
	  IN PVOID Section
	);

	VOID
	WINAPI
	RtlLeaveCriticalSection(
	  IN PVOID Section
	);
}

typedef struct _LDR_MODULE {
  LIST_ENTRY              InLoadOrderModuleList;
  LIST_ENTRY              InMemoryOrderModuleList;
  LIST_ENTRY              InInitializationOrderModuleList;
  PVOID                   BaseAddress;
  PVOID                   EntryPoint;
  ULONG                   SizeOfImage;
  UNICODE_STRING          FullDllName;
  UNICODE_STRING          BaseDllName;
  ULONG                   Flags;
  SHORT                   LoadCount;
  SHORT                   TlsIndex;
  LIST_ENTRY              HashTableEntry;
  ULONG                   TimeDateStamp;

} LDR_MODULE, *PLDR_MODULE;



typedef struct _PEB_LDR_DATA {
  ULONG                   Length;
  BOOLEAN                 Initialized;
  PVOID                   SsHandle;
  LIST_ENTRY              InLoadOrderModuleList;
  LIST_ENTRY              InMemoryOrderModuleList;
  LIST_ENTRY              InInitializationOrderModuleList;

} PEB_LDR_DATA, *PPEB_LDR_DATA;

// Process Envorinment Block
typedef struct _PEB {
  DWORD smth[2]; // doesn't matter
  PVOID ImageBaseAddress;
  PPEB_LDR_DATA Ldr;
  PVOID                   ProcessParameters;
  PVOID                   SubSystemData;
  PVOID                   ProcessHeap;
  PVOID                   FastPebLock;
  PVOID                   FastPebLockRoutine;
  PVOID                   FastPebUnlockRoutine;
  ULONG                   EnvironmentUpdateCount;
  PVOID                   KernelCallbackTable;
  PVOID                   EventLogSection;
  PVOID                   EventLog;
  PVOID                   FreeList;
  ULONG                   TlsExpansionCounter;
  PVOID                   TlsBitmap;
  ULONG                   TlsBitmapBits[0x2];
  PVOID                   ReadOnlySharedMemoryBase;
  PVOID                   ReadOnlySharedMemoryHeap;
  PVOID                   ReadOnlyStaticServerData;
  PVOID                   AnsiCodePageData;
  PVOID                   OemCodePageData;
  PVOID                   UnicodeCaseTableData;
  ULONG                   NumberOfProcessors;
  ULONG                   NtGlobalFlag;
  BYTE                    Spare2[0x4];
  LARGE_INTEGER           CriticalSectionTimeout;
  ULONG                   HeapSegmentReserve;
  ULONG                   HeapSegmentCommit;
  ULONG                   HeapDeCommitTotalFreeThreshold;
  ULONG                   HeapDeCommitFreeBlockThreshold;
  ULONG                   NumberOfHeaps;
  ULONG                   MaximumNumberOfHeaps;
  PVOID                   **ProcessHeaps;
  PVOID                   GdiSharedHandleTable;
  PVOID                   ProcessStarterHelper;
  PVOID                   GdiDCAttributeList;
  PVOID                   LoaderLock;
  ULONG                   OSMajorVersion;
  ULONG                   OSMinorVersion;
  ULONG                   OSBuildNumber;
  ULONG                   OSPlatformId;
  ULONG                   ImageSubSystem;
  ULONG                   ImageSubSystemMajorVersion;
  ULONG                   ImageSubSystemMinorVersion;
  ULONG                   GdiHandleBuffer[0x22];
  ULONG                   PostProcessInitRoutine;
  ULONG                   TlsExpansionBitmap;
  BYTE                    TlsExpansionBitmapBits[0x80];
  ULONG                   SessionId;
} PEB, *PPEB;

// Thread Environment Block
typedef struct _TEB {
 DWORD smth[12]; // doesn't matter
 PPEB Peb;
} TEB, *PTEB;

// получаем текущий PEB
PPEB GetPEB( )
{
	TEB* teb;
	__asm
	{
		mov eax, dword ptr fs:[18h]
		mov teb, eax
	}
	return teb->Peb;
}


bool
LdrpReadSections (
  IN void* base,
  IN HANDLE hFile
  )
/*
	Читаем секции из файла и грузим их по соответствующим виртуальным адресам
*/
{
	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	DWORD read;
	GetHeaders((PCHAR) base, &pfh, &poh, &psh);

	for( int i=0; i < pfh->NumberOfSections ; i++, psh++ )
	{
		DWORD VirtualSize = (i==pfh->NumberOfSections-1)?
				(poh->SizeOfImage-psh->VirtualAddress)
			   :(psh+1)->VirtualAddress - psh->VirtualAddress;
		LPVOID va = (LPVOID)( (DWORD)base + psh->VirtualAddress );
		OVERLAPPED lp = {0};
		
		printf(" %-14s    %08x     %08x       %08x              %08x\n",
			psh->Name,
			psh->VirtualAddress,
			VirtualSize,
			psh->PointerToRawData,
			psh->SizeOfRawData
			);

		lp.Offset = psh->PointerToRawData;

		// выделяем память под секцию
		LPVOID m = VirtualAlloc( va, VirtualSize, MEM_COMMIT, PAGE_READWRITE );
		if( (DWORD)m != ((DWORD)va & 0xfffff000) )
		{
			printf("ERROR while committing memory %d\n", GetLastError());
			return false;
		}

		// читаем секцию
		if( !ReadFile( hFile, va, psh->SizeOfRawData, &read, &lp ) || read!=psh->SizeOfRawData )
		{
			printf("Cannot read section [%d]\n", GetLastError());
			return false;
		}
	}

	return true;
}


bool
LdrpApplyFixups (
  IN void* base,
  IN DWORD ImageBaseDelta
  )
/*
	Применить фиксапы к образу
*/
{
	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	GetHeaders((PCHAR) base, &pfh, &poh, &psh);

	// требуется корректировка?
	if( ImageBaseDelta )
	{
		printf("\nApplying fixups, ImageBaseDelta = %08x\n", ImageBaseDelta);

		if( !poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress )
		{
			// нет фиксапов
			printf("No fixups present looking to the nonzero image base delta\n");
			return false;
		}

		PIMAGE_BASE_RELOCATION Reloc = (PIMAGE_BASE_RELOCATION) RVATOVA(poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress, base);
		int i = 0;

_repeat:
		// проходим по фиксапам
		for( PIMAGE_FIXUP_ENTRY Fixup = (PIMAGE_FIXUP_ENTRY)( (DWORD)Reloc + sizeof(IMAGE_BASE_RELOCATION) );
		     (DWORD)Fixup < (DWORD)Reloc + Reloc->SizeOfBlock;
			 Fixup ++, i ++
				 )
		{
			if (Fixup->Type == 0)
				continue;
			if( Fixup->Type == IMAGE_REL_BASED_HIGHLOW )
			{
				// адрес инструкции для патча
				DWORD* Patch = (DWORD*)RVATOVA( Reloc->VirtualAddress + Fixup->Offset, base );
				
//				printf("Fixup #%d: Type = %d, Offset %08x [%08x -> %08x]\n",
//					i,
//					Fixup->Type,
//					RVATOVA( Reloc->VirtualAddress + Fixup->Offset, base ),
//					*Patch,
//					*Patch+ImageBaseDelta
//					);

				// патч
				*Patch += ImageBaseDelta;
			}
			// другие типы фиксапов не обрабатываем
			else
			{
				printf("Incorrect fixup type %d\n", Fixup->Type);
				return false;
			}
		}

		//if( *(WORD*)((DWORD)Reloc + Reloc->SizeOfBlock) != 0 )
		if ( ((ULONG)Reloc - RVATOVA(poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress, base)) <
			    poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size )
		{
			Reloc = (PIMAGE_BASE_RELOCATION)((DWORD)Reloc + Reloc->SizeOfBlock);
			goto _repeat;
		}
	}

	return true;
}


bool
LdrpProcessImport (
  IN void* base
  )
/*
	Обработка директории импорта
*/
{
	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	GetHeaders((PCHAR) base, &pfh, &poh, &psh);

	// Первый дескриптор импорта
	PIMAGE_IMPORT_DESCRIPTOR impdesc = (PIMAGE_IMPORT_DESCRIPTOR) RVATOVA(poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, base);
	printf(" DLL Name       TimeDateStamp   Import\n");

	for( ; impdesc->Name ; impdesc++ )
	{
		// Получаем имя DLL
		char* dllname = (LPTSTR) RVATOVA(impdesc->Name, base);
		printf("%-15s    %08x     ",  dllname, impdesc->TimeDateStamp);

		HINSTANCE hDll = NULL;

		if (!stricmp(dllname, "win32k.sys"))
		{
			hDll = GetModuleHandle(0);
		}


		if( !hDll )
		{
			// не вышло...
			printf("Error loading '%s'. Application initialization failed\n");
			return false;
		}

		// откуда брать функции - из FirstThunk или из OriginalFirstThunk
		DWORD RvaOfThunks = impdesc->FirstThunk;

		// Импорт забинден?
		if( impdesc->TimeDateStamp == -1 )
		{
			// Да, чекаем на валидность тупой hgkgjhgkhkjhkhгншгншнгроверкой первой функции
			DWORD *Func;

			// грузим первую функци.
			PIMAGE_THUNK_DATA OriginalThunk = (PIMAGE_THUNK_DATA)RVATOVA(impdesc->OriginalFirstThunk, base);
			PIMAGE_THUNK_DATA Thunk = (PIMAGE_THUNK_DATA)RVATOVA(impdesc->FirstThunk, base);

			if( OriginalThunk->u1.Ordinal & 0xf0000000 )
			{
				OriginalThunk->u1.Ordinal &= 0xffff;
				*(FARPROC*)&Func = GetProcAddress( hDll, (char*) OriginalThunk->u1.Ordinal );
			}
			else
			{
				PIMAGE_IMPORT_BY_NAME Name = (PIMAGE_IMPORT_BY_NAME) RVATOVA(OriginalThunk->u1.AddressOfData, base);
				*(FARPROC*)&Func = GetProcAddress( hDll, (char*) Name->Name );
			}

			if (!*(FARPROC*)&Func)
				__asm int 3;

			// сравниваем
			if( Thunk->u1.Function == (ULONG_PTR)Func )
			{
				printf("CORRECT\n");
				continue;
			}
			// адреса не совпадают, пробинденный импорт надо перестроить
			else
			{
				printf("FAILED [needs rebuilding]\n");
				RvaOfThunks	= impdesc->OriginalFirstThunk; // для ребиндинга брать функции из OriginalFirstThunk
			}
		}
		else
			printf("NOT BOUND\n");

		// Обрабатываем thunk'и
		for( PIMAGE_THUNK_DATA Thunk = (PIMAGE_THUNK_DATA)RVATOVA(RvaOfThunks, base); Thunk->u1.Ordinal ; Thunk++ )
		{
			// Функция задана ординатой?
			if( Thunk->u1.Ordinal & 0xf0000000 )
			{
				Thunk->u1.Ordinal &= 0xffff;
				printf("Ordinal: %04x, ", Thunk->u1.Ordinal);
				
				Thunk->u1.Function = (DWORD_PTR) GetProcAddress( hDll, (char*) Thunk->u1.Ordinal );

				if( !Thunk->u1.Function )
				{
					printf("Can't find address for this function\n");
					return false;
				}

				printf("got address %08x\n", Thunk->u1.Function);
			}
			// функция задана по имени
			else
			{
				PIMAGE_IMPORT_BY_NAME Name = (PIMAGE_IMPORT_BY_NAME) RVATOVA(Thunk->u1.AddressOfData, base);
				printf("%03d(0x%04x) \t %-20s ", Name->Hint, Name->Hint, Name->Name );

				Thunk->u1.Function = (DWORD_PTR) GetProcAddress( hDll, (char*) Name->Name );
				printf("= %08x\n", Thunk->u1.Function);
			}

			if (!Thunk->u1.Function)
				__asm int 3;

			// не важно откуда читали, записываем все равно в FirstThunk
			PIMAGE_THUNK_DATA ThunkToWrite;
			ThunkToWrite = (PIMAGE_THUNK_DATA)(
				(DWORD)RVATOVA(impdesc->FirstThunk, base) + ((DWORD)Thunk - (DWORD)RVATOVA(RvaOfThunks, base))
				);
			ThunkToWrite->u1.Function = Thunk->u1.Function;
		}

	}
	return true;
}


bool
LdrpProtectSections (
  IN void* base
  )
/*
	Выставить атрибуты секций содержащим их страницам
*/
{
	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	GetHeaders((PCHAR) base, &pfh, &poh, &psh);

	psh = (PIMAGE_SECTION_HEADER)( (DWORD)poh + sizeof(IMAGE_OPTIONAL_HEADER) );
	
	for( int i=0; i < pfh->NumberOfSections ; i++, psh++ )
	{
		DWORD VirtualSize = (i==pfh->NumberOfSections-1)?
				(poh->SizeOfImage-psh->VirtualAddress)
			   :(psh+1)->VirtualAddress - psh->VirtualAddress;
		LPVOID va = (LPVOID)( (DWORD)base + psh->VirtualAddress );

		// считаем атрибуты
		DWORD Attributes = PAGE_READWRITE;
		if( psh->Characteristics & IMAGE_SCN_MEM_EXECUTE || psh->Characteristics & IMAGE_SCN_MEM_READ )
		{
			if( psh->Characteristics & IMAGE_SCN_MEM_WRITE )
				Attributes = PAGE_READWRITE;
			else
				Attributes = PAGE_READONLY;
		}
		else if( psh->Characteristics & IMAGE_SCN_MEM_WRITE )
			Attributes = PAGE_READWRITE;
		
		MEMORY_BASIC_INFORMATION mbi = {0};
		if( VirtualQuery( va, &mbi, sizeof(mbi) ) && mbi.Protect == PAGE_READWRITE )
			continue;

		// ставим атрибуты
		if( !VirtualProtect( va, VirtualSize, Attributes, &Attributes ) )
		{
			printf("Cannot protect section [%d]\n", GetLastError());
			return false;
		}
	}

	return true;
}


typedef struct MODULE32
{
	char ModuleName[256];
	PVOID ImageBase;
	ULONG ImageSize;
} *PMODULE32;

MODULE32 LoadedModules[128];
ULONG LoadedModCount = 0;


HMODULE
LdrLoadExecutable (
  IN LPSTR path,
  IN PVOID base = 0
  )
/*
	Загружает образ с пути path по адресу base.
	Если base=0, используется поле IMAGE_OPTIONAL_HEADER.ImageBase
*/
{
	HANDLE hFile;
	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_SECTION_HEADER	psh;
	PIMAGE_OPTIONAL_HEADER poh;
	char buffer[ 0x1000 ] = {0};
	DWORD read;
	LPBYTE Mapping;

	//
	// Открываем файл образа
	//

	printf("Opening file '%s'\n", path);

	hFile = CreateFile( path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	if( hFile == INVALID_HANDLE_VALUE )
		return NULL;

	//
	// Читаем хидеры
	//

	printf("Reading header\n");
	if( !ReadFile( hFile, buffer, 1024, &read, 0 ) || !read )
	{
		CloseHandle( hFile );
		return NULL;
	}

	/*PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER) buffer;
	pfh = (PIMAGE_FILE_HEADER)( (DWORD)buffer + dos->e_lfanew + 4 );
	poh = (PIMAGE_OPTIONAL_HEADER)( pfh+1 );
	psh = (PIMAGE_SECTION_HEADER)( (DWORD)poh + sizeof(IMAGE_OPTIONAL_HEADER) );*/
	GetHeaders( buffer, &pfh, &poh, &psh );

	if( *(WORD*)buffer != 'ZM' )
	{
//		SetLastError(RtlNtStatusToDosError(STATUS_INVALID_IMAGE_NOT_MZ));
		CloseHandle( hFile );
		return NULL;
	}



	//
	// Резервируем пространство под образ
	//

	if( !base )
		base = (PVOID) poh->ImageBase;

	printf("Reserving pages[ OriginalBase = %08x, Base = %08x, Size = %08x ]\n", poh->ImageBase, base, poh->SizeOfImage);
	
	UnmapViewOfFile( (PVOID)poh->ImageBase );
	VirtualFree( (PVOID)poh->ImageBase, poh->SizeOfImage, MEM_DECOMMIT );
	VirtualFree( (PVOID)poh->ImageBase, poh->SizeOfImage, MEM_RELEASE );

_repeat:
	Mapping = (LPBYTE) VirtualAlloc( base, poh->SizeOfImage, MEM_RESERVE, PAGE_NOACCESS );
	if( !Mapping )	
	{
		printf("alloc failed at %x\n", base);
		*(ULONG*)&base += 0x10000000;
		goto _repeat;

		CloseHandle( hFile );
		return NULL;
	}

	//
	// Копируем хидеры
	//

	Mapping = (LPBYTE) VirtualAlloc( base, 0x1000, MEM_COMMIT, PAGE_READWRITE );
	if( !Mapping )	
	{
		CloseHandle( hFile );
		return NULL;
	}
	memcpy( Mapping, buffer, read );
	
	base = Mapping;

	//
	// Читаем секции из файла и грузим их по соответствующим виртуальным адресам
	//

	printf("\nReading sections\n");
	printf("%-15s %-15s %-15s %-15s %-15s\n", "Section", "VirtAddress", "VirtSize", "RawData", "SizeOfRawData" );

	if( !LdrpReadSections( base, hFile ) )
	{
		VirtualFree( base, poh->SizeOfImage, MEM_DECOMMIT );
		VirtualFree( base, poh->SizeOfImage, MEM_RELEASE );
		CloseHandle( hFile );
		return NULL;
	}

	//
	// Корректируем фиксапы
	//

	DWORD ImageBaseDelta = (DWORD)base - (DWORD)poh->ImageBase;

	printf("Relocating from %X delta %X, new base %X\n", poh->ImageBase, ImageBaseDelta, base);

	if( !LdrpApplyFixups( base, ImageBaseDelta ) )
	{
		VirtualFree( base, poh->SizeOfImage, MEM_DECOMMIT );
		VirtualFree( base, poh->SizeOfImage, MEM_RELEASE );
		CloseHandle( hFile );
		return NULL;
	}

	//
	// Обрабатываем таблицу импорта
	//

	printf("\nProcessing import\n");

	
	if( !LdrpProcessImport( base ) )
	{
		VirtualFree( base, poh->SizeOfImage, MEM_DECOMMIT );
		VirtualFree( base, poh->SizeOfImage, MEM_RELEASE );
		CloseHandle( hFile );
		return NULL;
	}
	

	//
	// Выставляем атрибуты секций
	//

	printf("\nProtecting sections\n");

	if( !LdrpProtectSections( base ) )
	{
		VirtualFree( base, poh->SizeOfImage, MEM_DECOMMIT );
		VirtualFree( base, poh->SizeOfImage, MEM_RELEASE );
		CloseHandle( hFile );
		return NULL;
	}

	//
	// Все удачно, закрываем файл и возвращаем базу
	//
	// Важно! Файл не готов к запуску. Чтобы запустить этот образ
	//  нужно еще скорректировать поле PEB.ImageBaseAddress в PEB,
	//  иначе не будут работать функции Win32 для работы с ресурсами.
	//

	CloseHandle( hFile );
	printf("Executable loaded successfully\n");

	strcpy (LoadedModules [LoadedModCount].ModuleName, path);
	LoadedModules[LoadedModCount].ImageBase = Mapping;
	LoadedModules[LoadedModCount++].ImageSize = poh->SizeOfImage;

	return (HMODULE) Mapping;
}

void
LdrpAddModuleToModuleList (
  IN void* base,
  IN LIST_ENTRY* List
)
{
/*
	LIST_ENTRY * Last = List->Blink;

	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	GetHeaders((PCHAR) base, &pfh, &poh, &psh);

	LDR_MODULE * NewEntry = (LDR_MODULE *) HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( LDR_MODULE ) );
	
	NewEntry->InInitializationOrderModuleList.Flink = 
	NewEntry->InMemoryOrderModuleList.Flink =
	NewEntry->InLoadOrderModuleList.Flink = Last->Flink;

	NewEntry->InInitializationOrderModuleList.Flink->Blink = 
	NewEntry->InMemoryOrderModuleList.Flink->Blink =
	NewEntry->InLoadOrderModuleList.Flink->Blink = (LIST_ENTRY*) NewEntry;

	NewEntry->InInitializationOrderModuleList.Blink = 
	NewEntry->InMemoryOrderModuleList.Blink =
	NewEntry->InLoadOrderModuleList.Blink = Last;
	Last->Flink = (LIST_ENTRY*) NewEntry;

	NewEntry->BaseAddress = base;
	NewEntry->LoadCount = 1;
	NewEntry->EntryPoint = (PVOID)RVATOVA(poh->AddressOfEntryPoint, base);
	NewEntry->SizeOfImage = poh->SizeOfImage;
	RtlInitUnicodeString( &NewEntry->BaseDllName, L"testdll.dll" );
	RtlInitUnicodeString( &NewEntry->FullDllName, L"D:\\Progs\\fixup\\testdll.dll" );
	NewEntry->TimeDateStamp = pfh->TimeDateStamp;
*/
}

void
LdrpFixPeb (
  IN void* base
  )
/*
	Подготовить PEB процесса к запуску
*/
{
	// Фиксим PEB.ImageBaseAddress
	PPEB peb = GetPEB( );
//	peb->ImageBaseAddress = base;

	// Добавляем модуль в PEB.Ldr.In...OrderModuleList
	/*RtlEnterCriticalSection( peb->LoaderLock );

	LdrpAddModuleToModuleList( base, &peb->Ldr->InLoadOrderModuleList );
	LdrpAddModuleToModuleList( base, &peb->Ldr->InInitializationOrderModuleList );
	LdrpAddModuleToModuleList( base, &peb->Ldr->InMemoryOrderModuleList );

	RtlLeaveCriticalSection( peb->LoaderLock );
	*/

	__asm nop;
}

typedef struct  _DRVFN  /* drvfn */
{
    ULONG   iFunc;
    PVOID     pfn;
} DRVFN, *PDRVFN;

typedef struct  tagDRVENABLEDATA
{
    ULONG   iDriverVersion;
    ULONG   c;
    DRVFN  *pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;


PMODULE32 LdrLookupModByPointer (PVOID Pointer)
{
	for (ULONG i=0; i<LoadedModCount; i++)
	{
		if ( (ULONG)Pointer >= (ULONG)LoadedModules[i].ImageBase &&
			 (ULONG)Pointer <  (ULONG)LoadedModules[i].ImageBase + LoadedModules[i].ImageSize )
		{
			return &LoadedModules[i];
		}
	}
	return NULL;
}


void
LdrCallLoadedImage (
  IN HMODULE hModule
  )
/*
	Запускает загруженный образ
*/
{
	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	GetHeaders((PCHAR) hModule, &pfh, &poh, &psh);

	// 
	// Фиксим PEB
	//
	printf("\nPreparing to start image, fixing PEB\n");

	LdrpFixPeb( hModule );

	// Все ок. Запускаем
	printf("Ready. Running....\n");
	LPVOID entry = (LPVOID)( (DWORD)hModule + poh->AddressOfEntryPoint );

BOOL (APIENTRY *DrvEnableDriver)(
    ULONG          iEngineVersion,
    ULONG          cj,
    DRVENABLEDATA *pded
    );

	*(PVOID*)&DrvEnableDriver = entry;

	DRVENABLEDATA ded = {0};
	BOOL b = DrvEnableDriver (0x00030101, sizeof(ded), &ded);


	printf("DrvEnableDriver returned %d\n", b);

	if (!b)
		Sleep(-1);

	printf("DISPLAY DRIVER LOADED\n");
	printf("===============================================================\n"
		   "Dumping entries                   \n"
		   "Index		Entry		Relative	Module    \n"
		   "===============================================================\n");

	for (ULONG i=0; i<ded.c; i++)
	{
		char *modname = "";

		PMODULE32 mod = LdrLookupModByPointer (ded.pdrvfn[i].pfn);
		if (mod)
		{
			modname = strrchr (mod->ModuleName, '\\');
			if (modname)
				modname ++;
			else
				modname = "";
		}

		printf("%2d		%08x	%08x	%s\n", 
			ded.pdrvfn[i].iFunc, 
			ded.pdrvfn[i].pfn,
			mod ? ((ULONG)ded.pdrvfn[i].pfn - (ULONG)mod->ImageBase) : 0,
			modname);
	}

	Sleep(-1);
}

void
LdrUnloadExecutable (
  IN HMODULE hModule
  )
/*
	Выгружает образ из памяти
*/
{
	PIMAGE_FILE_HEADER		pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	GetHeaders((PCHAR) hModule, &pfh, &poh, &psh);

	VirtualFree( hModule, poh->SizeOfImage, MEM_DECOMMIT );
	VirtualFree( hModule, poh->SizeOfImage, MEM_RELEASE );
}


extern "C" BOOLEAN
LdrpCheckForLoadedDll (
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN StaticLink,
    IN BOOLEAN Wx86KnownDll,
    OUT PLDR_MODULE *LdrDataTableEntry
    );

void *SysLoad (PCHAR DllName)
{
	static ULONG Base = 0x10000000;

	char name[1024];
	GetSystemDirectory (name, sizeof(name)-1);
	lstrcat (name, "\\");
	lstrcat (name, DllName);

	PVOID b = (PVOID) Base;
	PVOID ptr = LdrLoadExecutable (name, &b);

	Base += 0x1000000;

	return ptr;
}


int main()
{
	void* ptr = SysLoad ("igxprd32.dll");

//	__asm int 3
	LdrCallLoadedImage( (HINSTANCE)ptr );

	return 0;
}

VOID APIENTRY EngMultiByteToUnicodeN(
    LPWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
    )
{
	*BytesInUnicodeString = MultiByteToWideChar (CP_ACP, 0, MultiByteString, BytesInMultiByteString, 
		UnicodeString, MaxBytesInUnicodeString);
}

PVOID APIENTRY EngLoadImage(PWCHAR Name)
{
	char aName[512];
	WideCharToMultiByte (CP_ACP, 0, Name, wcslen(Name)+1, aName, sizeof(aName)-1, 0, 0);

	if (!stricmp(aName, "win32k.sys"))
		return GetModuleHandle(0);

	return SysLoad (aName);
}

PVOID
RtlFindImageProcedureByNameOrPointer(
	IN PVOID Base,
	IN PCHAR FunctionName OPTIONAL,
	IN PVOID FunctionEntry OPTIONAL
	)
{
	PIMAGE_DOS_HEADER mz;
	PIMAGE_FILE_HEADER pfh;
	PIMAGE_OPTIONAL_HEADER poh;
	PIMAGE_EXPORT_DIRECTORY pexd;
	PULONG AddressOfFunctions;
	PULONG AddressOfNames;
	PUSHORT AddressOfNameOrdinals;
	ULONG i;

	
	// Get headers
	*(PUCHAR*)&mz = (PUCHAR)Base;
	*(PUCHAR*)&pfh = (PUCHAR)Base + mz->e_lfanew + sizeof(IMAGE_NT_SIGNATURE);
	*(PIMAGE_FILE_HEADER*)&poh = pfh + 1;

	// Get export
	*(PUCHAR*)&pexd = (PUCHAR)Base + poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	*(PUCHAR*)&AddressOfFunctions = (PUCHAR)Base + pexd->AddressOfFunctions;
	*(PUCHAR*)&AddressOfNames = (PUCHAR)Base + pexd->AddressOfNames;
	*(PUCHAR*)&AddressOfNameOrdinals = (PUCHAR)Base + pexd->AddressOfNameOrdinals;

	// Find function
	for( i=0; i<pexd->NumberOfNames; i++ ) 
	{
		PCHAR name = ((char*)Base + AddressOfNames[i]);
		PVOID addr = (PVOID*)((ULONG)Base + AddressOfFunctions[AddressOfNameOrdinals[i]]);

		if ( (FunctionName))
		{
			if( !strcmp( name, FunctionName ) ) 
			{
				return addr;
			}
		}
		else if ( (FunctionEntry))
		{
			if (FunctionEntry == addr)
				return name;
		}
		else
		{
			printf ("SHOULD NOT REACH HERE\n");
			__asm int 3
		}
	}
	
	return NULL;
}

PVOID APIENTRY EngFindImageProcAddress(PVOID Mod, PCHAR Func)
{
//	if (!Mod)
//		__asm int 3
//
//	if (!stricmp(Func,"DrvEnableDriver"))
//		__asm int 3

	return (PVOID) RtlFindImageProcedureByNameOrPointer (Mod, Func, 0);
}

void db()
{
	__asm int 3
}


void BRUSHOBJ_hGetColorTransform
() {db();} void BRUSHOBJ_pvAllocRbrush
() {db();} void BRUSHOBJ_pvGetRbrush
() {db();} void BRUSHOBJ_ulGetBrushColor
() {db();} void CLIPOBJ_bEnum
() {db();} void CLIPOBJ_cEnumStart
() {db();} void CLIPOBJ_ppoGetPath
() {db();} void EngAcquireSemaphore
() {db();} void EngAllocMem
() {db();} void EngAllocPrivateUserMem
() {db();} void EngAllocSectionMem
() {db();} void EngAllocUserMem
() {db();} void EngAlphaBlend
() {db();} void EngAssociateSurface
() {db();} void EngBitBlt
() {db();} void EngBugCheckEx
() {db();} void EngCheckAbort
() {db();} void EngClearEvent
() {db();} void EngComputeGlyphSet
() {db();} void EngControlSprites
() {db();} void EngCopyBits
() {db();} void EngCreateBitmap
() {db();} void EngCreateClip
() {db();} void EngCreateDeviceBitmap
() {db();} void EngCreateDeviceSurface
() {db();} void EngCreateDriverObj
() {db();} void EngCreateEvent
() {db();} void EngCreatePalette
() {db();} void EngCreatePath
() {db();} void EngCreateSemaphore
() {db();} void EngCreateWnd
() {db();} void EngDebugBreak
() {db();} void EngDebugPrint
() {db();} void EngDeleteClip
() {db();} void EngDeleteDriverObj
() {db();} void EngDeleteEvent
() {db();} void EngDeleteFile
() {db();} void EngDeletePalette
() {db();} void EngDeletePath
() {db();} void EngDeleteSafeSemaphore
() {db();} void EngDeleteSemaphore
() {db();} void EngDeleteSurface
() {db();} void EngDeleteWnd
() {db();} void EngDeviceIoControl
() {db();} void EngDitherColor
() {db();} void EngDxIoctl
() {db();} void EngEnumForms
() {db();} void EngEraseSurface
() {db();} void EngFileIoControl
() {db();} void EngFileWrite
() {db();} void EngFillPath
() {db();} void EngFindResource
() {db();} void EngFntCacheAlloc
() {db();} void EngFntCacheFault
() {db();} void EngFntCacheLookUp
() {db();} void EngFreeMem
() {db();} void EngFreeModule
() {db();} void EngFreePrivateUserMem
() {db();} void EngFreeSectionMem
() {db();} void EngFreeUserMem
() {db();} void EngGetCurrentCodePage
() {db();} void EngGetCurrentProcessId
() {db();} void EngGetCurrentThreadId
() {db();} void EngGetDriverName
() {db();} void EngGetFileChangeTime
() {db();} void EngGetFilePath
() {db();} void EngGetForm
() {db();} void EngGetLastError
() {db();} void EngGetPrinter
() {db();} void EngGetPrinterData
() {db();} void EngGetPrinterDataFileName
() {db();} void EngGetPrinterDriver
() {db();} void EngGetProcessHandle
() {db();} void EngGetTickCount
() {db();} void EngGetType1FontList
() {db();} void EngGradientFill
() {db();} void EngHangNotification
() {db();} 

typedef struct _ENGSAFESEMAPHORE {
  HANDLE  hsem;
  LONG  lCount;
} ENGSAFESEMAPHORE;

BOOL APIENTRY EngInitializeSafeSemaphore(
	ENGSAFESEMAPHORE *sem
	)
{
	sem->hsem = (HANDLE)1;
	sem->lCount = 0;
	return 1;
}

void EngIsSemaphoreOwned
() {db();} void EngIsSemaphoreOwnedByCurrentThread
() {db();} void EngLineTo
() {db();} void EngLoadModule
() {db();} void EngLoadModuleForWrite
() {db();} void EngLockDirectDrawSurface
() {db();} void EngLockDriverObj
() {db();} void EngLockSurface
() {db();} void EngLpkInstalled
() {db();} void EngMapEvent
() {db();} void EngMapFile
() {db();} void EngMapFontFile
() {db();} void EngMapFontFileFD
() {db();} void EngMapModule
() {db();} void EngMapSection
() {db();} void EngMarkBandingSurface
() {db();} void EngModifySurface
() {db();} void EngMovePointer
() {db();} void EngMulDiv
() {db();} void EngMultiByteToWideChar
() {db();} void EngNineGrid
() {db();} void EngPaint
() {db();} void EngPlgBlt
() {db();} void EngProbeForRead
() {db();} void EngProbeForReadAndWrite
() {db();} void EngQueryDeviceAttribute
() {db();} void EngQueryLocalTime
() {db();} void EngQueryPalette
() {db();} void EngQueryPerformanceCounter
() {db();} void EngQueryPerformanceFrequency
() {db();} void EngQuerySystemAttribute
() {db();} void EngReadStateEvent
() {db();} void EngReleaseSemaphore
() {db();} void EngRestoreFloatingPointState
() {db();} void EngSaveFloatingPointState
() {db();} void EngSecureMem
() {db();} void EngSetEvent
() {db();} void EngSetLastError
() {db();} void EngSetPointerShape
() {db();} void EngSetPointerTag
() {db();} void EngSetPrinterData
() {db();} void EngSort
() {db();} void EngStretchBlt
() {db();} void EngStretchBltROP
() {db();} void EngStrokeAndFillPath
() {db();} void EngStrokePath
() {db();} void EngTextOut
() {db();} void EngTransparentBlt
() {db();} void EngUnicodeToMultiByteN
() {db();} void EngUnloadImage
() {db();} void EngUnlockDirectDrawSurface
() {db();} void EngUnlockDriverObj
() {db();} void EngUnlockSurface
() {db();} void EngUnmapEvent
() {db();} void EngUnmapFile
() {db();} void EngUnmapFontFile
() {db();} void EngUnmapFontFileFD
() {db();} void EngUnsecureMem
() {db();} void EngWaitForSingleObject
() {db();} void EngWideCharToMultiByte
() {db();} void EngWritePrinter
//() {db();} void _except_handler2
() {db();} void FLOATOBJ_Add
() {db();} void FLOATOBJ_AddFloat
() {db();} void FLOATOBJ_AddFloatObj
() {db();} void FLOATOBJ_AddLong
() {db();} void FLOATOBJ_Div
() {db();} void FLOATOBJ_DivFloat
() {db();} void FLOATOBJ_DivFloatObj
() {db();} void FLOATOBJ_DivLong
() {db();} void FLOATOBJ_Equal
() {db();} void FLOATOBJ_EqualLong
() {db();} void FLOATOBJ_GetFloat
() {db();} void FLOATOBJ_GetLong
() {db();} void FLOATOBJ_GreaterThan
() {db();} void FLOATOBJ_GreaterThanLong
() {db();} void FLOATOBJ_LessThan
() {db();} void FLOATOBJ_LessThanLong
() {db();} void FLOATOBJ_Mul
() {db();} void FLOATOBJ_MulFloat
() {db();} void FLOATOBJ_MulFloatObj
() {db();} void FLOATOBJ_MulLong
() {db();} void FLOATOBJ_Neg
() {db();} void FLOATOBJ_SetFloat
() {db();} void FLOATOBJ_SetLong
() {db();} void FLOATOBJ_Sub
() {db();} void FLOATOBJ_SubFloat
() {db();} void FLOATOBJ_SubFloatObj
() {db();} void FLOATOBJ_SubLong
() {db();} void FONTOBJ_cGetAllGlyphHandles
() {db();} void FONTOBJ_cGetGlyphs
() {db();} void FONTOBJ_pfdg
() {db();} void FONTOBJ_pifi
() {db();} void FONTOBJ_pjOpenTypeTablePointer
() {db();} void FONTOBJ_pQueryGlyphAttrs
() {db();} void FONTOBJ_pvTrueTypeFontFile
() {db();} void FONTOBJ_pwszFontFilePaths
() {db();} void FONTOBJ_pxoGetXform
() {db();} void FONTOBJ_vGetInfo
//() {db();} void _global_unwind2
() {db();} void HeapVidMemAllocAligned
() {db();} void HT_ComputeRGBGammaTable
() {db();} void HT_Get8BPPFormatPalette
() {db();} void HT_Get8BPPMaskPalette
//() {db();} void _itoa
//() {db();} void _itow
//() {db();} void _local_unwind2
() {db();} void PALOBJ_cGetColors
() {db();} void PATHOBJ_bCloseFigure
() {db();} void PATHOBJ_bEnum
() {db();} void PATHOBJ_bEnumClipLines
() {db();} void PATHOBJ_bMoveTo
() {db();} void PATHOBJ_bPolyBezierTo
() {db();} void PATHOBJ_bPolyLineTo
() {db();} void PATHOBJ_vEnumStart
() {db();} void PATHOBJ_vEnumStartClipLines
() {db();} void PATHOBJ_vGetBounds
() {db();} void RtlAnsiCharToUnicodeChar
() {db();} void RtlMultiByteToUnicodeN
() {db();} void RtlRaiseException
() {db();} void RtlUnicodeToMultiByteN
() {db();} void RtlUnicodeToMultiByteSize
() {db();} void RtlUnwind
() {db();} void RtlUpcaseUnicodeChar
() {db();} void RtlUpcaseUnicodeToMultiByteN
() {db();} void STROBJ_bEnum
() {db();} void STROBJ_bEnumPositionsOnly
() {db();} void STROBJ_bGetAdvanceWidths
() {db();} void STROBJ_dwGetCodePage
() {db();} void STROBJ_fxBreakExtra
() {db();} void STROBJ_fxCharacterExtra
() {db();} void STROBJ_vEnumStart
() {db();} void VidMemFree
() {db();} void WNDOBJ_bEnum
() {db();} void WNDOBJ_cEnumStart
() {db();} void WNDOBJ_vSetConsumer
() {db();} void XFORMOBJ_bApplyXform
() {db();} void XFORMOBJ_iGetFloatObjXform
() {db();} void XFORMOBJ_iGetXform
() {db();} void XLATEOBJ_cGetPalette
() {db();} void XLATEOBJ_hGetColorTransform
() {db();} void XLATEOBJ_iXlate
() {db();} void XLATEOBJ_piVector
() {db();} 