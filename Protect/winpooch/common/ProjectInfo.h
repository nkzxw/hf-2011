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


#ifndef _PROJECTINFO_H
#define _PROJECTINFO_H


#define VERSION_HIGH	0
#define VERSION_MED	6
#define VERSION_LOW	6


#define APPLICATION_NAME	"Winpooch"

#define WEB_SITE_ADDRESS	"http://winpooch.sourceforge.net/"

#define BUG_REPORT_PAGE		"http://sourceforge.net/tracker/?group_id=122629&atid=694093"

#define DONATION_PAGE		"http://sourceforge.net/donate/index.php?group_id=122629"


#define STRINGIFY(s) #s
#define STRINGIFY2(s) STRINGIFY(s)

#define MAKE_VERSION_STRING(H,M,L,B) STRINGIFY2(H) "." STRINGIFY2(M) "." STRINGIFY2(L)

#define MAKE_VERSION_COMMA(H,M,L,B) H,M,L,B

#define APPLICATION_VERSION_STRING \
  MAKE_VERSION_STRING(VERSION_HIGH,VERSION_MED,VERSION_LOW,APPLICATION_BUILD)

#define APPLICATION_VERSION_COMMA \
  MAKE_VERSION_COMMA(VERSION_HIGH,VERSION_MED,VERSION_LOW,APPLICATION_BUILD)



#endif
