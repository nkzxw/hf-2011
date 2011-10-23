//********************************************************************
//	created:	11:8:2008   19:24
//	file:		display.cpp
//	author:		tiamo
//	purpose:	display
//********************************************************************

#include "stdafx.h"

//
// console in device id
//
ULONG BlConsoleInDeviceId								= 0;

//
// console out device id
//
ULONG BlConsoleOutDeviceId								= 1;

//
// current attribute,start with white on black
//
UCHAR TextCurrentAttribute								= 7;

//
// current column
//
USHORT TextColumn;

//
// current row
//
USHORT TextRow;

//
// output unicode string
//
VOID putwS(__in PUNICODE_STRING String)
{
	for(USHORT i = 0; i < String->Length / sizeof(WCHAR); i ++)
	{
		UCHAR Value										= static_cast<UCHAR>(String->Buffer[i]);
		ULONG Count;
		ArcWrite(BlConsoleOutDeviceId,&Value,sizeof(Value),&Count);
	}
}

//
// output hex long
//
VOID putx(__in ULONG x)
{
	if(x >> 4)
		putx(x >> 4);

	UCHAR j												= static_cast<UCHAR>(x & 0xf);
	if(j > 9)
		j												+= ('A' - 10);
	else
		j												+= '0';

	ArcWrite(BlConsoleOutDeviceId,&j,sizeof(j),&x);
}

//
// output long integer
//
VOID puti(__in LONG i)
{
	UCHAR Value;
	ULONG Count;
	if(i < 0)
	{
		i												= -i;
		Value											= '-';
		ArcWrite(BlConsoleOutDeviceId,&Value,sizeof(Value),&Count);
	}

	if(i / 10)
		puti(i / 10);

	Value												= static_cast<UCHAR>(i % 10) + '0';
	ArcWrite(BlConsoleOutDeviceId,&Value,sizeof(Value),&Count);
}

//
// output an unsigned long
//
VOID putu(__in ULONG u)
{
	if(u / 10)
		putu(u / 10);

	UCHAR Value											= static_cast<UCHAR>(u % 10) + '0';
	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,&Value,sizeof(Value),&Count);
}

//
// output char
//
PCHAR TextCharOut(__in PCHAR Buffer)
{
	if(DbcsLangId)
		return static_cast<PCHAR>(static_cast<PVOID>(TextGrCharOut(static_cast<PUCHAR>(static_cast<PVOID>(Buffer)))));

	return static_cast<PCHAR>(static_cast<PVOID>(TextTmCharOut(static_cast<PUCHAR>(static_cast<PVOID>(Buffer)))));
}

//
// output string
//
VOID TextStringOut(__in PCHAR String)
{
	if(DbcsLangId)
		TextGrStringOut(static_cast<PUCHAR>(static_cast<PVOID>(String)));
	else
		TextTmStringOut(static_cast<PUCHAR>(static_cast<PVOID>(String)));
}

//
// clear to end of line
//
VOID TextClearToEndOfLine()
{
	if(DbcsLangId)
		TextGrClearToEndOfLine();
	else
		TextTmClearToEndOfLine();
}

//
// clear from start of line
//
VOID TextClearFromStartOfLine()
{
	if(DbcsLangId)
		TextGrClearFromStartOfLine();
	else
		TextTmClearFromStartOfLine();
}

//
// clear to end of display
//
VOID TextClearToEndOfDisplay()
{
	if(DbcsLangId)
		TextGrClearToEndOfDisplay();
	else
		TextTmClearToEndOfDisplay();
}

//
// set cursor position
//
VOID TextSetCursorPosition(__in ULONG X,__in ULONG Y)
{
	TextColumn											= static_cast<USHORT>(X);
	TextRow												= static_cast<USHORT>(Y);

	if(DbcsLangId)
		TextGrPositionCursor(TextRow,TextColumn);
	else
		TextTmPositionCursor(TextRow,TextColumn);
}

//
// get cursor position
//
VOID TextGetCursorPosition(__out PULONG X,__out PULONG Y)
{
	*X													= TextColumn;
	*Y													= TextRow;
}

//
// set attribute
//
VOID TextSetCurrentAttribute(__in UCHAR Attribute)
{
	TextCurrentAttribute								= Attribute;

	if(DbcsLangId)
		TextGrSetCurrentAttribute(Attribute);
	else
		TextTmSetCurrentAttribute(Attribute);
}

