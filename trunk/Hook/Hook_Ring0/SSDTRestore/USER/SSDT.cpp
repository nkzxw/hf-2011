#include "SSDT.h"

ULONG NTBase = 0;			//NT ��ַ
ULONG TotalSSDTCount = -1;		//SSDT�������
pSSDTSaveTable pSSDTST = NULL;	//�����SSDT�����
PSYSMODULELIST pList = NULL;	//ģ����Ϣ����
ULONG RealCount = 0;		//ö�����ҵ��ķ������

void GetOldSSDTAddress();

pNtQuerySystemInformationProto pNtQuerySystemInformation = NULL;

HANDLE LoadDriver( 
	IN LPCTSTR lpFileName 
	)
{
		HANDLE hDriver = INVALID_HANDLE_VALUE;
		char OpenName[MAX_PATH+1];
		sprintf( OpenName, "\\\\.\\%s", EXE_DRIVER_NAME );
		SC_HANDLE hSCManager = OpenSCManager( NULL, NULL,SC_MANAGER_CREATE_SERVICE );
		if ( NULL != hSCManager )
		{
		    SC_HANDLE hService = CreateService( hSCManager, 
		    																		EXE_DRIVER_NAME,
																		        DISPLAY_NAME, 
																		        SERVICE_START,
																		        SERVICE_KERNEL_DRIVER, 
																		        SERVICE_DEMAND_START,
																		        SERVICE_ERROR_IGNORE, 
																		        lpFileName, 
																		        NULL, 
																		        NULL, 
																		        NULL, 
																		        NULL, 
																		        NULL);
		    if ( ERROR_SERVICE_EXISTS == GetLastError() )
		    {
		        hService = OpenService( hSCManager, EXE_DRIVER_NAME, SERVICE_START );
		    }
		    
		    if( !StartService( hService, 0, NULL ) )
				{
						if( GetLastError() != 1056 )	//�Ѿ�����
						{
								//TODO
						}
						else{
								//TODO
						}
				}
		  
		    CloseServiceHandle( hService );
		    CloseServiceHandle( hSCManager );
		    hDriver = CreateFileA( OpenName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
				if( hDriver == INVALID_HANDLE_VALUE )
				{
						//TODO
				}
		}
		return hDriver;
}

void UnloadDriver( 
	IN HANDLE hDriver 
	)
{
    CloseHandle( hDriver );

    SC_HANDLE hSCManager = OpenSCManager( NULL, NULL,SC_MANAGER_CREATE_SERVICE );
    if ( NULL != hSCManager )
    {
        SC_HANDLE hService = OpenService( hSCManager, EXE_DRIVER_NAME, DELETE | SERVICE_STOP );
        if ( NULL != hService )
        {
            SERVICE_STATUS ss;
            ControlService( hService, SERVICE_CONTROL_STOP, &ss );
            DeleteService( hService );
            CloseServiceHandle( hService );
        }
        CloseServiceHandle( hSCManager );
    }
}

//�õ�SSDT
BOOL GetSSDT( 
	IN HANDLE hDriver, 
	OUT PSSDT ssdt 
	)
{
	if( ssdt == NULL )
	{
		return FALSE;
	}
	DWORD dwRet;
	BOOL bRet = DeviceIoControl( hDriver, IOCTL_GETSSDT, NULL, 0, ssdt, sizeof( SSDT ), &dwRet, NULL );
	if( bRet )
	{
		TotalSSDTCount = ssdt->ulNumberOfServices;		//��ȡ�������
		return TRUE;
	}
	return FALSE;
}

BOOL SetSSDT( 
	IN HANDLE hDriver, 
	IN PSSDT ssdt 
	)
{
	if( ssdt == NULL )
	{
		return FALSE;
	}
	
	DWORD dwRet;
	BOOL bRet = DeviceIoControl( hDriver, IOCTL_SETSSDT, ssdt, sizeof( SSDT ), NULL, 0, &dwRet, NULL );
	if( bRet )
	{
		TotalSSDTCount = ssdt->ulNumberOfServices;		//��ȡ�������
		return TRUE;
	}
	return FALSE;
}

BOOL GetHook( 
	IN HANDLE hDriver, 
	IN ULONG ulIndex, 
	OUT PULONG ulAddr 
	)
{
	if( ulAddr == NULL )
	{
		return FALSE;
	}
	
	DWORD dwRet;
	BOOL bRet = DeviceIoControl( hDriver, IOCTL_GETHOOK, &ulIndex, sizeof( ULONG ), ulAddr, sizeof( ULONG ), &dwRet, NULL );
	return bRet;
}

BOOL SetHook( 
	IN HANDLE hDriver, 
	IN ULONG ulIndex, 
	IN OUT PULONG ulAddr 
	)
{
	if( ulAddr == NULL )
	{
		return FALSE;
	}
	DWORD dwRet;
	BOOL bRet = DeviceIoControl( hDriver, IOCTL_SETHOOK, &ulIndex, sizeof( ULONG ), \
		ulAddr, sizeof( ULONG ), &dwRet, NULL );
	return bRet;
}

//����ģ���б�
PSYSMODULELIST CreateModList( 
	OUT PULONG ulNtBase 
	)
{
		HINSTANCE hNTDll;
		ULONG nRet;
		ULONG nQuerySize;
		ULONG Success;
		PSYSMODULELIST pModInfo = NULL;

		do
		{
			if( !( hNTDll = LoadLibrary( "ntdll.dll" ) ) )
			{
				break;
			}
			
			if( pNtQuerySystemInformation == NULL )
			{
				pNtQuerySystemInformation = (pNtQuerySystemInformationProto)GetProcAddress( hNTDll, "NtQuerySystemInformation" );
				if(!pNtQuerySystemInformation )
				{
					break;
				}
			}

			Success = pNtQuerySystemInformation(SystemModuleInfo, 
																					NULL, 
																					0, 
																					&nQuerySize
																					);
																					
			pModInfo = (PSYSMODULELIST)malloc( nQuerySize );
			if( !pModInfo )
			{
				break;
			}
			
			Success = pNtQuerySystemInformation(SystemModuleInfo, 
																					pModInfo, 
																					nQuerySize, 
																					&nRet
																					);
			if( Success < 0 )
			{
				free( pModInfo );
				pModInfo = NULL;
				break;
			}
		
			*ulNtBase = (ULONG)(pModInfo->smi->Base);	//˳��õ�NT��ַ(ntoskrnl.exe���ں��еļ��ػ�ַ) ^_^
		
	} while( FALSE );

	::FreeLibrary( hNTDll );

	return pModInfo;
}

void DestroyModList( 
	IN PSYSMODULELIST pList 
	)
{
	free( pList );
}

BOOL GetModuleNameByAddr( 
	IN ULONG ulAddr, 
	IN PSYSMODULELIST pList, 
	OUT LPSTR buf, 
	IN DWORD dwSize 
	)
{
		ULONG i;
		
		for ( i = 0; i < pList->ulCount; i++ )
		{
			ULONG ulBase = (ULONG)pList->smi[i].Base;
			ULONG ulMax  = ulBase + pList->smi[i].Size;
			if ( ulBase <= ulAddr && ulAddr < ulMax )
			{
				lstrcpynA( buf, pList->smi[i].ImageName, dwSize );
				return TRUE;
			}
		}
		
		return FALSE;
}


BOOL SSDTSTOrderByServiceNum( pSSDTSaveTable pSsdtST )
{
	ULONG ulMaxServiceNumber = 0;		//������е����ֵ
	ULONG i, j;
	//
	//�ҳ�����
	//
	for( i = 0; i < RealCount; i ++ )
	{
		ULONG ulCurServiceNum;
		ulCurServiceNum = ((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulServiceNumber;
		ulMaxServiceNumber = ulCurServiceNum > ulMaxServiceNumber? ulCurServiceNum : ulMaxServiceNumber;
	}
	
	//
	//��������������ú���û�з���ŵ�
	//
	for( i = RealCount; i < TotalSSDTCount; i ++ )
	{
		ulMaxServiceNumber ++;		
		((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulServiceNumber = \
			ulMaxServiceNumber;
	}
	
	//
	//�����������ð������
	//
	for( i = TotalSSDTCount - 1; i > 0; i -- )
	{
		for( j = 0; j < i; j ++ )
		{
			if( ((pSSDTSaveTable)((ULONG)pSSDTST + j * sizeof(SSDTSaveTable)))->ulServiceNumber > 
				  ((pSSDTSaveTable)((ULONG)pSSDTST + (j+1) * sizeof(SSDTSaveTable)))->ulServiceNumber )	//����
			{
				SSDTSaveTable SSDTSTTemp;
				memcpy( \
					(void*)&SSDTSTTemp, \
					(void*)((ULONG)pSSDTST + j * sizeof(SSDTSaveTable)), \
					sizeof( SSDTSaveTable ) \
					);
				memcpy( \
					(void*)((ULONG)pSSDTST + j * sizeof(SSDTSaveTable)), \
					(void*)((ULONG)pSSDTST + (j+1) * sizeof(SSDTSaveTable)), \
					sizeof( SSDTSaveTable ) \
					);
				memcpy( \
					(void*)((ULONG)pSSDTST + (j+1) * sizeof(SSDTSaveTable)), \
					(void*)&SSDTSTTemp, \
					sizeof( SSDTSaveTable ) \
					);
			}
		}
	}

	return TRUE;
}

//ö��SSDT
BOOL EnumSSDT( IN HANDLE hDriver )
{
	HINSTANCE hNtDllInst = NULL;
	ULONG ulNtDllOffset;
	ULONG ulFuncNameCount = 0;
	PIMAGE_EXPORT_DIRECTORY pImgExpDir = NULL;
	PULONG pFuncNameArray = NULL;
	ULONG i;
	BOOL bOK = TRUE;
	
	do
	{
		RealCount = 0;			//������0
		if( pList )		//���д�û���ͷ�
		{
			DestroyModList( pList );	//�ͷ���
			pList = NULL;
		}
		pList = CreateModList( &NTBase );	//����ģ����Ϣ����,˳��õ�NT��ַ
		if( pList == NULL )		//����ʧ��
		{
			bOK = FALSE;
			break;
		}
		
		if( !( hNtDllInst = LoadLibrary( "ntdll" ) ) )
		{
			bOK = FALSE;
			break;
		}
		/////////////////////////////////////////////////////////
		//����SSDT���滺���
		//�õ�SSDT����
		SSDT ssdt;
		if( !GetSSDT( hDriver, &ssdt ) )
		{
			bOK = FALSE;
			break;
		}
		if( TotalSSDTCount == -1 )		//�õ�SSDT����ʧ��
		{
			bOK = FALSE;
			break;
		}
		if( pSSDTST )		//pSSDTST����ֵ,���ͷ���
		{
			free( pSSDTST );
			pSSDTST = NULL;
		}
		pSSDTST = (pSSDTSaveTable)malloc( TotalSSDTCount * sizeof( SSDTSaveTable ) );
		if( pSSDTST == NULL )
		{
			bOK = FALSE;
			break;
		}
		for( i = 0; i < TotalSSDTCount; i ++ )	//��ʼ����
		{
			((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulServiceNumber = -1;
			((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulCurrentFunctionAddress = 0L;
			((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulOriginalFunctionAddress = 0L;
			memset( ((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ServiceFunctionName, \
				0, \
				sizeof(((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ServiceFunctionName));
			memset( ((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ModuleName, \
				0, \
				sizeof(((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ModuleName));
		}
		/////////////////////////////////////////////////////////
		//ö��
		ulNtDllOffset = (ULONG)hNtDllInst;
		//PEͷ��
		ulNtDllOffset += ((PIMAGE_DOS_HEADER)hNtDllInst)->e_lfanew + sizeof( DWORD );
		//����Ŀ¼
		ulNtDllOffset += sizeof( IMAGE_FILE_HEADER ) + sizeof( IMAGE_OPTIONAL_HEADER )
			- IMAGE_NUMBEROF_DIRECTORY_ENTRIES * sizeof( IMAGE_DATA_DIRECTORY );
		//������
		ulNtDllOffset = (DWORD)hNtDllInst + ((PIMAGE_DATA_DIRECTORY)ulNtDllOffset)->VirtualAddress;
		//����Ŀ¼��
		pImgExpDir = (PIMAGE_EXPORT_DIRECTORY)ulNtDllOffset;
		//�õ���������
		ulFuncNameCount = pImgExpDir->NumberOfNames;
		//����������ָ��
		pFuncNameArray = (PULONG)( (ULONG)hNtDllInst + pImgExpDir->AddressOfNames );
		/////////////////////
		//ѭ���Һ�����
		for( i = 0; i < ulFuncNameCount; i ++ )
		{
			//������
			PCSTR pszName = (PCSTR)( pFuncNameArray[i] + (ULONG)hNtDllInst );
			if( pszName[0] == 'N' && pszName[1] == 't' )	//Nt ��ͷ�ĺ���
			{
				//���ұ�
				LPWORD pOrdNameArray = (LPWORD)( (ULONG)hNtDllInst + pImgExpDir->AddressOfNameOrdinals );
				//������ַ
				LPDWORD pFuncArray = (LPDWORD)( (ULONG)hNtDllInst + pImgExpDir->AddressOfFunctions );
				//��������
				LPCVOID pFuncCode = (LPCVOID)( (ULONG)hNtDllInst + pFuncArray[pOrdNameArray[i]] );
				//��ȡ�����
				SSDTEntry EntryCode;
				memcpy( &EntryCode, pFuncCode, sizeof( SSDTEntry ) );
				if( EntryCode.byMov == 0xB8 )	// MOV EAX, XXXX
				{
					ULONG ulAddr = 0;
					if( !GetHook( hDriver, EntryCode.ulIndex, &ulAddr ) )
					{
						bOK = FALSE;
						break;
					}
					////////////////////////
					//ͨ����ַ�õ�ģ����
					char ModNameBuf[MAX_PATH+1];
					memset( ModNameBuf, 0, sizeof( ModNameBuf ) );

					if( GetModuleNameByAddr( ulAddr, pList, ModNameBuf, sizeof( ModNameBuf )-1 ) )
					{
						memcpy( \
							((pSSDTSaveTable)((ULONG)pSSDTST + RealCount * sizeof(SSDTSaveTable)))->ModuleName, \
							ModNameBuf, \
							sizeof( ModNameBuf ) \
							);
					}
					////////////////////////////////////////////////////
					//����SSDT��Ϣ���������
					((pSSDTSaveTable)((ULONG)pSSDTST + RealCount * sizeof(SSDTSaveTable)))->ulServiceNumber = \
						EntryCode.ulIndex;	//�����
					((pSSDTSaveTable)((ULONG)pSSDTST + RealCount * sizeof(SSDTSaveTable)))->ulCurrentFunctionAddress = \
						ulAddr;		//��ǰ������ַ
					memcpy( \
						((pSSDTSaveTable)((ULONG)pSSDTST + RealCount * sizeof(SSDTSaveTable)))->ServiceFunctionName, \
						pszName, \
						sizeof( ((pSSDTSaveTable)((ULONG)pSSDTST + RealCount * sizeof(SSDTSaveTable)))->ServiceFunctionName )
						);
					
					RealCount ++;
				}
			}
		}
	} while( FALSE );

	::FreeLibrary( hNtDllInst );
	
	if( bOK )	//�ɹ�
	{
		//��ȡʣ�µķ����
		for( i = RealCount; i < TotalSSDTCount; i ++ )
		{
			if( !GetHook( hDriver, i, &((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulCurrentFunctionAddress ) )
			{
				bOK = FALSE;
				break;
			}
			////////////////////////
			//ͨ����ַ�õ�ģ����
			char ModNameBuf[MAX_PATH+1];
			memset( ModNameBuf, 0, sizeof( ModNameBuf ) );
			
			if( GetModuleNameByAddr( \
				((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulCurrentFunctionAddress, \
				pList, ModNameBuf, sizeof( ModNameBuf )-1 ) )
			{
				memcpy( \
					((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ModuleName, \
					ModNameBuf, \
					sizeof( ModNameBuf ) \
					);
			}
		}
		//������Ž�������
		SSDTSTOrderByServiceNum( pSSDTST );

		//��ȡԭʼ������ַ
		GetOldSSDTAddress();
	}

	if( pList )
	{
		DestroyModList( pList );	//�ͷ�ģ������
		pList = NULL;
	}

	return bOK;
}
//�ָ�SSDT
BOOL ReSSDT( IN HANDLE hDriver )
{
	ULONG i;
	if( RealCount == 0 )
	{
		return FALSE;
	}
	for( i = 0; i < RealCount; i ++ )
	{
		if( \
			((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulCurrentFunctionAddress \
			!= ((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulOriginalFunctionAddress \
			)	//��ǰ��ַ��ԭʼ��ַ��ͬ,�ָ�ԭʼ��ֵַ
		{
			if( !SetHook( hDriver, \
				((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulServiceNumber, \
				&(((pSSDTSaveTable)((ULONG)pSSDTST + i * sizeof(SSDTSaveTable)))->ulOriginalFunctionAddress) \
				) )
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}
//�ָ�SSDT��ȥ����ϵͳ��
BOOL ReSSDTAndThrowSpilth( IN HANDLE hDriver )
{
	if( !ReSSDT( hDriver ) )
	{
		return FALSE;
	}
	//��ȡSSDT
	SSDT ssdt;
	if( !GetSSDT( hDriver, &ssdt ) )
	{
		return FALSE;
	}
	if( RealCount == 0 )
	{
		return FALSE;
	}
	ssdt.ulNumberOfServices = RealCount;
	if( !SetSSDT( hDriver, &ssdt ) )
	{
		return FALSE;
	}
	return TRUE;
}

//��ȡԭʼ������ַ,�Ա�ָ�SSDT
//˵��: �˹��ܵĺ���(�����)��Դ������
//PS: ��ʵ���벻���ܵõ�ȫ��ԭʼ������ַ�ķ�����
//�ҵķ���ֻ���Եõ����ֺ�����ԭʼ��ַ(�����õķ�����ͬ)

#define ibaseDD *(PDWORD)&ibase
#define RVATOVA(base,offset) ((PVOID)((DWORD)(base)+(DWORD)(offset)))

typedef struct { 
	WORD    offset:12;
	WORD    type:4;
} IMAGE_FIXUP_ENTRY, *PIMAGE_FIXUP_ENTRY;


DWORD GetHeaders(PCHAR ibase,
				 PIMAGE_FILE_HEADER *pfh,
				 PIMAGE_OPTIONAL_HEADER *poh,
				 PIMAGE_SECTION_HEADER *psh)

{ 
	PIMAGE_DOS_HEADER mzhead=(PIMAGE_DOS_HEADER)ibase;

	if    ((mzhead->e_magic!=IMAGE_DOS_SIGNATURE) ||
		(ibaseDD[mzhead->e_lfanew]!=IMAGE_NT_SIGNATURE))
		return FALSE;

	*pfh=(PIMAGE_FILE_HEADER)&ibase[mzhead->e_lfanew];
	if (((PIMAGE_NT_HEADERS)*pfh)->Signature!=IMAGE_NT_SIGNATURE) 
		return FALSE;
	*pfh=(PIMAGE_FILE_HEADER)((PBYTE)*pfh+sizeof(IMAGE_NT_SIGNATURE));

	*poh=(PIMAGE_OPTIONAL_HEADER)((PBYTE)*pfh+sizeof(IMAGE_FILE_HEADER));
	if ((*poh)->Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return FALSE;

	*psh=(PIMAGE_SECTION_HEADER)((PBYTE)*poh+sizeof(IMAGE_OPTIONAL_HEADER));
	return TRUE;
}

DWORD FindKiServiceTable(
	HMODULE hModule,
	DWORD dwKSDT)
{ 
    PIMAGE_FILE_HEADER    pfh;
    PIMAGE_OPTIONAL_HEADER    poh;
    PIMAGE_SECTION_HEADER    psh;
    PIMAGE_BASE_RELOCATION    pbr;
    PIMAGE_FIXUP_ENTRY    pfe;
    
    DWORD    dwFixups=0,i,dwPointerRva,dwPointsToRva,dwKiServiceTable;
    BOOL    bFirstChunk;
	
    GetHeaders((PCHAR)hModule,&pfh,&poh,&psh);

    if ((poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) &&
      (!((pfh->Characteristics)&IMAGE_FILE_RELOCS_STRIPPED))) {
      
      pbr=(PIMAGE_BASE_RELOCATION)RVATOVA(poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,hModule);
	
      bFirstChunk=TRUE;

      while (bFirstChunk || pbr->VirtualAddress) { 
        bFirstChunk=FALSE;
	
        pfe=(PIMAGE_FIXUP_ENTRY)((DWORD)pbr+sizeof(IMAGE_BASE_RELOCATION));
	
        for (i=0;i<(pbr->SizeOfBlock-sizeof(IMAGE_BASE_RELOCATION))>>1;i++,pfe++) { 
          if (pfe->type==IMAGE_REL_BASED_HIGHLOW) { 
            dwFixups++;
            dwPointerRva=pbr->VirtualAddress+pfe->offset;
            dwPointsToRva=*(PDWORD)((DWORD)hModule+dwPointerRva)-(DWORD)poh->ImageBase;
            if (dwPointsToRva==dwKSDT) { 
              if (*(PWORD)((DWORD)hModule+dwPointerRva-2)==0x05c7) {
                dwKiServiceTable=*(PDWORD)((DWORD)hModule+dwPointerRva+4)-poh->ImageBase;
                return dwKiServiceTable;
              }
            }    
          } 
        }
        *(PDWORD)&pbr+=pbr->SizeOfBlock;
      }
    }    
    
    return 0;
}

void GetOldSSDTAddress()
{     
		HMODULE   hKernel;
		PCHAR    	pKernelName;
		PDWORD    pService;
		PIMAGE_FILE_HEADER    pfh;
		PIMAGE_OPTIONAL_HEADER    poh;
		PIMAGE_SECTION_HEADER    psh;
		DWORD    dwKernelBase,dwServices=0;
		DWORD    dwKSDT;
		DWORD    dwKiServiceTable;
		
		ULONG n;

    pNtQuerySystemInformation(SystemModuleInfo,&n,0,&n);
		PULONG pBuf = (PULONG)malloc( n*sizeof(ULONG) );
    pNtQuerySystemInformation(SystemModuleInfo, pBuf, n*sizeof(*pBuf), 0);
		PSYSTEM_MODULE_INFORMATION module = PSYSTEM_MODULE_INFORMATION(pBuf+1);

    dwKernelBase=(DWORD)module->Base;
    pKernelName = module->ModuleNameOffset + module->ImageName;

    hKernel=LoadLibraryEx(pKernelName, 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!hKernel) {
        return;
    }

    if (!(dwKSDT=(DWORD)GetProcAddress(hKernel,"KeServiceDescriptorTable"))) {
        return;
    }

    dwKSDT-=(DWORD)hKernel;

    if (!(dwKiServiceTable=FindKiServiceTable(hKernel,dwKSDT))) { 
        return;
    }

    GetHeaders((PCHAR)hKernel,&pfh,&poh,&psh);
    dwServices=0;

    for (pService=(PDWORD)((DWORD)hKernel+dwKiServiceTable);
				 *pService-poh->ImageBase<poh->SizeOfImage;
			   pService++,dwServices++)
		{
				((pSSDTSaveTable)((ULONG)pSSDTST + dwServices * sizeof(SSDTSaveTable)))->ulOriginalFunctionAddress = *pService-poh->ImageBase+dwKernelBase;
		}
		FreeLibrary(hKernel);

		free( p );
}