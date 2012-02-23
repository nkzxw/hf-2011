#ifndef _LOCK_H_
#define _LOCK_H_

/*
*  �����д�������ܵݹ飬Ҳ��������.
*  �ݹ������ʵ��̫���ӻ����Ͼ���Ҫ��һ����ǰȡ�ö������߳�ID�б���һ���µĿ�����������ģʽ
*/

class CCriticalLock
{
private:
    CRITICAL_SECTION section;
public:
    CCriticalLock()
    {
        ::InitializeCriticalSectionAndSpinCount(&section,0x800);
    }
    
    ~CCriticalLock()
    {
        DeleteCriticalSection(&section);
    }
    
    void Lock()
    {
        EnterCriticalSection(&section);
    }
    
    void Unlock()
    {
        LeaveCriticalSection(&section);
    }
    
    operator const LPCRITICAL_SECTION()
    {
    	return &section;
    }
};

class CAutoCLock
{
private:
    CCriticalLock *pcs;
public:
    CAutoCLock(CCriticalLock &cs)
    	:pcs(&cs)
    {
        pcs->Lock();
    }
    ~CAutoCLock()
    {
        pcs->Unlock();
    }
};

class CMyEvent
{
protected:
    HANDLE  hEvent;
public:
    CMyEvent(BOOL bManualReset=FALSE, BOOL bInitialState=FALSE, LPTSTR lpName=NULL)
    	:hEvent(NULL)
    {
        hEvent=::CreateEvent(NULL,bManualReset,bInitialState,lpName);
    }
    
    CMyEvent(DWORD  dwDesiredAccess, BOOL  bInheritHandle, LPCTSTR  lpName)
    	:hEvent(NULL)
    {
        hEvent=::OpenEvent(dwDesiredAccess,bInheritHandle,lpName );
    }
    
    ~CMyEvent()
    {
        if(hEvent!=NULL)
        {
            ::CloseHandle(hEvent);
        }
    }
    
    inline operator HANDLE()
    {
    	return hEvent;
    }
    
    inline BOOL  SetEvent()
    {
        return ::SetEvent(hEvent);
    }
    inline BOOL  PulseEvent()
    {
        return ::PulseEvent(hEvent);
    }
    
    inline BOOL  ResetEvent()
    {
        return ::ResetEvent(hEvent);
    }
    
    inline BOOL Wait(DWORD time = INFINITE)
    {
        return WAIT_TIMEOUT != ::WaitForSingleObject(hEvent,time);
    }
};

class ReadWriteLock
{
private:
    enum{
    	LOCK_LEVEL_NONE,
    	LOCK_LEVEL_WRITE,
    	LOCK_LEVEL_READ
    };
    
    int	m_currentLevel;									//��ǰ״̬
    
	int	m_readCount;									//������  
    
	CMyEvent m_unlockEvent;								//���л��ȴ��źţ��ֶ��źţ�״̬�л��� m_csStateChange����

    CCriticalLock m_accessMutex;						//lock�������뱣����lock����ֻ��һ���̷߳���
    
	CCriticalLock m_csStateChange;						//�ڲ�״̬�޸ı���
    
	ReadWriteLock(const ReadWriteLock&){}
    
	void operator=(const ReadWriteLock&){}
public:
    ReadWriteLock()
    	:m_unlockEvent(TRUE, FALSE)
    {
        m_currentLevel = LOCK_LEVEL_NONE;
        m_readCount    = 0;
    }
    
    ~ReadWriteLock()
    {
    }

    bool readlock(DWORD waittime=INFINITE)
    {
        CAutoCLock al(m_accessMutex);
        if(m_currentLevel == LOCK_LEVEL_WRITE)//����Ҫ��ȡ��������ʹ�ж�ʱд�����ˣ����Ҳ�ǵ���һ������ȴ������̷���
        {
            if(!m_unlockEvent.Wait(waittime))
                return false;
        }

        CAutoCLock al2(m_csStateChange);
        m_currentLevel = LOCK_LEVEL_READ;
        m_readCount ++;
        m_unlockEvent.ResetEvent();

        return true;
    }
    
    bool writelock(DWORD waittime=INFINITE)
    {
        CAutoCLock al(m_accessMutex);
        if(m_currentLevel != LOCK_LEVEL_NONE)//ͬ��
        {
            if(!m_unlockEvent.Wait(waittime))
                return false;
        }

        CAutoCLock al2(m_csStateChange);
        m_currentLevel = LOCK_LEVEL_WRITE;
        m_unlockEvent.ResetEvent();
        
		return true;
    } // lock()

    bool readunlock()
    {
        CAutoCLock al2(m_csStateChange);
        if ( m_currentLevel != LOCK_LEVEL_READ )
            return false;

        m_readCount --;
        if ( m_readCount == 0 )
        {
            m_currentLevel = LOCK_LEVEL_NONE;
            m_unlockEvent.SetEvent();
        }
        return true;
    }
    
    bool writeunlock()
    {
        CAutoCLock al2(m_csStateChange);
        if ( m_currentLevel != LOCK_LEVEL_WRITE )
            return false;
        
		m_currentLevel = LOCK_LEVEL_NONE;
        m_unlockEvent.SetEvent();
        return true;
    }
};

class CAutoReadLock
{
private:
    ReadWriteLock *rwl;
public:
    CAutoReadLock(ReadWriteLock &lock)
    	:rwl(&lock)
    {
        rwl->readlock();
    }
    
    ~CAutoReadLock()
    {
        //ATLVERIFY(rwl->readunlock());
		rwl->readunlock ();
    }
};

class CAutoReadTryLock
{
private:
    ReadWriteLock *rwl;
    bool locked;
public:
    CAutoReadTryLock(ReadWriteLock &lock,DWORD waittime=0)
    	:rwl(&lock)
    {
        locked=rwl->readlock(waittime);
    }
    
    bool Locked() const {return locked;}
    
    ~CAutoReadTryLock()
    {
        if(locked)
        {
            //ATLVERIFY(rwl->readunlock());
			rwl->readunlock ();
        }
    }
};

class CAutoWriteLock
{
private:
    ReadWriteLock *rwl;
public:
    CAutoWriteLock(ReadWriteLock &lock)
    	:rwl(&lock)
    {
        rwl->writelock();
    }
    
    ~CAutoWriteLock()
    {
        //ATLVERIFY(rwl->writeunlock());
		rwl->writeunlock ();
    }
};

class CAutoWriteTryLock
{
private:
    ReadWriteLock *rwl;
    bool locked;
public:
    CAutoWriteTryLock(ReadWriteLock &lock,DWORD waittime=0)
    	:rwl(&lock)
    {
        locked=rwl->writelock(waittime);
    }
    
    bool Locked() const {return locked;}
    
    ~CAutoWriteTryLock()
    {
        if(locked)
        {
            //ATLVERIFY(rwl->writeunlock());
			rwl->writeunlock ();
        }
    }
};


/*
//�����ʹ�÷������ȴ���һ��ReadWriteLockʵ����Ȼ�����������ĸ��Զ��������������ʹ�á���

ReadWriteLock lock;
{
	CAutoReadLock al(lock);
	i++;
}

*/
#endif //_LOCK_H_