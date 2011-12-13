#pragma once

#include <windows.h>
#include <winnt.h>

#define PE_MAX_DATA_SECTIONS 32

typedef struct _PE_DOS_DATA_
{
	IMAGE_DOS_HEADER header;
	DWORD			 stub_size;
	BYTE			 *stub;
} PEDOSDATA;

typedef struct _PE_SECTION_
{
    IMAGE_SECTION_HEADER  header;
    BYTE                 *data;
} PESECTION;

class CPEToolHelp  
{
    private:
		HANDLE m_hMem;
		DWORD  m_file_size;
		BYTE  *m_lpImage;

		PEDOSDATA	      m_dos;
		IMAGE_NT_HEADERS  m_hdr_nt;
		DWORD     m_sections_count;
		PESECTION m_sections[PE_MAX_DATA_SECTIONS];

		DWORD   m_ep;
		DWORD   m_LoadLibrary_addr;
		DWORD	m_GetProcAddress_addr;

    private:
		inline BYTE *allocate(DWORD _size);
		inline void free(BYTE *_lpMem);
		inline void free_loaded_sections(void);
		inline void find_api_calls(void);
		inline DWORD rva_to_offset(DWORD _rva);
		inline DWORD align_to( DWORD _size, DWORD _base_size );
		inline DWORD get_section_index( DWORD _rva );
    public:

		CPEToolHelp();
		~CPEToolHelp();

		BOOL LoadFile(char *_file);
		BOOL SaveFile(char *_file);

		DWORD Get_LoadLibraryAddr(void)
		{ 
			return this->m_LoadLibrary_addr; 
		}

		DWORD Get_GetProcAddressAddr(void)
		{ 
			return this->m_GetProcAddress_addr; 
		}

		DWORD Get_ExecEntryPoint(void)
		{ 
			return this->m_ep; 
		}

		void AddCodeSection( char *_name, BYTE *_section, DWORD _section_size, DWORD _entry_point_offset );
};

