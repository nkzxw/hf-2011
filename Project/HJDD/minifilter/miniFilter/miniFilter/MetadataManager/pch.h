/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    This module includes all  the headers which need to be
    precompiled & are included by all the source files in this
    project


Environment:

    Kernel mode


--*/

#ifndef __FMM_PCH_H__
#define __FMM_PCH_H__

//
//  Enabled warnings
//

#pragma warning(error:4100)     //  Enable-Unreferenced formal parameter
#pragma warning(error:4101)     //  Enable-Unreferenced local variable
#pragma warning(error:4061)     //  Eenable-missing enumeration in switch statement
#pragma warning(error:4505)     //  Enable-identify dead functions

//
//  Includes
//

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "MetadataManagerStruc.h"
#include "MetadataManagerProc.h"

#if (WINVER == 0x0500)
#define VOLSNAPCONTROLTYPE  ((ULONG) 'S')
#define IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES         CTL_CODE(VOLSNAPCONTROLTYPE, 0, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#endif __FMM_PCH_H__

