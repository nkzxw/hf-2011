#include "comhdr.h"

CKl_HookModule::CKl_HookModule(PVOID base /* = NULL */) : m_base((char*)base)
{    
    if ( m_base )
        m_size = GetModuleSize(m_base);
}

CKl_HookModule::CKl_HookModule(PCHAR ModuleName)
{
    m_base = (char*)GetBase(ModuleName);

    if ( m_base )
        m_size = GetModuleSize( m_base );
}

// CKl_HookModule::~CKl_HookModule()
// {
// 
// }
#ifndef USER_MODE

PVOID
CKl_HookModule::GetBase(
	char* ModuleName
	)
{
    PSYSTEM_INFO_DRIVERS	pInfoBuff = NULL;
    ULONG	dwBuffSize;
    BOOLEAN	bNotenough;
    ULONG	ModuleNameLength = (ULONG)strlen ( ModuleName );
   
    PVOID	ModuleBase = NULL;
    ULONG	ModuleSize = 0;
        
    dwBuffSize  = 0x8000;	// Exactly as it is in NTDLL.DLL
    bNotenough  = TRUE;
   
    while( bNotenough )
    {
        if ( dwBuffSize > 0x8000 * 20 ) // Too much, hopeless :(
            return NULL;
        
        if ( pInfoBuff = (PSYSTEM_INFO_DRIVERS)kmalloc ( dwBuffSize ) )
        {
            if ( STATUS_INFO_LENGTH_MISMATCH == ZwQuerySystemInformation( (ULONG)0x0b, pInfoBuff, dwBuffSize, NULL ) ) 
            {
                dwBuffSize += 0x8000;
                kfree ( pInfoBuff );
            } 
            else 
                bNotenough = FALSE;
        }
        else        
            return NULL;
    }

    for ( ULONG i = 0; i < pInfoBuff->NumberOfDrivers; ++i ) 
    {
        ULONG   len = (ULONG)strlen ( pInfoBuff->Drivers[i].PathName );
        
        if ( !_strnicmp(pInfoBuff->Drivers[i].PathName + len - ModuleNameLength, ModuleName, ModuleNameLength ) )
        {
            ModuleBase = (char*)pInfoBuff->Drivers[i].BaseAddress;
            break; 
        }
    }	
    
    if ( pInfoBuff )
        kfree(pInfoBuff);
    
    if ( 0 == strcmp( ModuleName, "ntoskrnl.exe" ) )
    {
        
        BOOLEAN     bExit = FALSE;
        DWORD_PTR   OsBase;
        
        // ne ispolzovat MM_LOWEST_SYSTEM_ADDRESS or SYSTEM_BASE - exist different defines - 
        //#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0x80000000 and 
        //#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0800000

#ifndef _WIN64
#define _os_min_base 0x80000000
#else
#define _os_min_base ((DWORD_PTR)MM_LOWEST_SYSTEM_ADDRESS)
#endif        
        OsBase = (DWORD_PTR)IoAllocateIrp;
        OsBase &= ~0xFFF;
        
        while ( !bExit )
        {
            __try
            {
                if ( *(short*)OsBase == 'ZM')
                {
                    bExit = TRUE;
                }
                else
                {
                    OsBase -= PAGE_SIZE;
                }
                
                if ( OsBase <= _os_min_base )
                {
                    OsBase = 0;
                    bExit = TRUE;
                }
                
                
            } __except(EXCEPTION_EXECUTE_HANDLER)
            {
                OsBase = 0;
                bExit = TRUE;
            }
        }
        
        m_base = (char*)OsBase;

        if (bExit == TRUE && OsBase != 0)
        {
            if( (GetExportByName("IoCallDriver") ) )
            {
                if ( ContainPtr((PVOID)IoAllocateIrp) )
                    return m_base;
            }
        }
        
        m_base = NULL;
    }

    return ModuleBase;
}
#endif // USER_MODE

