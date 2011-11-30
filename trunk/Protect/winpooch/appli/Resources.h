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

#define IDC_STATIC		(-1)

// resource types
#define RT_IMAGE		MAKEINTRESOURCE(100)

// dialog boxes
#define DLG_RULE		10
#define DLG_PARAM		11
#define DLG_PROGPATH		12
#define DLG_ASK			13
#define DLG_VIRUS		14
#define DLG_CONFIG		15

// icons
#define IDI_APP				100
#define IDI_TRAY_IDLE0			101
#define IDI_TRAY_IDLE1			102
#define IDI_TRAY_IDLE2			103
#define IDI_TRAY_IDLE3			104
#define IDI_TRAY_IDLE4			105
#define IDI_TRAY_ALERT0			106
#define IDI_TRAY_ALERT1			107
#define IDI_TRAY_SCAN0			108
#define IDI_TRAY_SCAN1			109
#define IDI_TRAY_SCAN2			110
#define IDI_DEFAULT			111
#define IDI_UNKNOWN			112
#define IDI_TOOL_ADD			113
#define IDI_TOOL_REMOVE			114
#define IDI_TOOL_EDIT			115
#define IDI_TOOL_UP			116
#define IDI_TOOL_DOWN			117
#define IDI_COLUMN_UP			118
#define IDI_COLUMN_DOWN			119
#define IDI_STATE_UNKNOWN		120
#define IDI_STATE_FAILED		121
#define IDI_STATE_DISABLED		122
#define IDI_STATE_HOOKED		123
#define IDI_REASON_FILE_READ		124
#define IDI_REASON_FILE_WRITE		125
#define IDI_REASON_NET_CONNECT		126
#define IDI_REASON_NET_LISTEN		127
#define IDI_REASON_REG_SETVALUE		128
#define IDI_REASON_SYS_EXECUTE		129
#define IDI_CLOCK			130
#define IDI_REASON_REG_QUERYVALUE	131
#define IDI_REASON_SYS_KILLPROCESS	132

// bitmaps and images
#define IDB_ASK			201
#define IDB_SPLASH		202
#define IDB_VIRUS		203
#define IDB_FILTERS		204
#define IDB_HISTORY		205
#define IDB_PROCESSES		206
#define IDB_CONFIGURATION	208
#define IDB_ABOUT		209
#define IDB_SCANCACHE		210

// wave files
#define IDW_BARK		300

// text controls
#define IDT_REASON		1000
#define IDT_REACTION		1001
#define IDT_VERBOSITY		1002
#define IDT_TYPE		1003
#define IDT_VALUE		1004
#define IDT_PATH		1005
#define IDT_PROCESS		1006
#define IDT_OPTIONS		1007
#define IDT_FILE		1008
#define IDT_REPORT		1009
#define IDT_LANGUAGE		1011
#define IDT_ANTIVIRUS		1012
#define IDT_MAX_LOG_SIZE	1013
#define IDT_KILOBYTES		1014
#define IDT_CONFIGURATION	1015
#define IDT_FILTERS		1016
#define IDT_PROGRAM		1017
#define IDT_SOUNDS		1018
#define IDT_ALERT		1019
#define IDT_ASK			1020
#define IDT_VIRUS		1021
#define IDT_FILTERS_GROUP	1022
#define IDT_ANTIVIRUS_GROUP	1023
#define IDT_SCAN_PATTERNS	1024
#define IDT_SCAN_FOLDERS	1025

// controls
#define IDC_REASON		2000
#define IDC_PARAMS		2001
#define IDC_ACCEPT		2002
#define IDC_ASK			2003
#define IDC_FEIGN		2004
#define IDC_REJECT		2005
#define IDC_SILENT		2006
#define IDC_LOG			2007
#define IDC_ALERT		2008
#define IDC_TYPE		2009
#define IDC_VALUE		2010
#define IDC_PATH		2011
#define IDC_BROWSE		2012
#define IDC_PROCESS		2013
#define IDC_LANGUAGE		2014
#define IDC_PICTURE		2015
#define IDC_PROGRAMLIST		2016
#define IDC_RULELIST		2017
#define IDC_ENABLE_HOOK		2018
#define IDC_DISABLE_HOOK	2019
#define IDC_PROCESSLIST		2020
#define IDC_TABCONTROL		2021
#define IDC_SCAN		2022
#define IDC_REPORT		2023
#define IDC_NEW_FILTER		2024
#define IDC_ANTIVIRUS		2025
#define IDC_SPLASH_SCREEN	2027
#define IDC_CHECK_UPDATES	2028
#define IDC_MAX_LOG_SIZE	2030
#define IDC_IMPORT		2032
#define IDC_EXPORT		2033
#define IDC_PROGRAM		2034
#define IDC_RESET		2035
#define IDC_HISTORYLIST		2036
#define IDC_UNHOOK		2037
#define IDC_KILL_PROCESS	2038
#define IDC_VIEW_README		2039
#define IDC_VIEW_CHANGELOG	2040
#define IDC_VIEW_FAQ		2041
#define IDC_VIEW_LICENSE	2042
#define IDC_TRUSTEDFILES	2043
#define IDC_NO_SOUND		2044
#define IDC_DEFAULT_SOUNDS	2045
#define IDC_CUSTOM_SOUNDS	2046
#define IDC_BROWSE_ALERT_SOUND	2047
#define IDC_BROWSE_ASK_SOUND	2048
#define IDC_BROWSE_VIRUS_SOUND  2049
#define IDC_ALERT_SOUND		2050
#define IDC_ASK_SOUND		2051
#define IDC_VIRUS_SOUND		2052
#define IDC_TRAY_ICON_ANIMATION 2053
#define IDC_SCAN_PATTERNS	2056
#define IDC_SCAN_FOLDERS	2057
#define IDC_MAKE_DONATION	2058

// menu items
#define IDM_OPEN		10001
#define IDM_SHUTDOWN		10003
#define IDM_PROGRAM_ADD		11001
#define IDM_PROGRAM_EDIT	11002
#define IDM_PROGRAM_REMOVE	11003
#define IDM_RULE_ADD		12001
#define IDM_RULE_EDIT		12002
#define IDM_RULE_REMOVE		12003
#define IDM_RULE_UP		12004
#define IDM_RULE_DOWN		12005
#define IDM_HISTORY_CLEAR	13001
#define IDM_HISTORY_CREATE_RULE	13002
#define IDM_HISTORY_VIEWLOG	13003
#define IDM_PROCESS_HOOK	14001
#define IDM_PROCESS_UNHOOK	14002
#define IDM_PROCESS_KILL	14003
#define IDM_FOLDER_ADD		15001
#define IDM_FOLDER_REMOVE	15002
#define IDM_PATTERN_ADD		16001
#define IDM_PATTERN_REMOVE	16002
