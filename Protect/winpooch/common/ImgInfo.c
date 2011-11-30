/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2006  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/


/******************************************************************/
/* Build configuration                                            */
/******************************************************************/

#define	TRACE_LEVEL	2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

#include "ImgInfo.h"

#include <windows.h>
#include <winnt.h>

#include "Strlcpy.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define CV_PDB70_SIGNATURE 0x53445352

/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  DWORD  CvSignature;
  GUID Signature;
  DWORD Age;
  BYTE PdbFileName[0];
} CV_INFO_PDB70 ;


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

ULONG _ImgGuid_RvaToOffset (IMAGE_NT_HEADERS* pNtHeaders, ULONG nRva) ;

int _snprintf(char *buffer,size_t count,const char *format, ...);


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL ImgInfo_GetInfo (IMGINFO* pInfo, VOID* pBuffer, UINT nSize, BOOL bFromFile) 
{
  IMAGE_DOS_HEADER	*pDosHeader ;
  IMAGE_NT_HEADERS	*pNtHeader ;
  IMAGE_DATA_DIRECTORY	*pDataDirectory ;
  IMAGE_DATA_DIRECTORY	*pDataDirectoryDbg ;
  IMAGE_DEBUG_DIRECTORY	*pDbgDirectory ;
  UINT			nDbgDirectorySize ;
  UINT			nRemainSize ;
  ULONG			nDbgDirectoryOffset ;

  TRACE_INFO (TEXT("Buffer address : 0x%08X\n"), pBuffer) ;
  TRACE_INFO (TEXT("Buffer size : 0x%08X\n"), nSize) ;

  // is buffer ig enough to store the DOS header ?
  if( nSize < sizeof(IMAGE_DOS_HEADER) ) {
    TRACE_ERROR (TEXT("Buffer too small (%d bytes) to contain DOS header\n"), nSize) ;
    return FALSE ;
  }

  // get DOS header address
  pDosHeader = pBuffer ;

  // verify DOS header signature
  if( pDosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
    TRACE_ERROR (TEXT("Invalid DOS header signature\n")) ;
    return FALSE ;
  }
  
  pNtHeader = (VOID*)( (BYTE*)pBuffer + pDosHeader->e_lfanew ) ;
  nRemainSize = nSize - ((BYTE*)pNtHeader - (BYTE*)pBuffer) ;

  // verify remaining size
  if( nRemainSize < sizeof(IMAGE_NT_HEADERS) ) {
    TRACE_ERROR (TEXT("Buffer too small (%d bytes) to contain NT header\n"), nSize) ;
    return FALSE ;
  }

  TRACE_INFO (TEXT("IMAGE_NT_HEADERS (address = base+0x%08X = 0x%08X)\n"),
	      (BYTE*)pNtHeader-(BYTE*)pBuffer, pNtHeader) ;

  TRACE_INFO (TEXT(".Signature = 0x%08X\n"), pNtHeader->Signature) ;

  // verify NT header signature
  if( pNtHeader->Signature != IMAGE_NT_SIGNATURE ) {
    TRACE_ERROR (TEXT("Invalid NT header signature\n")) ;
    return FALSE ;
  }

  TRACE_INFO (TEXT("IMAGE_FILE_HEADER\n")) ;
  TRACE_INFO (TEXT(".Machine              = 0x%04X\n"), pNtHeader->FileHeader.Machine) ;
  TRACE_INFO (TEXT(".NumberOfSections     = 0x%04X\n"), pNtHeader->FileHeader.NumberOfSections) ;
  TRACE_INFO (TEXT(".TimeDateStamp        = 0x%08X\n"), pNtHeader->FileHeader.TimeDateStamp) ;
  TRACE_INFO (TEXT(".PointerToSymbolTable = 0x%08X\n"), pNtHeader->FileHeader.PointerToSymbolTable) ;
  TRACE_INFO (TEXT(".NumberOfSymbols      = 0x%08X\n"), pNtHeader->FileHeader.NumberOfSymbols) ;
  TRACE_INFO (TEXT(".SizeOfOptionalHeader = 0x%04X\n"), pNtHeader->FileHeader.SizeOfOptionalHeader) ;
  TRACE_INFO (TEXT(".Characteristics      = 0x%04X\n"), pNtHeader->FileHeader.Characteristics) ;

  pInfo->dwTimeStamp = pNtHeader->FileHeader.TimeDateStamp ;

  TRACE_INFO (TEXT("IMAGE_OPTIONAL_HEADER\n")) ;
  TRACE_INFO (TEXT(".Magic                = 0x%04X\n"), pNtHeader->OptionalHeader.Magic) ;
  TRACE_INFO (TEXT(".MajorLinkerVersion   = 0x%02X\n"), pNtHeader->OptionalHeader.MajorLinkerVersion) ;
  TRACE_INFO (TEXT(".MinorLinkerVersion   = 0x%02X\n"), pNtHeader->OptionalHeader.MinorLinkerVersion) ;
  TRACE_INFO (TEXT(".SizeOfImage          = 0x%08X\n"), pNtHeader->OptionalHeader.SizeOfImage) ;
  TRACE_INFO (TEXT(".CheckSum             = 0x%08X\n"), pNtHeader->OptionalHeader.CheckSum) ;

  pInfo->dwCheckSum = pNtHeader->OptionalHeader.CheckSum ;
  
  pDataDirectory = pNtHeader->OptionalHeader.DataDirectory ;

  TRACE_INFO (TEXT("directory address = 0x%08X\n"), (BYTE*)pDataDirectory-(BYTE*)pBuffer) ;

  pDataDirectoryDbg = &pDataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] ;

  TRACE_INFO (TEXT("IMAGE_DATA_DIRECTORY\n")) ;
  TRACE_INFO (TEXT(".VirtualAddress = 0x%08X\n"), pDataDirectoryDbg->VirtualAddress) ;
  TRACE_INFO (TEXT(".Size = 0x%08X\n"), pDataDirectoryDbg->Size) ;
  TRACE_INFO (TEXT("Number of IMAGE_DEBUG_DIRECTORY = %d (%d)\n"), 
	      pDataDirectoryDbg->Size/sizeof(IMAGE_DEBUG_DIRECTORY),
	      pDataDirectoryDbg->Size%sizeof(IMAGE_DEBUG_DIRECTORY)) ;

  if( bFromFile )
    nDbgDirectoryOffset = _ImgGuid_RvaToOffset (pNtHeader, pDataDirectoryDbg->VirtualAddress) ;
  else
    nDbgDirectoryOffset = pDataDirectoryDbg->VirtualAddress ;

  pDbgDirectory = (VOID*)( (BYTE*)pBuffer + nDbgDirectoryOffset) ;
  nDbgDirectorySize = pDataDirectoryDbg->Size ;

  TRACE_INFO (TEXT("IMAGE_DEBUG_DIRECTORY (address = base+0x%08X = 0x%08X)\n"), nDbgDirectoryOffset, pDbgDirectory) ;
  TRACE_INFO (TEXT(".Characteristics  = 0x%08X\n"), pDbgDirectory->Characteristics) ;
  TRACE_INFO (TEXT(".TimeDateStamp    = 0x%08X\n"), pDbgDirectory->TimeDateStamp) ;
  TRACE_INFO (TEXT(".MajorVersion     = 0x%04X\n"), pDbgDirectory->MajorVersion) ;
  TRACE_INFO (TEXT(".MinorVersion     = 0x%04X\n"), pDbgDirectory->MinorVersion) ;
  TRACE_INFO (TEXT(".Type             = 0x%08X\n"), pDbgDirectory->Type) ;
  TRACE_INFO (TEXT(".SizeOfData       = 0x%08X\n"), pDbgDirectory->SizeOfData) ;
  TRACE_INFO (TEXT(".AddressOfRawData = 0x%08X\n"), pDbgDirectory->AddressOfRawData) ;
  TRACE_INFO (TEXT(".PointerToRawData = 0x%08X\n"), pDbgDirectory->PointerToRawData) ;

  nRemainSize = nSize - ((BYTE*)pDbgDirectory - (BYTE*)pBuffer) ;

  if( nRemainSize < nDbgDirectorySize ) {
    TRACE_ERROR (TEXT("Buffer too small (%d bytes) to contain debug directory\n"), nSize) ;
    return FALSE ;
  }

  TRACE_INFO(TEXT("Debug directory type: %d\n"), pDbgDirectory->Type) ;

  switch( pDbgDirectory->Type )
    {
    case IMAGE_DEBUG_TYPE_MISC:
      {
	IMAGE_DEBUG_MISC * pImgDbgMisc ;
	
	if( bFromFile )
	  pImgDbgMisc = (VOID*)( (BYTE*)pBuffer + pDbgDirectory->PointerToRawData ) ;
	else
	  pImgDbgMisc = (VOID*)( (BYTE*)pBuffer + pDbgDirectory->AddressOfRawData ) ;

	TRACE_INFO (TEXT("IMAGE_DEBUG_MISC (at offset 0x%08X)\n"), pDbgDirectory->PointerToRawData) ;
	TRACE_INFO (TEXT(".DataType = 0x%08X\n"), pImgDbgMisc->DataType) ;
	TRACE_INFO (TEXT(".Length   = 0x%08X\n"), pImgDbgMisc->Length) ;
	TRACE_INFO (TEXT(".Unicode  = %s\n"), pImgDbgMisc->Unicode ? TEXT("TRUE") : TEXT("FALSE")) ;
	TRACE_INFO (TEXT(".Data     = %hs\n"), pImgDbgMisc->Data) ;

	strcpy (pInfo->szSymFilename, pImgDbgMisc->Data) ;
	
	_snprintf (pInfo->szSymSignature, 64,
		   "%08X%06X", 
		   pNtHeader->FileHeader.TimeDateStamp,
		   pNtHeader->OptionalHeader.SizeOfImage) ;
      }
      break ;

    case IMAGE_DEBUG_TYPE_CODEVIEW:
      {
	CV_INFO_PDB70		*pCvData ;

	if( bFromFile )
	  pCvData = (VOID*)( (BYTE*)pBuffer + pDbgDirectory->PointerToRawData ) ;
	else
	  pCvData = (VOID*)( (BYTE*)pBuffer + pDbgDirectory->AddressOfRawData ) ;
	
	// verify that codeview data is of type RSDS (PDB 7.0)
	if( pCvData->CvSignature != CV_PDB70_SIGNATURE ) {
	  TRACE_ERROR (TEXT("CodeView data is not of type PDB70 (0x%08X)\n"), pCvData->CvSignature) ;
	  return FALSE ;
	}
	
	strcpy (pInfo->szSymFilename, pCvData->PdbFileName) ;	
	
	_snprintf (pInfo->szSymSignature, 64,
		   "%08X%04X%04X%02hX%02hX%02hX%02hX%02hX%02hX%02hX%02hX%d", 
		   pCvData->Signature.Data1, pCvData->Signature.Data2,
		   pCvData->Signature.Data3, pCvData->Signature.Data4[0],
		   pCvData->Signature.Data4[1], pCvData->Signature.Data4[2],
		   pCvData->Signature.Data4[3], pCvData->Signature.Data4[4],
		   pCvData->Signature.Data4[5], pCvData->Signature.Data4[6],
		   pCvData->Signature.Data4[7], pCvData->Age) ;

	TRACE_INFO (TEXT("Symbol file name = %s\n"), pInfo->szSymFilename) ;
	TRACE_INFO (TEXT("PDB signature = %s\n"), pInfo->szSymSignature) ;
      }
      break ;

    default:
      
      TRACE_WARNING (TEXT("Debug information type is not supported (%d)\n"), pDbgDirectory->Type) ;
      
      pInfo->szSymFilename[0] = 0 ;
      pInfo->szSymSignature[0] = 0 ;
    }
 
  return TRUE ;
}


