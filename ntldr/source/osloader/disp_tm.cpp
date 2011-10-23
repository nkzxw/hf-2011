//********************************************************************
//	created:	13:8:2008   6:16
//	file:		disp_tm.cpp
//	author:		tiamo
//	purpose:	text mode display
//********************************************************************

#include "stdafx.h"

//
// video buffer start
//
#define VIDEO_BUFFER_VA									0xb8000

//
// rows count
//
#define VIDEO_ROWS										25

//
// columns count
//
#define VIDEO_COLUMNS									80

//
// bytes per row
//
#define VIDEO_BYTES_PER_ROW								(VIDEO_COLUMNS * 2)

//
// screen location
//
PUCHAR Vp												= reinterpret_cast<PUCHAR>(VIDEO_BUFFER_VA);

//
// set attribute
//
VOID TextTmSetCurrentAttribute(__in UCHAR Attribute)
{

}

//
// set cursor position
//
VOID TextTmPositionCursor(__in USHORT Row,__in USHORT Column)
{
	if(Row >= VIDEO_ROWS)
		Row												= VIDEO_ROWS - 1;

	if(Column >= VIDEO_COLUMNS)
		Column											= VIDEO_COLUMNS - 1;

	Vp													= Add2Ptr(VIDEO_BUFFER_VA,Row * VIDEO_BYTES_PER_ROW + 2 * Column,PUCHAR);
}

//
// clear from current location to the end of line
//
VOID TextTmClearToEndOfLine()
{
	//
	// calculate address of current cursor position
	//
	PUSHORT p											= Add2Ptr(VIDEO_BUFFER_VA,TextRow * VIDEO_BYTES_PER_ROW,PUSHORT) + TextColumn;

	//
	// fill with blanks up to end of line.
	//
	for(USHORT u = TextColumn; u < VIDEO_COLUMNS; u ++,p ++)
		*p												= (TextCurrentAttribute << 8) + ' ';
}

//
// clear from start of line
//
VOID TextTmClearFromStartOfLine()
{
	//
	// Calculate address of start of line in video buffer
	//
	PUSHORT p											= Add2Ptr(VIDEO_BUFFER_VA,TextRow * VIDEO_BYTES_PER_ROW,PUSHORT);

	//
	// fill with blanks up to char before cursor position.
	//
	for(USHORT u = 0; u < TextColumn; u ++, p ++)
		*p												= (TextCurrentAttribute << 8) + ' ';
}

//
// clear to end of display
//
VOID TextTmClearToEndOfDisplay()
{
	//
	// clear current line
	//
	TextTmClearToEndOfLine();

	//
	// clear the remaining lines
	//
	PUSHORT p											= Add2Ptr(VIDEO_BUFFER_VA,(TextRow + 1) * VIDEO_BYTES_PER_ROW,PUSHORT);
	for(USHORT y = TextRow + 1; y < VIDEO_ROWS; y ++)
	{
		for(USHORT x = 0; x < VIDEO_COLUMNS; x ++,p ++)
			*p											= (TextCurrentAttribute << 8) + ' ';
	}
}

//
// clear display
//
VOID TextTmClearDisplay()
{
	//
	// write blanks in the current attribute to the entire screen.
	//
	for(ULONG u = 0; u < VIDEO_ROWS * VIDEO_COLUMNS; u ++)
		Add2Ptr(VIDEO_BUFFER_VA,0,PUSHORT)[u]			= (TextCurrentAttribute << 8) + ' ';
}

//
// scrolls the display up one line. The cursor position is not changed.
//
VOID TextTmScrollDisplay()
{
	PUSHORT Dp											= reinterpret_cast<PUSHORT>(VIDEO_BUFFER_VA);
	PUSHORT Sp											= Add2Ptr(VIDEO_BUFFER_VA,VIDEO_BYTES_PER_ROW,PUSHORT);

	//
	// move each row up one row
	//
	for(USHORT i = 0 ; i < VIDEO_ROWS - 1 ; i ++)
	{
		for(USHORT j = 0; j < VIDEO_COLUMNS; j ++)
			*Dp++										= *Sp++;
	}

	//
	// write blanks in the bottom line, using the attribute from the leftmost char on the bottom line on the screen.
	//
	USHORT c											= (*Dp & 0xff00) + ' ';

	for(USHORT i = 0; i < VIDEO_COLUMNS; i ++)
		*Dp++											= c;
}

//
// writes a character on the display at the current position
//
PUCHAR TextTmCharOut(__in PUCHAR pc)
{
	UCHAR c												= *pc;
	switch(c)
	{
	case '\n':
		if(TextRow == VIDEO_ROWS -1)
		{
			TextTmScrollDisplay();
			TextSetCursorPosition(0,TextRow);
		}
		else
		{
			TextSetCursorPosition(0,TextRow + 1);
		}
		break;

		//
		// ignore
		//
	case '\r':
		break;

	case '\t':
		{
			UCHAR temp										= ' ';
			USHORT u										= 8 - TextColumn % 8;
			USHORT u2										= u;
			while(u--)
				TextTmCharOut(&temp);

			TextSetCursorPosition(TextColumn + u2 - 1,TextRow);
		}
		break;

	default :
		*Vp++												= c;
		*Vp++												= TextCurrentAttribute;
		TextSetCursorPosition(TextColumn + 1,TextRow);
	}

	return pc + 1;
}

//
// output string
//
VOID TextTmStringOut(__in PUCHAR String)
{
	PUCHAR p												= String;
	while(*p)
		p													= TextTmCharOut(p);
}
