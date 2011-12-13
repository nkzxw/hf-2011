// hf_pack.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Chf_packApp:
// See hf_pack.cpp for the implementation of this class
//

class Chf_packApp : public CWinApp
{
public:
	Chf_packApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Chf_packApp theApp;