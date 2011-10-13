//---------------------------------------------------------------------------
//
// NtInjectorThread.cpp
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
#include "NtInjectorThread.h"
#include "Injector.h"

//---------------------------------------------------------------------------
//
// class CNtInjectorThread
//
//---------------------------------------------------------------------------
CNtInjectorThread::CNtInjectorThread(CRemThreadInjector* pInjector):
	CNtProcessMonitor(),
	m_pInjector(pInjector)
{

}

CNtInjectorThread::~CNtInjectorThread()
{

}


void CNtInjectorThread::OnCreateProcess(DWORD dwProcessId)
{
	m_pInjector->InjectModuleInto(dwProcessId);
}

void CNtInjectorThread::OnTerminateProcess(DWORD dwProcessId)
{
	//
	// We shouldn't eject the module here, because the process
	// already has been terminated, meaning that the DLL probably
	// is unloaded or possibly the process is not running anymore.
	//
}

//----------------------------End of the file -------------------------------