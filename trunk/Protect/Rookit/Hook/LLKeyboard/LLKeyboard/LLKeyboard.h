// LLKeyboard.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CLLKeyboardApp:
// �йش����ʵ�֣������ LLKeyboard.cpp
//

class CLLKeyboardApp : public CWinApp
{
public:
	CLLKeyboardApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CLLKeyboardApp theApp;