bool
CKl_HookModule::ContainPtr( void* ptr )
{
    if ( m_base )
    {
        ULONG   size = GetModuleSize( m_base );

        if ( (DWORD_PTR)ptr - (DWORD_PTR)m_base <= size )
            return true;
        
        return false;
    }
    return false;
}

bool
CKl_HookModule::isPE()
{
    if ( m_base )
    {
        PIMAGE_DOS_HEADER pDosHeader  = (PIMAGE_DOS_HEADER)m_base;
    }
    
    return false;
}

ULONG
CKl_HookModule::GetModuleSize(
		PVOID ModuleBase
		)
{
    PIMAGE_DOS_HEADER	pDosHeader  = (PIMAGE_DOS_HEADER)ModuleBase;
    PIMAGE_NT_HEADERS	pNTHeader = (PIMAGE_NT_HEADERS)( (PCHAR)ModuleBase + pDosHeader->e_lfanew );
    
    return pNTHeader->OptionalHeader.SizeOfImage;
}

PVOID
CKl_HookModule::GetExportByName( char* fName )
{
    if ( m_base )
    {
        PVOID                   fAddr;
        PIMAGE_DOS_HEADER       pDosHeader;
        PIMAGE_NT_HEADERS       pNTHeader;
        PIMAGE_EXPORT_DIRECTORY exportDir;
        
        DWORD   *functions;
        WORD    *ordinals;
        DWORD   *name;

        pDosHeader  = (PIMAGE_DOS_HEADER)m_base;
        
        pNTHeader   = (PIMAGE_NT_HEADERS)( m_base + pDosHeader->e_lfanew );
        
        exportDir   = (PIMAGE_EXPORT_DIRECTORY)( m_base + pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
        
        functions   = (DWORD*) ( m_base + (DWORD_PTR)exportDir->AddressOfFunctions );
        ordinals    = (WORD*)  ( m_base + (DWORD_PTR)exportDir->AddressOfNameOrdinals );

        name        = (DWORD *) ( m_base + (DWORD_PTR)exportDir->AddressOfNames );
       
        for ( DWORD i = exportDir->Base; i < exportDir->NumberOfFunctions; ++i ) 
        {
            if  ( functions[i] == 0 )   // Skip over gaps in exported function
                continue;               // ordinals (the entrypoint is 0 for these
            
            if ( 0 == strcmp ( m_base + (DWORD_PTR)name[ i ], fName ) )
            {
                fAddr = m_base + functions[ ordinals[i] ];                
                return fAddr;
            }
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------------------------


CKl_ModLoader::CKl_ModLoader(PVOID ModuleLoadBase /* = NULL */) : m_base( (char*)ModuleLoadBase )
{    
}

// CKl_ModLoader::~CKl_ModLoader()
// {
//     if ( m_DriverBase )
//         kfree( m_DriverBase );
// }

void
CKl_ModLoader::PrepareForExec(PVOID *StartEntry)
{
    unsigned int    Idx;
	DWORD_PTR		RelocDelta;
    ULONG           NumRelocs, RelocSize;
    DWORD           CurrentSize, TotalRelocs;
    
    PULONG                  PEMagic;
    PIMAGE_DOS_HEADER       PEDosHeader;
    PIMAGE_FILE_HEADER      PEFileHeader;
    PIMAGE_OPTIONAL_HEADER  PEOptionalHeader;
    PIMAGE_SECTION_HEADER   PESectionHeaders;
    PRELOCATION_DIRECTORY   RelocDir;
    PRELOCATION_ENTRY       RelocEntry;
    
    PVOID                   *ImportAddressList;
    PDWORD_PTR              FunctionNameList;
    PCHAR                   pName;
    WORD                    Hint;

    *StartEntry      = NULL;
    
    /*  If MZ header exists  */
    PEDosHeader      = (PIMAGE_DOS_HEADER) m_base;
    
    PEMagic          = (PULONG)                 ( m_base + PEDosHeader->e_lfanew );
    
    PEFileHeader     = (PIMAGE_FILE_HEADER)     ( m_base + PEDosHeader->e_lfanew + sizeof(ULONG) );
    
    PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ( m_base + PEDosHeader->e_lfanew + sizeof(ULONG) 
                                                                                 + sizeof(IMAGE_FILE_HEADER) );
    
    PESectionHeaders = (PIMAGE_SECTION_HEADER)  ( m_base + PEDosHeader->e_lfanew + sizeof(ULONG) 
                                                                                 + sizeof(IMAGE_FILE_HEADER) 
                                                                                 + sizeof(IMAGE_OPTIONAL_HEADER));
    
    m_DriverBase = (char*)kmalloc( PEOptionalHeader->SizeOfImage );
    RtlZeroMemory( m_DriverBase, PEOptionalHeader->SizeOfImage );

    RtlCopyMemory( m_DriverBase, m_base, PEOptionalHeader->SizeOfHeaders );
    
    CurrentSize = 0;


    if ( PEDosHeader->e_magic == IMAGE_DOS_MAGIC && PEDosHeader->e_lfanew != 0L )
    {
        /*  Copy image sections into virtual section  */                
        for ( Idx = 0; Idx < PEFileHeader->NumberOfSections; ++Idx )
        {   
            //  Copy current section into current offset of virtual section
            if ( PESectionHeaders[Idx].Characteristics & (IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA) )
            {                        
                RtlCopyMemory(PESectionHeaders[Idx].VirtualAddress   + m_DriverBase,
                              PESectionHeaders[Idx].PointerToRawData + m_base,
                              PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData ? 
                              PESectionHeaders[Idx].SizeOfRawData    : PESectionHeaders[Idx].Misc.VirtualSize );
            }
            else
            {                        
                RtlZeroMemory(PESectionHeaders[Idx].VirtualAddress + m_base, PESectionHeaders[Idx].Misc.VirtualSize );             
            }
            CurrentSize += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize, PEOptionalHeader->SectionAlignment);
        }
    }

    kprintf("relocate base %p", g_Base2Relocation);
	RelocDelta  = (DWORD_PTR)g_Base2Relocation - (DWORD_PTR)PEOptionalHeader->ImageBase;
//    RelocDelta  = (DWORD_PTR)m_DriverBase - (DWORD_PTR)PEOptionalHeader->ImageBase;

    RelocDir    = (PRELOCATION_DIRECTORY) ( (PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) + m_DriverBase );
	RelocSize = PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

    TotalRelocs = 0;

    while ( TotalRelocs < CurrentSize && RelocDir->SizeOfBlock != 0 )
    {
        NumRelocs   = ( RelocDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY) ) / sizeof( WORD );
        
        RelocEntry  = (PRELOCATION_ENTRY) ( RelocDir + 1 );
        
        for ( Idx = 0; Idx < NumRelocs; ++Idx )
        {
            ULONG   Offset;
            ULONG   Type;
            PDWORD_PTR  RelocItem;
            
            Offset      = RelocEntry[Idx].TypeOffset & 0xfff;
            Type        = (RelocEntry[Idx].TypeOffset >> 12) & 0xf;
       //     RelocItem   = (PDWORD_PTR)( hm.m_base + RelocDir->VirtualAddress + Offset );
            RelocItem   = (PDWORD_PTR)( m_DriverBase + RelocDir->VirtualAddress + Offset );

#ifdef _WIN64                        
            if ( Type == IMAGE_REL_BASED_HIGHLOW ||  Type == IMAGE_REL_BASED_DIR64 )
#else
            if ( Type == IMAGE_REL_BASED_HIGHLOW )
#endif
            {
                (*RelocItem) += RelocDelta;
				
            }
            else 
            if ( Type != 0 )
            {
                // Error Here !                
                // break;                
            }
        }
        
        TotalRelocs += RelocDir->SizeOfBlock;
        RelocDir = (PRELOCATION_DIRECTORY)( (DWORD_PTR)RelocDir + RelocDir->SizeOfBlock );                
    }



    if ( PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress )
    {
        PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
        
        /*  Process each import module  */
        ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)(m_DriverBase + PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        
        while ( ImportModuleDirectory->dwRVAModuleName )
        {
            /*  Check to make sure that import lib is kernel  */
            PCHAR RVAModName = pName = m_DriverBase + ImportModuleDirectory->dwRVAModuleName;
            
            /*  Get the import address list  */
            ImportAddressList = (PVOID*) ( m_DriverBase +  ImportModuleDirectory->dwRVAFunctionAddressList );
            
            /*  Get the list of functions to import  */
            if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
            {
                FunctionNameList = (PDWORD_PTR) ( m_DriverBase + ImportModuleDirectory->dwRVAFunctionNameList );
            }
            else
            {
                FunctionNameList = (PDWORD_PTR) ( m_DriverBase + ImportModuleDirectory->dwRVAFunctionAddressList );
            }

            CKl_HookModule  hm( RVAModName );

            if ( 0 == _stricmp( "HAL.dll", RVAModName ) )
            {
                if ( hm.m_base == NULL )
                    hm.m_base = (char*)hm.GetBase("halmacpi.dll");
            }

            if ( hm.m_base == NULL )
            {
                hm.m_base = (char*)hm.GetBase("ntkrnlpa.exe");
                if ( hm.m_base == NULL )
                {
                    break;
                }
            }

            /*  Walk through function list and fixup addresses  */
            while ( *FunctionNameList != 0L )
            {
                if ((*FunctionNameList) & 0x80000000) // hint
                {
                    pName = NULL;
                    Hint = (WORD)((*FunctionNameList) & 0xffff);
                }
                else // hint-name
                {
                    pName = (PCHAR)( m_DriverBase + *FunctionNameList + 2   );
                    Hint = *(PWORD)( m_DriverBase + *FunctionNameList       );
                }                    
                
                /*  Fixup the current import symbol  */               
                
                if ( NULL == (*ImportAddressList = hm.GetExportByName(pName)) )
                {
                    // Error Here !!!
                                        
                    break;
                    
                }
                
                if (*ImportAddressList == NULL)
                {
                    // Error Here !!!
                    // Maybe ntoskrnl.exe ??                    
                    break;
                }
                
                ++ImportAddressList;
                ++FunctionNameList;
            }
            
            ++ImportModuleDirectory;
        }
    }
    
    *StartEntry = ( PEOptionalHeader->AddressOfEntryPoint + (PCHAR)m_DriverBase );
}
////

CKl_Object::CKl_Object() : m_nRefCount(0)
{
}

CKl_Object::~CKl_Object()
{
}

void* CKl_Object::operator new(size_t size)
{
    void*   Memory = kmalloc(size);
	
#ifdef TRACK_MEM
    //Dbg->Print(BIT_TRACK_MEM, LEVEL_INTERESTING, "Alloc %x bytes +(%x)\n", size, Memory );
#endif
	
    return Memory;
}

void CKl_Object::operator delete(void* p)
{    
    ASSERT ( p );
    if ( p )
    {
#ifdef TRACK_MEM
		//  Dbg->Print(BIT_TRACK_MEM, LEVEL_INTERESTING, "Free -(%x)\n", p );
#endif
        kfree( p );
    }
}

ULONG
CKl_Object::Ref()
{
    InterlockedIncrement( &m_nRefCount );
	
    return m_nRefCount;
}

ULONG
CKl_Object::Deref()
{
    ULONG   nRefCount = InterlockedDecrement( &m_nRefCount );
	
    if ( nRefCount == 0 )
    {
        delete this;
    }
	
    return nRefCount;
}