//
// get attribute
//
UCHAR TextGetCurrentAttribute()
{
	return TextCurrentAttribute;
}

//
// clear display
//
VOID TextClearDisplay()
{
	if(DbcsLangId)
		TextGrClearDisplay();
	else
		TextTmClearDisplay();

	TextSetCursorPosition(0,0);
}


//
// standard printf function with a subset of formating features supported.
//
// supported
//	%d, %ld - signed short, signed long
//	%u, %lu - unsigned short, unsigned long
//	%c, %s  - character, string
//	%x, %lx - unsigned print in hex, unsigned long print in hex
//	%wS,%wZ	- unicode string
//
//	does not do:
//
//	- field width specification
//	- floating point.
//
VOID BlPrint(__in PCHAR cp,...)
{
	PVOID ap											= Add2Ptr(&cp,sizeof(PCHAR),PVOID);
	USHORT b											= 0;
	ULONG Count											= 0;
	ULONG DeviceId										= BlConsoleOutDeviceId ? BlConsoleOutDeviceId : 1;

	while(b = *cp ++)
	{
		if(b == '%')
		{
			USHORT c									= *cp ++;
			switch(c)
			{
			case 'c':
				ArcWrite(DeviceId,ap,sizeof(UCHAR),&Count);
				ap										= Add2Ptr(ap,sizeof(LONG),PVOID);
				break;

			case 'd':
				puti(*static_cast<PLONG>(ap));
				ap										= Add2Ptr(ap,sizeof(LONG),PVOID);
				break;

			case 'l':
				{
					USHORT d							= *cp ++;
					switch(d)
					{
					case 'd':
						puti(*static_cast<PLONG>(ap));
						ap								= Add2Ptr(ap,sizeof(LONG),PVOID);
						break;

					case 'u':
						putu(*static_cast<PUSHORT>(ap));
						ap								= Add2Ptr(ap,sizeof(LONG),PVOID);
						break;

					case 'x':
						{
							ULONG x						= *static_cast<PULONG>(ap);
							ULONG ZeroLength			= (x < 0x10) + (x < 0x100) + (x < 0x1000) + (x < 0x10000) + (x < 0x100000) + (x < 0x1000000) + (x < 0x10000000);
							while(ZeroLength --)
								ArcWrite(DeviceId,"0",sizeof(UCHAR),&Count);
							putx(x);
							ap							= Add2Ptr(ap,sizeof(LONG),PVOID);
						}
						break;
					}
				}
				break;

			case 's':
				{
					PCHAR String						= *static_cast<PCHAR*>(ap);
					ArcWrite(DeviceId,String,strlen(String),&Count);
					ap									= Add2Ptr(ap,sizeof(PCHAR),PVOID);
				}
				break;

			case 'u':
				putu(*static_cast<PUSHORT>(ap));
				ap										= Add2Ptr(ap,sizeof(LONG),PVOID);
				break;

			case 'w':
				{
					USHORT d							= *cp ++;
					switch(d)
					{
					case 'S':
					case 'W':
						putwS(*static_cast<PUNICODE_STRING*>(ap));
						ap								= Add2Ptr(ap,sizeof(PUNICODE_STRING),PVOID);
						break;
					}
				}
				break;

			case 'x':
				{
					USHORT x							= *static_cast<PUSHORT>(ap);
					ULONG ZeroLength					= (x < 0x10) + (x < 0x100) + (x < 0x1000);
					while(ZeroLength --)
						ArcWrite(DeviceId,"0",sizeof(UCHAR),&Count);
					putx(x);
					ap									= Add2Ptr(ap,sizeof(LONG),PVOID);
				}
				break;

			default:
				ArcWrite(DeviceId,&b,sizeof(UCHAR),&Count);
				ArcWrite(DeviceId,&c,sizeof(UCHAR),&Count);
				break;
			}
		}
		else
		{
			if(!DbcsLangId || !GrIsDBCSLeadByte(cp[-1]))
			{
				ArcWrite(DeviceId,cp - 1,1,&Count);
			}
			else
			{
				ArcWrite(DeviceId,cp - 1,2,&Count);
				cp										+= 1;
			}
		}
	}
}

WCHAR PcAnsiToUnicode[128] =
{
	0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x0087,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
	0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
	0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
	0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
	0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
	0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0,
};