// CBTHook.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CCBTHookApp:
// �йش����ʵ�֣������ CBTHook.cpp
//

class CCBTHookApp : public CWinApp
{
public:
	CCBTHookApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CCBTHookApp theApp;