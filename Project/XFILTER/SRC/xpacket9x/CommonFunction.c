//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/31
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
//

#include "xprecomp.h"
#pragma hdrstop

//
// 得到当前日期/时间、星期和时间
//
// 参数：
//		Week:	返回星期，1、2 ... 7 分别表示 星期日、星期一 ... 星期日
//		pTime:	返回当前时间在当天的秒数
//
// 返回值：
//		ULONG数值表示的日期/时间，保存的是从1970/1/1到现在的秒数，与CTime表示形式相同
//
//
//
ULONG GetCurrentTime(unsigned char* Week, ULONG* pTime)
{
	ULONG Date, Time, DateTime;
	Time = VTD_Get_Date_And_Time(&Date) / 1000;
	DateTime = Time + (Date * ONE_DAY_SECONDS) + SECONDS_OF_1980_1970;
	*Week = (unsigned char)(Date % 7 + WEEK_OF_1980_01_01);
	if(*Week > 7)*Week -= 7;
	*pTime = Time;

	dprintf("DateTime: %u, DayCount:%u, DayTime:%u, Week:%u\n"
		, DateTime
		, Date
		, Time
		, *Week);

	return DateTime;
}

#pragma comment( exestr, "B9D3B8FD2A65716F6F717068777065766B71702B")
