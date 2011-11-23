

#include "comhdr.h"

char	*m_ModuleName;
char	*m_ModuleLoadBase;
wchar_t	*m_ModulePath;
CKl_ModLoader	*ldr;
PDRIVER_OBJECT	m_DriverObject;
////////////////////////////////////////////////////////////////////////////////
//º¯ÊýÉùÃ÷
wchar_t* 
GetModuleName(wchar_t* ModPath);
///////////////////////////////////////////////////////

ULONG	myloader(char*  Name, wchar_t* Path, PDRIVER_OBJECT	fakeDriverObject, BOOLEAN Boot ,BOOLEAN bExcute)
{   
    HANDLE              hFile;
    NTSTATUS            ntStatus;
    OBJECT_ATTRIBUTES   ObjAttr;	
    UNICODE_STRING      FileName;
    IO_STATUS_BLOCK     ioStatus;
    FILE_STANDARD_INFORMATION   fi;
    ULONG               FileSize = 0;
    wchar_t             Buf[1000];
	ULONG				loadbase=0;


    m_DriverObject	=	fakeDriverObject;
    if ( m_ModuleName = (char*)kmalloc ( strlen( Name) + 1 ) )
    {
        strcpy ( m_ModuleName, Name );
    }
    
    if ( m_ModulePath = (wchar_t*)kmalloc ( (wcslen( Path) + 1) << 1 ) )
    {
        wcscpy ( m_ModulePath, Path );            
    }
    
    if ( Boot )
    {
        swprintf ( Buf, L"\\SystemRoot\\System32\\%s", GetModuleName(m_ModulePath) );
    }
    else
    {
        swprintf ( Buf, L"\\??\\%s", m_ModulePath);
    }
    
    RtlInitUnicodeString(&FileName, Buf);
    
    InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	ntStatus = ZwCreateFile( 
					&hFile,  
					FILE_READ_DATA | SYNCHRONIZE, 
					&ObjAttr,  
					&ioStatus, 
					NULL,  
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ, 
					FILE_OPEN, 
					FILE_SYNCHRONOUS_IO_NONALERT, 
					NULL,
					0 );
    
    if ( STATUS_SUCCESS != ntStatus )
    {   
        if ( Boot )
        {
            swprintf ( Buf, L"\\??\\%s", m_ModulePath );
            
            RtlInitUnicodeString( &FileName, Buf );
        }
        
        ntStatus = ZwCreateFile( 
					&hFile,  
					FILE_READ_DATA | SYNCHRONIZE,
					&ObjAttr,  
					&ioStatus, 
					NULL,  
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ, 
					FILE_OPEN, 
					FILE_SYNCHRONOUS_IO_NONALERT, 
					NULL,
					0 );
    }
    
    
    if ( STATUS_SUCCESS == ntStatus )
    {
		ntStatus = ZwQueryInformationFile(
									hFile, 
									&ioStatus, 
									&fi, 
									sizeof (fi), 
									FileStandardInformation);
		
        if ( NT_SUCCESS( ntStatus ) )
        {
            FileSize =  fi.EndOfFile.LowPart;
        }
        
        if ( FileSize )
        {
            if ( m_ModuleLoadBase = (char*)kmalloc( FileSize ) ) 
            {   
				ntStatus = ZwReadFile( 
									hFile, 
									NULL, 
									NULL, 
									NULL, 
									&ioStatus, 
									m_ModuleLoadBase, 
									FileSize, 
									0, 
									NULL);

				if ( NT_SUCCESS ( ntStatus ) )
                {

						ldr = new CKl_ModLoader( m_ModuleLoadBase );
						
						if ( ldr )
						{
							
					//		InitDriverObject();
					//		__asm int 3
							if (bExcute)
							{
								ldr->PrepareForExec( (PVOID*)&m_DriverObject->DriverInit );
								m_DriverObject->DriverInit ( m_DriverObject, NULL);
							}
							else
							{
								
								ULONG utmp;
								ldr->PrepareForExec( (PVOID*)&utmp );
							}
							loadbase	=	(ULONG)ldr->m_DriverBase;
							kprintf("init baseAddress :%x", ldr->m_DriverBase);

						}
          
                }
				kfree(m_ModuleLoadBase);
            }
        }
        
        ZwClose( hFile );

    }
	return loadbase;
}

wchar_t* 
GetModuleName(wchar_t* ModPath)
{
    wchar_t* ModName = ModPath;    
	
    if ( ModPath )
    {   
        ModName += wcslen( ModPath );
        while ( ( ModName != ModPath ) && ( *ModName != L'\\' ) )
            ModName--;
		
        if ( *ModName == L'\\')
            ModName++;
    }
	
    return ModName;
}

