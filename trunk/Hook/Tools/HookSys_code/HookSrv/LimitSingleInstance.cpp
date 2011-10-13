//---------------------------------------------------------------------------
//
// LimitSingleInstance.cpp
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook server
//
// DESCRIPTION: Controls number of the process instances
//              This code is from Q243953 in case you lose the article 
//              and wonder where this code came from...
//             
//
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "HookSrv.h"
#include "LimitSingleInstance.h"

//---------------------------------------------------------------------------
//
// class CLimitSingleInstance
//
//---------------------------------------------------------------------------

CLimitSingleInstance::CLimitSingleInstance(char* pszMutexName)
{
	// be sure to use a name that is unique for this application otherwise
	// two apps may think they are the same if they are using same name for
	// 3rd parm to CreateMutex
	m_hMutex = ::CreateMutex(NULL, FALSE, pszMutexName); //do early
	m_dwLastError = ::GetLastError(); //save for use later...
}
  
CLimitSingleInstance::~CLimitSingleInstance() 
{
	if (m_hMutex)  //don't forget to close handles...
	{
		::CloseHandle(m_hMutex); //do as late as possible
		m_hMutex = NULL;       //good habit to be in
	}
}

BOOL CLimitSingleInstance::IsAnotherInstanceRunning() 
{
	return (ERROR_ALREADY_EXISTS == m_dwLastError);
}
//----------------------------End of the file -------------------------------
