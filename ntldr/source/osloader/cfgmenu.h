//********************************************************************
//	created:	20:9:2008   5:29
//	file: 		cfgmenu.h
//	author:		tiamo
//	purpose:	config menu
//********************************************************************

#pragma once

//
// start config prompt
//
VOID BlStartConfigPrompt();

//
// end config prompt
//
BOOLEAN BlEndConfigPrompt();

//
// display config menu
//
BOOLEAN BlConfigMenuPrompt(__in ULONG Timeout,__inout PBOOLEAN UseLastKnownGood,__inout PHCELL_INDEX ControlSetKeyCell,
						   __inout PCM_HARDWARE_PROFILE_LIST* ProfileList,__inout PCM_HARDWARE_DOCK_INFO_LIST* DockInfoList);
