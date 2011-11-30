/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2005  Benoit Blanchon                      */
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

#ifndef _FILTREASON_H
#define _FILTREASON_H

// Reason identifiers
typedef enum {
  FILTREASON_UNDEFINED,
  FILTREASON_FILE_READ,
  FILTREASON_FILE_WRITE,
  FILTREASON_NET_CONNECT,
  FILTREASON_NET_LISTEN,
  FILTREASON_NET_SEND,
  FILTREASON_REG_SETVALUE,
  FILTREASON_REG_QUERYVALUE,
  FILTREASON_SYS_EXECUTE,
  FILTREASON_SYS_KILLPROCESS,
  _FILTREASON_COUNT //< has to be the last one
} FILTREASON ;

#endif
