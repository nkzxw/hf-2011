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

#ifndef _SCANRESULT_H
#define _SCANRESULT_H

typedef enum {
  SCAN_NOT_SCANNED,	// the file hasn't been scanned yet
  SCAN_BEING_SCANNED,	// the file is currently being scanned
  SCAN_NO_VIRUS,	// the file was scanned and no virus was found
  SCAN_VIRUS,		// a virus was found and not allowed to run
  SCAN_VIRUS_ACCEPTED,	// a virus was found but allowed to run
  SCAN_FAILED,		// scanning failed and openning the file is not allowed
  SCAN_FAILED_ACCEPTED,	// scanning failed but openning the file is allowed
} SCANRESULT ;

#endif
