// AntiHook.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CAntiHookApp:
// �йش����ʵ�֣������ AntiHook.cpp
//

class CAntiHookApp : public CWinApp
{
public:
	CAntiHookApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CAntiHookApp theApp;