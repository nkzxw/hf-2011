// CheckModule.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CCheckModuleApp:
// �йش����ʵ�֣������ CheckModule.cpp
//

class CCheckModuleApp : public CWinApp
{
public:
	CCheckModuleApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()

};

extern CCheckModuleApp theApp;