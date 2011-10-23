//********************************************************************
//	created:	20:8:2008   23:05
//	file:		selkrnl.h
//	author:		tiamo
//	purpose:	select kernel
//********************************************************************

#pragma once

//
// select kernel
//
PCHAR BlSelectKernel(__in ULONG DriveId,__in PCHAR IniFileContents,__inout PCHAR* LoadOptions,__in BOOLEAN UseTimeout);