// GlobalApp.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CGlobalAppApp:
// �йش����ʵ�֣������ GlobalApp.cpp
//

class CGlobalAppApp : public CWinApp
{
public:
	CGlobalAppApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CGlobalAppApp theApp;