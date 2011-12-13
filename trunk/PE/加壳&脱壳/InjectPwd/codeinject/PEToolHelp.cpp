#include "stdafx.h"
#include "PEToolHelp.h"

CPEToolHelp::CPEToolHelp()
{
   this->m_LoadLibrary_addr = 0;
   this->m_GetProcAddress_addr = 0;
   this->m_lpImage        = NULL;
   this->m_file_size      = 0;
   this->m_sections_count = 0;
   (void) memset( &this->m_dos, 0x00, sizeof(this->m_dos) );
   (void) memset( &this->m_sections, 0x00, sizeof(this->m_sections) );

   this->m_hMem  = ::HeapCreate( NULL, 10 * 1024 * 1024, 0 );
}

CPEToolHelp::~CPEToolHelp()
{
   (void) ::HeapDestroy(this->m_hMem);
}

BYTE *CPEToolHelp::allocate(DWORD _size)
{
    if ( !this->m_hMem ) 
        return (BYTE *)NULL;

    return (BYTE *)::HeapAlloc( this->m_hMem, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, _size );
}

void CPEToolHelp::free(BYTE *_lpMem)
{
    if ( !this->m_hMem ) 
        return;

    (void) ::HeapFree( this->m_hMem, HEAP_NO_SERIALIZE, _lpMem ); 
}

void CPEToolHelp::free_loaded_sections(void)
{
    DWORD count = 0;
    if ( !this->m_sections_count )
        return;

    for ( count = 0; count < this->m_sections_count; count++ )
        if ( this->m_sections[count].data )
            this->free(this->m_sections[count].data);

    this->m_sections_count = 0;
    (void) memset( &this->m_sections, 0x00, sizeof(this->m_sections) );
}

inline DWORD CPEToolHelp::align_to( DWORD _size, DWORD _base_size )
{
	return ( ((_size + _base_size-1) / _base_size) * _base_size );
}

