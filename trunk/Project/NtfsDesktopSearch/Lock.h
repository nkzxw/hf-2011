// Lock.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

class CLock
{
    CRITICAL_SECTION m_cs;
public:
    CLock(){
        InitializeCriticalSection(&m_cs);
    }
    ~CLock(){
        DeleteCriticalSection(&m_cs);
    }
    void Lock(){
        EnterCriticalSection(&m_cs);
    }
    void UnLock(){
        LeaveCriticalSection(&m_cs);
    }
};