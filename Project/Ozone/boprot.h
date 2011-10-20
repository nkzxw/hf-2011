/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		boBOPROT.c
 *
 * Abstract:
 *
 *		This module implements buffer overflow protection related routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 08-Jun-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __BOPROT_H__
#define __BOPROT_H__


#include <NTDDK.h>


BOOLEAN InitBufferOverflowProtection();
VOID ShutdownBufferOverflowProtection();


#endif	/* __BOPROT_H__ */
