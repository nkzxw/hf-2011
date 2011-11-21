//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/31
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
//

#ifndef __COMMON_FUNCTION_H__
#define __COMMON_FUNCTION_H__

#define ONE_DAY_SECONDS			86400
#define SECONDS_OF_1980_1970	315504000	// 1980-01-01 - 1970-01-01 's seconds
#define WEEK_OF_1980_01_01		3			// 1980-01-01 is Tuesday
#define WEEK_OF_1970_01_01		5			// 1950-01-01 is Satuday

extern ULONG GetCurrentTime(unsigned char* Week, ULONG* pTime);

#endif //__COMMON_FUNCTION_H__
