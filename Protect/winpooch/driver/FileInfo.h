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

#ifndef _FILEINFO_H
#define _FILEINFO_H

#include <ntddk.h>

NTSTATUS FileInfo_NtPathToDosPath (HANDLE		hProcess,
				   HANDLE		hDirectory,
				   PUNICODE_STRING	pusDosPath,
				   PUNICODE_STRING	pusNtPath) ;

NTSTATUS FileInfo_GetPath (IN HANDLE hFile, 
			   OUT PUNICODE_STRING pusFilePath) ;

NTSTATUS FileInfo_GetLastWriteTime (IN LPCWSTR		wszFilePath,
				    OUT LARGE_INTEGER	* pliTime) ;

NTSTATUS FileInfo_GetLastWriteTimeFromHandle (IN HANDLE		hFile,
					      OUT LARGE_INTEGER	* pliTime) ;

#endif
