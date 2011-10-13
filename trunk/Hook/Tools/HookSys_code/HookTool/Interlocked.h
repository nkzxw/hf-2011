//---------------------------------------------------------------------------
//
// Interlocked.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook tool / Hook server
//				
// DESCRIPTION: This class signals a object when its current resource count is 0 
//              and nonsignaled when its current resource count is greater than 0.
// 
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com) 
//              This class is based on a concept suggested by Jeff Richter
//              in his book Programming Windows. For more details see "Creating 
//              Thread-Safe Datatypes and Inverse Semaphores" 
// DATE:		2002 November v1.00
//
//---------------------------------------------------------------------------
#ifndef _INTERLOCKED_H_
#define _INTERLOCKED_H_

#pragma once

// Instances of this class will be accessed by multiple processes. 
template <class T> 
class CWhenZero 
{
public:
	CWhenZero(BOOL bInitToZero):
		m_InitToZero(bInitToZero),
		m_mtx(NULL),
		m_hfm(NULL),
		m_hevtZero(NULL),
		m_pSharedRefCounter(NULL)
	{
		char szMMF[100];
		m_mtx = ::CreateMutexA(NULL, FALSE, "{85773247-082B-4804-8E44-1DD6AB96E41C}");
		strcpy(szMMF, "HookTool_RefCounter");
		m_hfm = ::CreateFileMappingA((HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE, 0, sizeof(T), szMMF);
		if (m_hfm != NULL) 
		{
			m_pSharedRefCounter = (T*)::MapViewOfFile(m_hfm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			m_hevtZero = ::CreateEventA(NULL, TRUE, bInitToZero, "{604CD81D-0B3A-480d-96DC-96A263A97E2D}");
			if (bInitToZero)
				SetRefCount(0);	
			else
				IncRefCount();
			// The event should be signaled if value is 0
		}
	}

	~CWhenZero() 
	{
		if (!m_InitToZero)
			DecRefCount();
		::UnmapViewOfFile(m_pSharedRefCounter);
		::CloseHandle(m_hevtZero);
		::CloseHandle(m_mtx);
		::CloseHandle(m_hfm);
	}

	// Return handle to event signaled when value is zero
	HANDLE GetZeroHandle() const 
	{ 
		return m_hevtZero; 
	}

private:
	void SetRefCount(T value)
	{
		::WaitForSingleObject(m_mtx, INFINITE);
		*m_pSharedRefCounter = value;
		OnValueChanged(*m_pSharedRefCounter);
		::ReleaseMutex(m_mtx);
	}
	void IncRefCount()
	{
		::WaitForSingleObject(m_mtx, INFINITE);
		++(*m_pSharedRefCounter);
		OnValueChanged(*m_pSharedRefCounter);
		::ReleaseMutex(m_mtx);
	}
	void DecRefCount()
	{
		::WaitForSingleObject(m_mtx, INFINITE);
		--(*m_pSharedRefCounter);
		OnValueChanged(*m_pSharedRefCounter);
		::ReleaseMutex(m_mtx);
	}
	void OnValueChanged(const T& newVal) const 
	{ 
		// For best performance, avoid jumping to 
		// kernel mode if we don't have to
		if (newVal == 0)
		{
			::SetEvent(m_hevtZero);
		}
		else
		{
			::ResetEvent(m_hevtZero);
		}
	}

private:
	BOOL m_InitToZero;
	HANDLE m_mtx;
	HANDLE m_hfm;
	HANDLE m_hevtZero;      // Signaled when data value is 0
	T* m_pSharedRefCounter;

};


#endif //_INTERLOCKED_H_

//--------------------- End of the file -------------------------------------
