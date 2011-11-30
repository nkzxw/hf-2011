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
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "DrvFilter.h"

// project's headers
#include "BuildCount.h"
#include "Link.h"
#include "ProcList.h"
#include "Trace.h"


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID DrvStatus_Trace ()
{
  DbgPrint ("---------------- Winpooch driver status ----------------\n"
	    "Build = %d\n"
	    "DrvFilter_IsLocked()	=> %d\n"
	    "ProcList_IsLocked()	=> %d\n"
	    "Link_IsAppWaiting()	=> %d\n"
	    "Link_IsRequesting()	=> %d\n"
	    "Process address		=> 0x%08X\n"
	    "Process ID			=> %u\n"
	    "--------------------------------------------------------\n",
	    DRIVER_BUILD,
	    DrvFilter_IsLocked(),
	    ProcList_IsLocked(),
	    Link_IsAppWaiting(),
	    Link_IsRequesting(),
	    ProcInfo_GetCurrentProcessAddress(),
	    ProcInfo_GetCurrentProcessId()) ;
}
