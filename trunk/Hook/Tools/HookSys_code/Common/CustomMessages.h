//---------------------------------------------------------------------------
//
// CustomMessages.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook tool / Hook server
//				
// DESCRIPTION: Declares custom Windows messages and registers them
// 
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------
#ifndef _CUSTOMMESSAGES_H_
#define _CUSTOMMESSAGES_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define UWM_HOOKTOOL_DLL_LOADED_MSG \
	"UWM_HOOKTOOL_DLL_LOADED - {68D9B79A-09E0-4e20-9273-767C8813CA1F}"
#define UWM_HOOKTOOL_DLL_UNLOADED_MSG \
	"UWM_HOOKTOOL_DLL_UNLOADED - {68D9B79A-09E0-4e20-9273-767C8813CA1F}"

const UINT UWM_HOOKTOOL_DLL_LOADED = 
	::RegisterWindowMessage(UWM_HOOKTOOL_DLL_LOADED_MSG);
const UINT UWM_HOOKTOOL_DLL_UNLOADED = 
	::RegisterWindowMessage(UWM_HOOKTOOL_DLL_UNLOADED_MSG);


#endif //_CUSTOMMESSAGES_H_

//--------------------- End of the file -------------------------------------
