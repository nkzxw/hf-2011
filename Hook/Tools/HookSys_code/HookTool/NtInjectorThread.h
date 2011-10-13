//---------------------------------------------------------------------------
//
// NtInjectorThread.h
//
// SUBSYSTEM: 
//				API Hooking system
// MODULE:    
//				Implements a thread that uses an NT device driver
//              for monitoring process creation
//
// DESCRIPTION:
//
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
//                                                                         
//---------------------------------------------------------------------------
#if !defined(_NTINJECTORTHREAD_H_)
#define _NTINJECTORTHREAD_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//---------------------------------------------------------------------------
//
// Includes
//
//---------------------------------------------------------------------------

#include "NtProcessMonitor.h"

//---------------------------------------------------------------------------
//
// Forward declararions
//
//---------------------------------------------------------------------------

class CRemThreadInjector;

//---------------------------------------------------------------------------
//
// class CNtInjectorThread
//
//---------------------------------------------------------------------------
class CNtInjectorThread: public CNtProcessMonitor  
{
public:
	CNtInjectorThread(CRemThreadInjector* pInjector);
	virtual ~CNtInjectorThread();
private:
	virtual void OnCreateProcess(DWORD dwProcessId);
	virtual void OnTerminateProcess(DWORD dwProcessId);
	CRemThreadInjector* m_pInjector;
};

#endif // !defined(_NTINJECTORTHREAD_H_)
//----------------------------End of the file -------------------------------