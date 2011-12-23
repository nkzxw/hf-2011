///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2010 - <company name here>
///
/// Defines for the version information in the resource file
///
/// (File was in the PUBLIC DOMAIN  - Created by: ddkwizard\.assarbad\.net)
///////////////////////////////////////////////////////////////////////////////

// $Id$

#ifndef __DRVVERSION_H_VERSION__
#define __DRVVERSION_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "buildnumber.h"

// ---------------------------------------------------------------------------
// Several defines have to be given before including this file. These are:
// ---------------------------------------------------------------------------
#define TEXT_AUTHOR            <author name(s)> // author (optional value)
#define PRD_MAJVER             1 // major product version
#define PRD_MINVER             0 // minor product version
#define PRD_BUILD              0 // build number for product
#define FILE_MAJVER            1 // major file version
#define FILE_MINVER            0 // minor file version
#define FILE_BUILD             _FILE_VERSION_BUILD // file build number
#define DRV_YEAR               2010 // current year or timespan (e.g. 2003-2009)
#define TEXT_WEBSITE           <website> // website
#define TEXT_PRODUCTNAME       Supercool driver-based tool // product's name
#define TEXT_FILEDESC          The driver for the supercool driver-based tool // component description
#define TEXT_COMPANY           <company name here> // company
#define TEXT_MODULE            drv_win7 // module name
#define TEXT_COPYRIGHT         Copyright \xA9 DRV_YEAR TEXT_COMPANY // copyright information
// #define TEXT_SPECIALBUILD      // optional comment for special builds
#define TEXT_INTERNALNAME      drv_win7.sys // copyright information
// #define TEXT_COMMENTS          // optional comments
// ---------------------------------------------------------------------------
// ... well, that's it. Pretty self-explanatory ;)
// ---------------------------------------------------------------------------

#endif // __DRVVERSION_H_VERSION__
