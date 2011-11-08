/******************************************************************************
*
* Regmon - Registry Monitor for Windows 95/98/Me/NT/2K/XP/IA64 
*		
* Copyright (c) 1996-2002 Mark Russinovich and Bryce Cogswell
* See readme.txt for terms and conditions.
*
* Displays Registry activity in real-time.
*
******************************************************************************/

extern HANDLE		SysHandle;


BOOL LoadDeviceDriver( const char * Name, const char * Path, 
					  HANDLE * lphDevice, PDWORD Error );
BOOL UnloadDeviceDriver( const char * Name );