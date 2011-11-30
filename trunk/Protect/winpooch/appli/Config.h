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

#ifndef _CONFIG_H
#define _CONFIG_H

#include <windows.h>

typedef enum {
  CFGINT_PROCGUARD_DETECTED,
  CFGINT_REGDEFEND_DETECTED,
  CFGINT_SPLASH_SCREEN,
  CFGINT_MAX_LOG_SIZE,
  CFGINT_SOUND,
  CFGINT_CHECK_FOR_UPDATES,
  CFGINT_SCAN_CACHE_LENGTH,
  CFGINT_SKIP_FIRST_CLOSE_WARNING,
  CFGINT_TRAY_ICON_ANIMATION,
  _CFGINT_COUNT //< This has to stay last
} CONFIG_INTEGER_ID ;

typedef enum {
  CFGSTR_LANGUAGE,
  CFGSTR_ANTIVIRUS,
  CFGSTR_ALERT_SOUND,
  CFGSTR_ASK_SOUND,
  CFGSTR_VIRUS_SOUND,
  _CFGSTR_COUNT //< This has to stay last
} CONFIG_STRING_ID ;

typedef enum {
  CFGSAR_SCAN_PATTERNS,
  CFGSAR_SCAN_FOLDERS,
  _CFGSAR_COUNT //< This has to stay last
} CONFIG_STRING_ARRAY_ID ;
  

/**
 * Initialize module and load config.
 */
BOOL Config_Init () ;

/**
 * Save config and uninitialize module.
 */
BOOL Config_Uninit () ;

/**
 * Load configuration from registry.
 */
BOOL Config_Load () ;

/**
 * Save configuration to registry.
 */
BOOL Config_Save () ;

/**
 * Get the value of a configuration integer.
 */
int Config_GetInteger (CONFIG_INTEGER_ID) ;

/**
 * Set the value of a configuration integer.
 */
void Config_SetInteger (CONFIG_INTEGER_ID, int) ;

/**
 * Save integer in the registry
 */
void Config_SaveInteger (CONFIG_INTEGER_ID) ;

/**
 * Get the value of a configuration string.
 */
LPCTSTR Config_GetString (CONFIG_STRING_ID) ;

/**
 * Set the value of a configuration string.
 */
void Config_SetString (CONFIG_STRING_ID, LPCTSTR) ;

/**
 * Save the string in the registry
 */
void Config_SaveString (CONFIG_STRING_ID) ;

/**
 * Get string array buffer and length
 */
LPCTSTR* Config_GetStringArray (CONFIG_STRING_ARRAY_ID, UINT * pnLength) ;

/**
 * Set string array
 */
void Config_SetStringArray (CONFIG_STRING_ARRAY_ID, LPCTSTR* pszArray, UINT nLength) ;

/**
 * Save the string array in the registry
 */
void Config_SaveStringArray (CONFIG_STRING_ARRAY_ID) ;

#endif