void CPEToolHelp::find_api_calls(void)
{
	char *dllName = NULL;
	DWORD dll_import = 0;
	DWORD dwThunk = 0;
	const IMAGE_IMPORT_DESCRIPTOR *lpImp = NULL;
	const IMAGE_THUNK_DATA *itd = NULL;
	const IMAGE_IMPORT_BY_NAME *name_import = NULL;

	dll_import = this->rva_to_offset(this->m_hdr_nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if ( !dll_import )
	   return;

	lpImp = (IMAGE_IMPORT_DESCRIPTOR *)((DWORD)this->m_lpImage + dll_import);
	while ( lpImp->Name )
	{
		dllName = (char *)((DWORD)(DWORD)this->m_lpImage + this->rva_to_offset(lpImp->Name)); 
		if ( stricmp( dllName, "kernel32.dll" ) )
		{
			lpImp++;
			continue;
		}

		dwThunk = (lpImp->OriginalFirstThunk ? lpImp->OriginalFirstThunk : lpImp->FirstThunk);
		itd = (IMAGE_THUNK_DATA *)((DWORD)this->m_lpImage + this->rva_to_offset(dwThunk));    
		dwThunk = lpImp->FirstThunk;
		while ( itd->u1.AddressOfData )
		{
			name_import = (IMAGE_IMPORT_BY_NAME *)((DWORD)this->m_lpImage + this->rva_to_offset((DWORD)itd->u1.AddressOfData));    

			if ( !stricmp( (char *)name_import->Name, "LoadLibraryA" ) )
				this->m_LoadLibrary_addr = this->m_hdr_nt.OptionalHeader.ImageBase + dwThunk;
			else if ( !stricmp( (char *)name_import->Name, "GetProcAddress" ) )
				this->m_GetProcAddress_addr = this->m_hdr_nt.OptionalHeader.ImageBase + dwThunk;
			
			itd++;
			dwThunk += sizeof(DWORD);
		}

		break;
	}

	return;
}

DWORD CPEToolHelp::get_section_index( DWORD _rva )
{
   DWORD iC = 0;

   for ( iC = 0; iC < this->m_sections_count; iC++ )
   {
		if( this->m_sections[iC].header.VirtualAddress && _rva <= (this->m_sections[iC].header.VirtualAddress + this->m_sections[iC].header.SizeOfRawData) )
			return iC;
   }

   return -1;
}

DWORD CPEToolHelp::rva_to_offset(DWORD _rva)
{
   BOOL bFound = FALSE;
   DWORD iC = 0;

   for ( iC = 0; iC < this->m_sections_count; iC++ )
   {
		if( this->m_sections[iC].header.VirtualAddress && _rva <= (this->m_sections[iC].header.VirtualAddress + this->m_sections[iC].header.SizeOfRawData) )
		{
			bFound = TRUE;
			break;
		}
   }

   if ( !bFound ) 
      return (DWORD)NULL;

   return (_rva + this->m_sections[iC].header.PointerToRawData - this->m_sections[iC].header.VirtualAddress);
}

BOOL CPEToolHelp::LoadFile(char *_file)
{
	HANDLE hFile = NULL;
    DWORD iC                         = 0;
    DWORD dwBytesRead                = 0;
    IMAGE_SECTION_HEADER *lpSection  = NULL;

    if ( _file == NULL ) 
        return FALSE;

    if ( hFile )
        (void) ::CloseHandle(hFile);
    
    hFile = ::CreateFile( _file,
                           GENERIC_READ,
                           FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    this->m_file_size = ::GetFileSize(hFile, NULL);
    
    if ( this->m_file_size < sizeof(IMAGE_NT_HEADERS) )
        goto CPETH_LF_ERROR;
        
    this->m_lpImage = this->allocate(this->m_file_size);
    if ( !this->m_lpImage )
        goto CPETH_LF_ERROR;

    (void) ::ReadFile( hFile, this->m_lpImage, this->m_file_size, &dwBytesRead, NULL );     

    if ( dwBytesRead != this->m_file_size )
        goto CPETH_LF_ERROR;
      
	//
    //load DOS header
	//
    (void) memcpy( &this->m_dos.header, this->m_lpImage, sizeof(IMAGE_DOS_HEADER) ); 
    if( this->m_dos.header.e_magic != IMAGE_DOS_SIGNATURE )
        goto CPETH_LF_ERROR;

	if ( this->m_dos.stub )
		this->free(this->m_dos.stub);

	this->m_dos.stub_size = this->m_dos.header.e_lfanew - sizeof(IMAGE_DOS_HEADER);

	if ( this->m_dos.stub_size )
	{
		this->m_dos.stub = this->allocate(this->m_dos.stub_size);
		(void) memcpy( this->m_dos.stub, this->m_lpImage + sizeof(IMAGE_DOS_HEADER), this->m_dos.stub_size ); 
	}
	else
		this->m_dos.stub = NULL;		

	//
    //load NT headers
	//
   (void) memcpy( &this->m_hdr_nt, this->m_lpImage + this->m_dos.header.e_lfanew, sizeof(IMAGE_NT_HEADERS) ); 
   if( this->m_hdr_nt.Signature != IMAGE_NT_SIGNATURE )
      goto CPETH_LF_ERROR;

    //
    //load sections
   //
    this->free_loaded_sections();
    
    this->m_sections_count = this->m_hdr_nt.FileHeader.NumberOfSections;   

    lpSection = (IMAGE_SECTION_HEADER *)(this->m_lpImage + this->m_dos.header.e_lfanew + sizeof(IMAGE_NT_HEADERS));

    for ( iC = 0; iC < this->m_sections_count; iC++ )
    {
         (void) memcpy( &this->m_sections[iC].header, lpSection, sizeof(IMAGE_SECTION_HEADER) );
         this->m_sections[iC].data = this->allocate(this->align_to(this->m_sections[iC].header.SizeOfRawData, this->m_hdr_nt.OptionalHeader.FileAlignment));
         (void) memcpy( this->m_sections[iC].data, 
                       this->m_lpImage + this->m_sections[iC].header.PointerToRawData,
                       this->m_sections[iC].header.SizeOfRawData );   

         lpSection++;
    }

	this->m_ep = this->m_hdr_nt.OptionalHeader.AddressOfEntryPoint + this->m_hdr_nt.OptionalHeader.ImageBase; 

	this->find_api_calls(); 

   this->free(this->m_lpImage);
   this->m_lpImage = NULL;

   (void) ::CloseHandle(hFile);

   return TRUE;

CPETH_LF_ERROR:

    if ( this->m_lpImage )
        this->free(this->m_lpImage);

    (void) ::CloseHandle(hFile);

    return FALSE;
}

BOOL CPEToolHelp::SaveFile(char *_file)
{
	DWORD  iC = 0;
	HANDLE hFile = NULL;
    DWORD dwExecSize = 0;
	DWORD dwFirstSec = 0;
	DWORD dwBytesWritten = 0;
    IMAGE_SECTION_HEADER *lpSection  = NULL;

    if ( _file == NULL || this->m_sections_count == 0 ) 
        return FALSE;

    if ( hFile )
        (void) ::CloseHandle(hFile);
    
    hFile = ::CreateFile( _file,
        				  GENERIC_READ | GENERIC_WRITE,
					      FILE_SHARE_WRITE | FILE_SHARE_READ,
	                      NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;
	
	dwExecSize = this->m_sections[this->m_sections_count-1].header.PointerToRawData +
				 this->m_sections[this->m_sections_count-1].header.SizeOfRawData;
	

	this->m_lpImage = this->allocate(dwExecSize);

	//
	//Align sections
	//
	for( iC = 0; iC < this->m_sections_count; iC++ )
	{
		this->m_sections[iC].header.VirtualAddress   = this->align_to(this->m_sections[iC].header.VirtualAddress, this->m_hdr_nt.OptionalHeader.SectionAlignment);
		this->m_sections[iC].header.PointerToRawData = this->align_to(this->m_sections[iC].header.PointerToRawData, this->m_hdr_nt.OptionalHeader.FileAlignment);
		this->m_sections[iC].header.SizeOfRawData    = this->align_to(this->m_sections[iC].header.SizeOfRawData, this->m_hdr_nt.OptionalHeader.FileAlignment);
	}


	this->m_hdr_nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0 ;
	this->m_hdr_nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;
	this->m_hdr_nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = 0;
	this->m_hdr_nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = 0;

	this->m_hdr_nt.OptionalHeader.SizeOfImage = this->m_sections[this->m_sections_count-1].header.VirtualAddress + this->m_sections[this->m_sections_count-1].header.Misc.VirtualSize;

	//
	//Copy DOS Info
	//
	(void) memcpy( this->m_lpImage, &this->m_dos.header, sizeof(IMAGE_DOS_HEADER) );  

	//
	//Copy DOS Stub
	//
	if ( this->m_dos.stub_size )
		(void) memcpy( this->m_lpImage + sizeof(IMAGE_DOS_HEADER), this->m_dos.stub, this->m_dos.stub_size );

	//
	//Copy NT Headers
	//
	(void) memcpy( this->m_lpImage + this->m_dos.header.e_lfanew, &this->m_hdr_nt, sizeof(IMAGE_NT_HEADERS));

	//
	//Copy Sections Headers
	//
	dwFirstSec = this->m_dos.header.e_lfanew + sizeof(IMAGE_NT_HEADERS);

	for( iC = 0; iC < this->m_sections_count ;iC++ )
		(void) memcpy( this->m_lpImage + dwFirstSec + iC * sizeof(IMAGE_SECTION_HEADER), 
					   &this->m_sections[iC].header, 
					   sizeof(IMAGE_SECTION_HEADER) );

	//
	//Copy Sections Data
	//
	for( iC = 0; iC < this->m_sections_count ;iC++ )
		(void) memcpy( this->m_lpImage + this->m_sections[iC].header.PointerToRawData,
				this->m_sections[iC].data,
				this->m_sections[iC].header.SizeOfRawData );

	(void) ::WriteFile( hFile, this->m_lpImage, dwExecSize, &dwBytesWritten, NULL );
	(void) ::FlushFileBuffers(hFile); 
    (void) ::CloseHandle(hFile);

    if ( this->m_lpImage )
        this->free(this->m_lpImage);

    return TRUE;
}

void CPEToolHelp::AddCodeSection( char *_name, BYTE *_section, DWORD _section_size, DWORD _entry_point_offset )
{
	DWORD idx = this->m_sections_count; 
	DWORD dwSectionSize = 0;

	if ( _name == NULL || _section == NULL )
		return;

	dwSectionSize = _section_size;

	this->m_sections[idx].data = this->allocate( this->align_to(dwSectionSize, this->m_hdr_nt.OptionalHeader.FileAlignment) );

	this->m_sections[idx].header.PointerToRawData = this->align_to( this->m_sections[idx-1].header.PointerToRawData + this->m_sections[idx-1].header.SizeOfRawData, 
																	this->m_hdr_nt.OptionalHeader.FileAlignment ); 

	this->m_sections[idx].header.VirtualAddress   = this->align_to( this->m_sections[idx-1].header.VirtualAddress + this->m_sections[idx-1].header.Misc.VirtualSize, 
																	this->m_hdr_nt.OptionalHeader.SectionAlignment ); 


	this->m_sections[idx].header.SizeOfRawData	  = this->align_to( dwSectionSize, this->m_hdr_nt.OptionalHeader.FileAlignment );
	this->m_sections[idx].header.Misc.VirtualSize = dwSectionSize;
	this->m_sections[idx].header.Characteristics  = 0xE0000040;

	(void) memcpy( this->m_sections[idx].header.Name, _name, (size_t)strlen(_name) );

	this->m_hdr_nt.FileHeader.NumberOfSections++;
	this->m_sections_count++; 

	(void) memcpy( this->m_sections[idx].data, _section, _section_size );

	this->m_hdr_nt.OptionalHeader.AddressOfEntryPoint = this->m_sections[idx].header.VirtualAddress + _entry_point_offset; 
}

