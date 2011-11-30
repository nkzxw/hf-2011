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

#ifndef _LANGUAGE_H
#define _LANGUAGE_H

#include <windows.h>

/**
 * Message sent when language is changed.
 */
#define WM_LANGUAGECHANGED  (WM_USER+3)

/**
 * Maximum length of a language name.
 */
#define MAX_LANGUAGE_NAME	32

/**
 * Language string identifiers.
 */
typedef enum {

  // specials
  _LANGUAGE_NAME_IN_ENGLISH=0x0000,
  _LANGUAGE_NAME,
  _LICENSE_FILE,

  // common strings
  _OK=0x0100,
  _CANCEL,
  _HELP,
  _ERROR_READING_FILE_S,
  _ERROR_WRITING_FILE_S,

  // startup messages
  _ALREADY_RUNNING=0x0200,  
  _WIN64_DETECTED,		// <- REMOVED
  _NO_FILTERS,
  _ERROR_IN_FILTERS,
  _NEW_VERSION_AVAILABLE,
  _DOWNLOAD_VERSION_S,
  _OLD_FILTERS_CLEARED,
  _DEFAULT_FILTER_UPDATED,
  _FIRST_CLOSE_WARNING,
  _CANT_RUN_WITH_WIN64,

  // menu items
  _OPEN=0x0300,
  _SHUTDOWN,

  // filters
  _FILTERS=0x0400,
  _PROGRAM,
  _REASON,
  _HOOK_THIS_PROGRAM,
  _DONT_HOOK_THIS_PROGRAM,
  _RULE,
  _SURE_REMOVE_FILTER_FOR_S,
  _CONFIRM_ERASE_FILTERS,
  _SURE_REMOVE_RULE,

  // params
  _PARAM=0x0500,
  _TYPE,
  _VALUE,
  _ANY_VALUE,
  _INTEGER,
  _STRING,
  _WILDCARDS,
  _PATH_SPEC,

  // params names
  _FILE_PATH=0x0600,
  _ADDRESS,
  _PORT,
  _KEY_PATH,
  _VALUE_NAME,
    
  // reactions
  _REACTION=0x0700,
  _ASK,
  _ACCEPT,
  _FEIGN,
  _REJECT,
  _ASK_DEFAULT_ACCEPT,
  _ASK_DEFAULT_FEIGN,
  _ASK_DEFAULT_REJECT,
  _KILL_PROCESS,
  _ASK_DEFAULT_KILL_PROCESS,

  // verbosity
  _VERBOSITY=0x0800,
  _SILENT,
  _LOG,
  _ALERT,
  
  // options
  _OPTIONS=0x0900,
  _VIRUS_SCAN,
  
  // processes window
  _PROCESSES=0x0A00,
  _STATE,
  _PROCESS,
  _PID,
  _PATH,
  _UNKNOWN_STATE,
  _HOOK_FAILED,			// <- REMOVED
  _HOOK_DISABLED,		// <- REMOVED
  _HOOKED_SINCE_BIRTH,
  _HOOKED_WHILE_RUNNING,
  _HOOKED_BY_PREV_INSTANCE,	// <- REMOVED
  _CONFIRM_KILL_PROTECTED_PROCESS,
  _FAILED_TO_KILL_PROCESS,
  _CONFIRM_KILL_PROCESS,

  // history / log
  _HISTORY=0x0B00,
  _ACCEPTED,
  _FEIGNED,
  _REJECTED,  
  _ACCEPTED_FROM_U_S,
  _FEIGNED_FROM_U_S,
  _REJECTED_FROM_U_S,
  _TIME,
  
  // configuration window
  _CONFIGURATION=0x0C00,
  _APPLY_CONFIGURATION,		// <- REMOVED
  _LANGUAGE,
  _ANTIVIRUS,
  _USE_DEBUG_PRIVILEGE,		// <- REMOVED
  _ENABLE_SPLASH_SCREEN,
  _CHECK_FOR_UPDATES,
  _ENABLE_SOUND,		// <- REMOVED
  _MAX_LOG_FILE_SIZE,
  _KILOBYTES,
  _IMPORT,
  _EXPORT,
  _RESET,
  _SOUNDS,
  _ALERTING,
  _ASKING,
  _VIRUS,
  _TRAY_ICON_ANIMATION,
  _NO_SOUND,
  _DEFAULT_SOUNDS,
  _CUSTOM_SOUNDS,
  _WAVE_FILES,
  _SCAN_ONLY_MATCHING_FILES,
  _BACKGROUND_SCAN_THESE_FOLDERS,
  _ADD_FOLDER,
  _REMOVE_FOLDER,
  _ADD_PATTERN,
  _REMOVE_PATTERN,

  // server messages
  _KEY_POOL_HIWAT=0x0D00,	// <- REMOVED
  _KEY_POOL_LOWAT,		// <- REMOVED
  _RULE_DOESNT_MATCH,

  // program menu
  _PROGRAM_MENU=0x0E00,
  _ADD_PROGRAM,
  _EDIT_PROGRAM,
  _REMOVE_PROGRAM,

  // antivirus
  _ANTIVIRUS_NONE=0x0F00,
  _ANTIVIRUS_CLAMWIN,
  _ANTIVIRUS_NOT_INSTALLED,
  _ANTIVIRUS_KASPERSKY_WS,
  _ANTIVIRUS_BITDEFENDER,
  _ANTIVIRUS_LIBCLAMAV,

  // virus dialog and ask dialog
  _FILE_S_IS_INFECTED=0x1000,
  _ANTIVIRUS_REPORT,
  _WHAT_DO_YOU_WANT,
  _S_IN_D_SECONDS,
  _ARE_YOU_SURE,
  _THE_FOLLOWING_PROCESS,
  _IS_TRYING_TO,
  _NEW_FILTER,
  _OTHER_OPTIONS,
  _UNHOOK_PROCESS,

  // program path dialog
  _SET_PROGRAM_PATH=0x1100,
  _PROGRAM_PATH,
  _HOOKED_PROCESSES,

  // about
  _ABOUT=0x1200,
  _VERSION_S,
  _TRANSLATION_BY,
  _LICENSE,
  _ABOUT_FREEIMAGE,
  _ABOUT_SITE,
  _ABOUT_DRAWING,
  _VIEW_README,
  _VIEW_CHANGELOG,
  _VIEW_FAQ,
  _VIEW_LICENSE,
  _ABOUT_LIBCLAMAV,
  _MAKE_DONATION,

  // filter menu
  _FILTER_MENU=0x1300,
  _ADD_RULE,
  _EDIT_RULE,
  _REMOVE_RULE,
  _MOVE_UP_RULE,
  _MOVE_DOWN_RULE,

  // file filters
  _ALL_FILES=0x1400,
  _EXECUTABLE_FILES,
  _FILTER_FILES,

  // history menu
  _HISTORY_MENU=0x1500,
  _CLEAN_HISTORY,
  _CREATE_RULE_FROM_EVENT,
  _VIEW_LOG_FILE,

  // process menu
  _PROCESS_MENU=0x1600,
  _HOOK_SELECTED_PROCESS,
  _UNHOOK_SELECTED_PROCESS,
  _KILL_SELECTED_PROCESS,

  // scan cache window
  _TRUSTED_FILES=0x1700,
  _SCAN_TIME,
  _NO_SCANNER_CONFIGURED,
  _NOT_SCANNED,
  _BEING_SCANNED,
  _NO_VIRUS,
  _VIRUS_FOUND,
  _SCAN_FAILED,
  _UNKNOWN,
  _SCANNER_CACHE,

} STRID ;

/**
 * Macro to retreive string.
 */
#define STR(X) (Language_GetString(X))

/**
 * Macro to retreive string with a default value.
 */
#define STR_DEF(X,DEF) (Language_IsLoaded()?Language_GetString(X):DEF)

/**
 * Initialize language module.
 */
BOOL Language_Init () ;

/**
 * Uninitialize language module.
 */
VOID Language_Uninit () ;

/**
 * Set HWND that will receive WM_LANGCHANGED
 */
VOID Language_SetHwnd (HWND) ;

/**
 * Load a new language.
 */
BOOL Language_LoadByName (LPCTSTR szName) ;

/**
 * Load a new language.
 */
BOOL Language_LoadByIndex (int i) ;

/**
 * Load a new language.
 */
BOOL Language_LoadByFile (LPCTSTR szPath) ;

/**
 * Retreive a string.
 */
LPCTSTR Language_GetString (STRID id) ;

/**
 * Retreive a language name in list.
 */
LPCTSTR Language_GetLanguage (int i) ;

/**
 * Retreive language count.
 */
int Language_GetLanguageCount () ;

/**
 * Is a language loaded ?
 */
BOOL Language_IsLoaded () ;

#endif
