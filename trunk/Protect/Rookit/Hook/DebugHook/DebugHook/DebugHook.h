// DebugHook.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CDebugHookApp:
// �йش����ʵ�֣������ DebugHook.cpp
//

class CDebugHookApp : public CWinApp
{
public:
	CDebugHookApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CDebugHookApp theApp;