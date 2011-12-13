#include "stdafx.h"
#include "inject.h"
#include "pack_code.h"
#include "pe.h"

#include <stdio.h>
#include <stdlib.h>

#pragma region REQUIRED_STRINGS

typedef struct _XREF_NAMES_
{
   char *dll;
   char *calls[64];
} XREFNAMES;

XREFNAMES vg_imports[] = {
   { "USER32.DLL", 
      {
         "RegisterClassExA",  // 00  / ref offset = 0 
         "CreateWindowExA",   // 17  / ref offset = 4
         "SetWindowTextA",    // 33  / ref offset = 8
         "ShowWindow",        // 48  / ref offset = 12
         "UpdateWindow",      // 59  / ref offset = 16
         "SetFocus",          // 72  / ref offset = 20
         "GetMessageA",       // 81  / ref offset = 24
         "TranslateMessage",  // 93  / ref offset = 28
         "DispatchMessageA",  // 110 / ref offset = 32 
         "GetWindowTextA",    // 127 / ref offset = 36
         "MessageBoxA",       // 142 / ref offset = 40
         "PostQuitMessage",   // 154 / ref offset = 44
         "DefWindowProcA",    // 170 / ref offset = 48
         "GetSystemMetrics",  // 185 / ref offset = 52
         "GetDlgItem",        // 202 / ref offset = 56 
         "DestroyWindow",     // 212 / ref offset = 60
         NULL 
      }
   }, // USER32.DLL

   { "KERNEL32.DLL", 
      {
         "ExitProcess",       // 230 ( 225 + _GAP_FUNCTION_NAMES )  / ref offset = 64
         NULL 
      }
   }, // KERNEL32.DLL

   { NULL, { NULL } }
};

char *vg_string_list[] = {
	"@PWDWIN@",                // offset = 0 bytes 
	" Type the password ...",  // offset = 9 bytes 
	"BUTTON",                  // offset = 32 bytes 
	"EDIT",                    // offset = 39 bytes 
	"OK",                      // offset = 44 bytes 
	"Cancel",                  // offset = 47 bytes 
	"Sorry! Wrong password.",  // offset = 54 bytes 
	"Password",                // offset = 77 bytes   
	NULL 
};

#pragma endregion

void fill_data_areas( CPEToolHelp *_pe,  char *_pwd )
{
	DWORD dwEPSize = 0;
	DWORD iOffSet = 0;
	DWORD iOffSet2 = 0;

	DWORD dwOriginalEntryPoint = _pe->Get_ExecEntryPoint();
	DWORD dwLoadLibrary        = _pe->Get_LoadLibraryAddr();
	DWORD dwGetProcAddress	   = _pe->Get_GetProcAddressAddr();
	BYTE  pass[12];            // Password not more than 10 chars

   //Init data areas
   (void) memset( vg_data_ep_data, 0x00, sizeof(vg_data_ep_data) );
   (void) memset( vg_data_pw_data, 0x00, sizeof(vg_data_pw_data) );

   //Saves Origial Entry-Point 
   (void) memcpy( vg_data_ep_data, &dwOriginalEntryPoint, sizeof(DWORD) );  // offset = 0

   //Saves LoadLibrary and GetProcAddress
   (void) memcpy( &vg_data_ep_data[4], &dwLoadLibrary, sizeof(DWORD) );     // offset = 4
   (void) memcpy( &vg_data_ep_data[8], &dwGetProcAddress, sizeof(DWORD) );  // offset = 8

   //Let's save the size (in bytes) of NewEntryPoint
   dwEPSize = (DWORD)NewEntryPoint_End - (DWORD)NewEntryPoint; 
   (void) memcpy( &vg_data_ep_data[12], &dwEPSize, sizeof(DWORD) );

   //Saves typed password
   (void) memset( pass, 0x00, sizeof(pass) );
   (void) strcpy( (char *)pass, _pwd );  
   (void) memcpy( &vg_data_ep_data[16], pass, 12 );  // offset = 16

   //Mark end of data area
   (void) memcpy( &vg_data_ep_data[_MAX_SECTION_DATA_SIZE_- 3], "<E>", 3 );

   //Mark end of data area
   (void) memcpy( &vg_data_pw_data[_MAX_SEC_SECTION_DATA_SIZE_ - 3], "<E>", 3 );

   //Add strings used by injected code
   iOffSet = _OFFSET_STRINGS;
   for ( int iC = 0; vg_string_list[iC]; iC++ )
   {
      (void) memcpy( &vg_data_ep_data[iOffSet], vg_string_list[iC], strlen(vg_string_list[iC]) );
      iOffSet += ((UINT)strlen(vg_string_list[iC])+1);
   }

   //Add all DLLs and Functions to be imported
   iOffSet  = _OFFSET_DLL_NAMES;
   iOffSet2 = _OFFSET_FUNCTION_NAMES;
   for ( int iC = 0; vg_imports[iC].dll; iC++ )
   {
      (void) memcpy( &vg_data_ep_data[iOffSet], vg_imports[iC].dll, strlen(vg_imports[iC].dll) );
      iOffSet += ((UINT)strlen(vg_imports[iC].dll)+1);

      for ( int iD = 0; vg_imports[iC].calls[iD]; iD++ )
      {
         (void) memcpy( &vg_data_ep_data[iOffSet2], vg_imports[iC].calls[iD], strlen(vg_imports[iC].calls[iD]) );
         iOffSet2 += ((UINT)strlen(vg_imports[iC].calls[iD])+1);
      }

	  //A small gap saparating functions groups
      iOffSet2 += _GAP_FUNCTION_NAMES; 
   }
}

void inject(char *pchFileName, char *pchPwd)
{
	UINT iOffSet = 0;
	CPEToolHelp *pe = new CPEToolHelp();
	char *lpPwd = pchPwd;

	//Loads PE file
	if ( !pe->LoadFile(pchFileName) )
	{
		//TODO
		return;
	}

	//Verifies if LoadLibrary and GetProcAddress are available
	if ( pe->Get_LoadLibraryAddr() == 0 || pe->Get_GetProcAddressAddr() == 0 )
	{
		//TODO
		return;
	}

	//Fills data areas
	fill_data_areas(pe, lpPwd);
	(void) memset( vg_new_section, 0x00, sizeof(vg_new_section) );
	
	//Add data for NewEntryPoint
	iOffSet = 0;
	(void) memcpy( &vg_new_section[iOffSet], vg_data_ep_data, sizeof(vg_data_ep_data) );

	//Add new NewEntryPoint Code
	iOffSet += sizeof(vg_data_ep_data);
	(void) memcpy( &vg_new_section[iOffSet], (BYTE *)NewEntryPoint, (DWORD)NewEntryPoint_End - (DWORD)NewEntryPoint );

	//Add data for PwdWindow
	iOffSet += ((DWORD)NewEntryPoint_End - (DWORD)NewEntryPoint);
	(void) memcpy( &vg_new_section[iOffSet], vg_data_pw_data, sizeof(vg_data_pw_data) );

	//Add new PwdWindow Code
	iOffSet += sizeof(vg_data_pw_data);
	(void) memcpy( &vg_new_section[iOffSet], (BYTE *)PwdWindow, (DWORD)PwdWindow_End - (DWORD)PwdWindow );

	pe->AddCodeSection( ".x123y", vg_new_section, _MAX_SECTION_SIZE, _MAX_SECTION_DATA_SIZE_ );   

	//Save new exec
	pe->SaveFile(pchFileName);
}