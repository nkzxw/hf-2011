//********************************************************************
//	created:	11:8:2008   19:30
//	file:		display.h
//	author:		tiamo
//	purpose:	display
//********************************************************************

#pragma once

//
// print to screen
//
VOID BlPrint(__in PCHAR cp,...);

//
// terminate display
//
VOID TextGrTerminate();

//
// output char
//
PCHAR TextCharOut(__in PCHAR Buffer);

//
// output text string
//
VOID TextStringOut(__in PCHAR Buffer);

//
// clear to end of line
//
VOID TextClearToEndOfLine();

//
// clear from start of line
//
VOID TextClearFromStartOfLine();

//
// clear to end of display
//
VOID TextClearToEndOfDisplay();

//
// set cursor position
//
VOID TextSetCursorPosition(__in ULONG X,__in ULONG Y);

//
// get cursor position
//
VOID TextGetCursorPosition(__out PULONG X,__out PULONG Y);

//
// set attribute
//
VOID TextSetCurrentAttribute(__in UCHAR Attribute);

//
// get attribute
//
UCHAR TextGetCurrentAttribute();

//
// set attribute
//
VOID TextTmSetCurrentAttribute(__in UCHAR Attribute);

//
// set cursor position
//
VOID TextTmPositionCursor(__in USHORT Row,__in USHORT Column);

//
// clear display
//
VOID TextClearDisplay();

//
// clear from current location to the end of line
//
VOID TextTmClearToEndOfLine();

//
// clear from start of line
//
VOID TextTmClearFromStartOfLine();

//
// clear to end of display
//
VOID TextTmClearToEndOfDisplay();

//
// clear display
//
VOID TextTmClearDisplay();

//
// output char
//
PUCHAR TextTmCharOut(__in PUCHAR pc);

//
// output string
//
VOID TextTmStringOut(__in PUCHAR String);

//
// if a char is a DBCS leadbyte.
//
BOOLEAN GrIsDBCSLeadByte(__in UCHAR c);

//
// set position
//
VOID TextGrPositionCursor(__in USHORT Row,__in USHORT Column);

//
// sets the attribute by setting up various VGA registers.
//
VOID TextGrSetCurrentAttribute(__in UCHAR Attribute);

//
// clear display
//
VOID TextGrClearDisplay();

//
// initialize
//
VOID TextGrInitialize(__in ULONG DiskId,__out_opt PULONG TotalLength);

//
// shutdown
//
VOID TextGrTerminate();

//
// output string
//
VOID TextGrStringOut(__in PUCHAR String);

//
// output char
//
PUCHAR TextGrCharOut(__in PUCHAR pc);

//
// clear to end of line
//
VOID TextGrClearToEndOfLine();

//
// clear from start of line
//
VOID TextGrClearFromStartOfLine();

//
// clear to end of display
//
VOID TextGrClearToEndOfDisplay();

//
// convert to utf8
//
VOID GetDBCSUtf8Translation(__in PUCHAR InputBuffer,__out PUCHAR UTF8Buffer);

//
// convert to utf8
//
VOID GetSBCSUtf8Translation(__in PUCHAR InputBuffer,__out PUCHAR UTF8Buffer);

//
// encode utf8
//
VOID UTF8Encode(__in WCHAR Unicode,__out PUCHAR UTF8Buffer);

//
// console in device id
//
extern ULONG											BlConsoleInDeviceId;

//
// console out device id
//
extern ULONG											BlConsoleOutDeviceId;

//
// current attribute,start with white on black
//
extern UCHAR											TextCurrentAttribute;

//
// current column
//
extern USHORT											TextColumn;

//
// current row
//
extern USHORT											TextRow;

//
// dbcs language id
//
extern ULONG											DbcsLangId;

//
// ansi to unicode convertor table
//
extern WCHAR											PcAnsiToUnicode[128];