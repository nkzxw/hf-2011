// QDING_KYODAI.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CQDING_KYODAIApp:
// �йش����ʵ�֣������ QDING_KYODAI.cpp
//

class CQDING_KYODAIApp : public CWinApp
{
public:
	CQDING_KYODAIApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CQDING_KYODAIApp theApp;