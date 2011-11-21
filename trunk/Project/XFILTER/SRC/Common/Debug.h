//=============================================================================================
/*
	Debug.h
	The Debug Header Function

	Project	: XFILTER 1.0 Personal Firewall
	Author	: Tony Zhu
	Create Date	: 2001/08/03
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================
#ifndef DEBUG_H
#define DEBUG_H

#ifdef _DEBUG

	#define ODS(fmt)					\
	{									\
		OutputDebugString(fmt);			\
		OutputDebugString(_T("\n"));	\
	}

	#define ODS2(fmt,fmt2)				\
	{									\
		OutputDebugString(fmt);			\
		OutputDebugString(fmt2);		\
		OutputDebugString(_T("\n"));	\
	}
	
	#define DP1(fmt, var)				\
	{									\
		TCHAR sOut[256];				\
		_stprintf(sOut, fmt, var);		\
		OutputDebugString(sOut);		\
	}

#else

	#define ODS(fmt)
	#define ODS2(fmt,fmt2)

	#define DP1(fmt, x1)			

#endif

#endif//DEBUG_H