ULONG _ImgGuid_RvaToOffset (IMAGE_NT_HEADERS* pNtHeaders, ULONG nRva) 
{ 
  IMAGE_SECTION_HEADER * pSectionHeader = IMAGE_FIRST_SECTION (pNtHeaders) ; 
  int i ;

  TRACE_INFO (TEXT("Converting RVA 0x%08X to offset\n"), nRva) ;
  
  for( i=0 ; i<pNtHeaders->FileHeader.NumberOfSections ; i++ ) 
    {
      DWORD nAddr = pSectionHeader[i].VirtualAddress ;
      DWORD nSize = pSectionHeader[i].Misc.VirtualSize ;

      TRACE_INFO (TEXT("IMAGE_SECTION_HEADER (address = 0x%08X)\n"), &pSectionHeader[i]) ;
      TRACE_INFO (TEXT(".Name                 = %-8s\n"), pSectionHeader[i].Name) ; 
      //TRACE_INFO (TEXT(".Misc.PhysicalAddress = 0x%08X\n"), pSectionHeader[i].Misc.PhysicalAddress) ; 
      TRACE_INFO (TEXT(".Misc.VirtualSize     = 0x%08X\n"), pSectionHeader[i].Misc.VirtualSize) ; 
      TRACE_INFO (TEXT(".VirtualAddress       = 0x%08X\n"), pSectionHeader[i].VirtualAddress) ; 
      TRACE_INFO (TEXT(".SizeOfRawData        = 0x%08X\n"), pSectionHeader[i].SizeOfRawData) ; 
      TRACE_INFO (TEXT(".PointerToRawData     = 0x%08X\n"), pSectionHeader[i].PointerToRawData) ;
      TRACE_INFO (TEXT(".PointerToRelocations = 0x%08X\n"), pSectionHeader[i].PointerToRelocations) ;
      TRACE_INFO (TEXT(".PointerToLinenumbers = 0x%08X\n"), pSectionHeader[i].PointerToLinenumbers) ;
      TRACE_INFO (TEXT(".Characteristics      = 0x%08X\n"), pSectionHeader[i].Characteristics) ;
     
      if( ( nRva >= nAddr ) && ( nRva < nAddr + nSize ) ) 
	{
	  int nOffset = (int)( nRva - nAddr + pSectionHeader[i].PointerToRawData ) ; 
	  
	  TRACE_INFO (TEXT(" -> RVA found on section %d (0x%08X-0x%08X) -> offset = 0x%08X\n"), 
		      i, nAddr, nAddr+nSize, nOffset) ;

	  return nOffset; 	  
	}
    }

  TRACE_INFO (TEXT("RVA not found !\n")) ;
  
  return 0 ;
}
