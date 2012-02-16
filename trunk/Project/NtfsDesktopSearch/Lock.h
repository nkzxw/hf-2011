// Lock.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
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