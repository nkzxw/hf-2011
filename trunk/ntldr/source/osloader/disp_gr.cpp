//********************************************************************
//	created:	13:8:2008   6:44
//	file:		disp_gr.cpp
//	author:		tiamo
//	purpose:	graph display
//********************************************************************

#include "stdafx.h"

//
// video buffer start
//
#define VIDEO_BUFFER_VA									0xa0000

//
// bytes per line
//
#define VIDEO_BYTES_PER_SCAN_LINE						80

//
// width pixels
//
#define VIDEO_WIDTH_PIXELS								640

//
// height
//
#define VIDEO_HEIGHT_SCAN_LINES							480

//
// size
//
#define VIDEO_SIZE_BYTES								(VIDEO_BYTES_PER_SCAN_LINE * VIDEO_HEIGHT_SCAN_LINES)

//
// per text row
//
#define VIDEO_BYTES_PER_TEXT_ROW						(VIDEO_BYTES_PER_SCAN_LINE * CharacterCellHeight)

//
// width for single byte
//
#define SBCSWIDTH										8

//
// width for multi byte
//
#define DBCSWIDTH										16

//
// current position
//
PUCHAR GrVp												= reinterpret_cast<PUCHAR>(VIDEO_BUFFER_VA);

//
// lead byte table. read from bootfont.bin.
//
UCHAR													LeadByteTable[2 * (MAX_DBCS_RANGE + 1)];

//
// height
//
ULONG													CharacterImageHeight;

//
// top padding
//
ULONG													CharacterTopPad;

//
// bottom padding
//
ULONG													CharacterBottomPad;

//
// language id
//
ULONG													DbcsLangId;

//
// dbcs count
//
LONG													DbcsCharCount;

//
// dbcs images
//
PUCHAR													DbcsImages;

//
// sbcs count
//
LONG													SbcsCharCount;

//
// sbcs images
//
PUCHAR													SbcsImages;

//
// cell height
//
ULONG													CharacterCellHeight;

//
// screen cell width
//
ULONG													ScreenWidthCells;

//
// screen cell height
//
ULONG													ScreenHeightCells;

//
// vga base register
//
PUCHAR													VgaRegisterBase;

//
// displays a DBCS or a SBCS character at the current cursor position.
//
VOID GrDisplayMBCSChar(__in PUCHAR Image,__in ULONG Width,__in UCHAR Top,__in UCHAR Bottom)
{
	//
	// validate parameter
	//
	if(!Image)
		return;

	//
	// save old position
	//
	PUCHAR VpOld										= GrVp;

	//
	// there are TOP_EXTRA lines at the top that we need to skip (background color).
	//
	for(ULONG i = 0; i < CharacterTopPad; i ++)
	{
		//
		// If DBCS char, we need to clear 2 bytes.
		//
		if(Width == DBCSWIDTH)
			*GrVp++										= Top;

		*GrVp++											= Top;

		//
		// position pointer at next scan line for the font image.
		//
		GrVp											+= VIDEO_BYTES_PER_SCAN_LINE - Width / SBCSWIDTH;
	}

	//
	// display full height of DBCS or SBCS char.
	//
	for(ULONG i = 0; i < CharacterImageHeight; i ++)
	{
		//
		// if DBCS char, need to display 2 bytes, so display first byte here.
		//
		if(Width == DBCSWIDTH)
			*GrVp++										= *Image++;

		//
		// display 2nd byte of DBCS char or the first and only byte of SBCS char.
		//
		*GrVp++											= *Image++;

		//
		// increment GrVP to display location of next row of font image.
		//
		GrVp											+= VIDEO_BYTES_PER_SCAN_LINE - Width / SBCSWIDTH;
	}

	//
	// there are BOT_EXTRA lines at the bottom that we need to fill with the background color.
	//
	for(ULONG i = 0; i < CharacterBottomPad; i ++)
	{
		//
		// if DBCS char, need to clear 2 bytes
		//
		if(Width == DBCSWIDTH)
			*GrVp++										= Bottom;

		*GrVp++											= Bottom;

		//
		// position pointer at next scan line for the font image.
		//
		GrVp											+= VIDEO_BYTES_PER_SCAN_LINE - Width / SBCSWIDTH;
	}

	//
	// increment cursor and video pointer
	//
	if(Width == DBCSWIDTH)
		TextSetCursorPosition(TextColumn + 2,TextRow);
	else
		TextSetCursorPosition(TextColumn + 1,TextRow);
}

//
// if a char is a DBCS leadbyte.
//
BOOLEAN GrIsDBCSLeadByte(__in UCHAR c)
{
	//
	// check to see if char is in leadbyte range.
	// BUGBUG:  If (CHAR)(0) is a valid leadbyte, this routine will fail.
	//
	for(ULONG i = 0; LeadByteTable[i]; i += 2)
	{
		if(LeadByteTable[i] <= c && LeadByteTable[i + 1] >= c)
			return TRUE;
	}

	return FALSE;
}

//
// get font image
//
PUCHAR GrGetDBCSFontImage(__in USHORT Code)
{
	LONG Min											= 0;
	LONG Max											= DbcsCharCount;
	LONG Multiplier										= 2 * CharacterImageHeight + 4;

	//
	// binary search for the image.
	//
	while(Max >= Min)
	{
		LONG Mid										= (Max + Min) / 2;
		LONG Index										= Mid * Multiplier;

		USHORT code										= (DbcsImages[Index] << 8) | DbcsImages[Index + 1];

		if(Code == code)
			return DbcsImages + Index + 2;

		if(Code < code)
			Max											= Mid - 1;
		else
			Min											= Mid + 1;
	}

	return 0;
}

//
// get sbcs image
//
PUCHAR GrGetSBCSFontImage(__in UCHAR Code)
{
	LONG Min											= 0;
	LONG Max											= SbcsCharCount;
	LONG Multiplier										= CharacterImageHeight + 3;

	//
	// binary search for the image.
	//
	while(Max >= Min)
	{
		LONG Mid										= (Max + Min) / 2;
		LONG Index										= Mid * Multiplier;

		if(Code == SbcsImages[Index])
			return SbcsImages + Index + 1;

		if(Code < SbcsImages[Index])
			Max											= Mid - 1;
		else
			Min											= Mid + 1;
	}

	return 0;
}

//
// sets the attribute by setting up various VGA registers.
//
VOID TextGrSetCurrentAttribute(__in UCHAR Attribute)
{
	//
	// address of GVRAM off the screen.
	//
	PUCHAR volatile OffTheScreen						= reinterpret_cast<PUCHAR>(0xa9600);

	union WordOrByte
	{
		struct Word
		{
			unsigned short    ax;
		}x;

		struct Byte
		{
			unsigned char     al, ah;
		}h;
	}regs;

	//
	// reset Data Rotate/Function Select regisger.
	// need to reset Data Rotate/Function Select.
	//
	WRITE_PORT_USHORT(Add2Ptr(VgaRegisterBase,0x3ce,PUSHORT),0x3 );

	//
	// set Enable Set/Reset to all (0f).
	//
	WRITE_PORT_USHORT(Add2Ptr(VgaRegisterBase,0x3ce,PUSHORT),0xf01);

	//
	// put background color into Set/Reset register.
	// this is done to put the background color into the latches later.
	// put BLUE color in Set/Reset register.
	//
	regs.x.ax											= (Attribute & 0x0f0) << 4;
	WRITE_PORT_USHORT(Add2Ptr(VgaRegisterBase,0x3ce,PUSHORT),regs.x.ax);

	//
	// put Set/Reset register value into GVRAM off the screen.
	//
	UCHAR temp											= 0;
	*OffTheScreen										= temp;

	//
	// read from screen, so the latches will be updated with the background color.
	//
	temp												= *OffTheScreen;

	//
	// set Data Rotate/Function Select register to be XOR.
	//
	WRITE_PORT_USHORT(Add2Ptr(VgaRegisterBase,0x3ce,PUSHORT),0x1803);

	//
	// XOR the foreground and background color and put it in Set/Reset register.
	//
	regs.h.ah											= (Attribute >> 4) ^ (Attribute & 0x0f);
	regs.h.al											= 0;
	WRITE_PORT_USHORT(Add2Ptr(VgaRegisterBase,0x3ce,PUSHORT),regs.x.ax);

	//
	// put Inverse(~) of the XOR of foreground and ground attribute into Enable Set/Reset register.
	//
	regs.x.ax											= ~regs.x.ax & 0x0f01;
	WRITE_PORT_USHORT(Add2Ptr(VgaRegisterBase,0x3ce,PUSHORT),regs.x.ax);
}

//
// set position
//
VOID TextGrPositionCursor(__in USHORT Row,__in USHORT Column)
{
	if(Row >= ScreenHeightCells)
		Row												= static_cast<USHORT>(ScreenHeightCells - 1);

	if(Column >= ScreenWidthCells)
		Column											= static_cast<USHORT>(ScreenWidthCells - 1);

	GrVp												= Add2Ptr(VIDEO_BUFFER_VA,Row * VIDEO_BYTES_PER_TEXT_ROW + Column,PUCHAR);
}

//
// clear display
//
VOID TextGrClearDisplay()
{
	for(ULONG i = 0; i < VIDEO_SIZE_BYTES; i ++)
		*Add2Ptr(VIDEO_BUFFER_VA,i,PUCHAR)				= 0x00;
}

//
// initialize
//
VOID TextGrInitialize(__in ULONG DiskId,__out_opt PULONG TotalLength)
{
	ULONG FileId;
	PCHAR FilePath										= "\\BOOTFONT.BIN";
	CHAR NetPath[132];
	if(TotalLength)
		*TotalLength									= 0;

	if(BlBootingFromNet)
	{
		strcpy(NetPath,NetBootPath);
		strcat(NetPath,"BOOTFONT.BIN");
		FilePath										= NetPath;
	}

	//
	// open file
	//
	ARC_STATUS Status									= BlOpen(DiskId,FilePath,ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
		return;

	//
	// read in the file header and check some values.
	// we enforce the width of 8/16 here.if this is changed code all over the rest of this module must also be changed.
	//
	BOOTFONTBIN_HEADER Header;
	ULONG Length;
	Status												= BlRead(FileId,&Header,sizeof(Header),&Length);
	if( Status == ESUCCESS && Length == sizeof(Header) && Header.Signature == BOOTFONTBIN_SIGNATURE &&
		Header.CharacterImageDbcsWidth == DBCSWIDTH && Header.CharacterImageSbcsWidth == SBCSWIDTH)
	{
		//
		// calculate the amount of memory needed to hold the sbcs and dbcs character entries.
		// each sbcs entry is 1 byte for the ascii value followed by n bytes for the image itself.we assume a width of 8 pixels.
		// for dbcs chars each entry is 2 bytes for the codepoint and n bytes for the image itself. we assume a width of 16 pixels.
		//
		// also perform further validation on the file by comparing the sizes given in the header against a size we calculate.
		//
		ULONG SbcsSize									= Header.NumSbcsChars * (Header.CharacterImageHeight + 1);
		ULONG DbcsSize									= Header.NumDbcsChars * ((2 * Header.CharacterImageHeight) + 2);

		if(TotalLength)
			*TotalLength								= SbcsSize + DbcsSize + sizeof(BOOTFONTBIN_HEADER);

		if(SbcsSize != Header.SbcsEntriesTotalSize && DbcsSize != Header.DbcsEntriesTotalSize)
		{
			//
			// allocate memory to hold the font.
			// we use FwAllocatePool() because that routine uses a separate heap that was inititialized before the high-level Bl memory system was initialized
			//
			PUCHAR FontImage							= static_cast<PUCHAR>(FwAllocatePool(SbcsSize + DbcsSize));
			if(FontImage)
			{
				//
				// the entries get read into the base of the region we carved out. the dbcs images get read in immediately after that.
				//
				SbcsImages								= FontImage;
				DbcsImages								= SbcsImages + Header.SbcsEntriesTotalSize;

				//
				// read in the sbcs entries.
				//
				LARGE_INTEGER SeekOffset;
				SeekOffset.QuadPart						= Header.SbcsOffset;

				if( BlSeek(FileId,&SeekOffset,SeekAbsolute) == ESUCCESS &&
					BlRead(FileId,SbcsImages,Header.SbcsEntriesTotalSize,&Length) == ESUCCESS &&
					Length == Header.SbcsEntriesTotalSize)
				{
					//
					// read in the dbcs entries.
					//
					SeekOffset.QuadPart					= Header.DbcsOffset;
					if( BlSeek(FileId,&SeekOffset,SeekAbsolute) == ESUCCESS &&
						BlRead(FileId,DbcsImages,Header.DbcsEntriesTotalSize,&Length) == ESUCCESS &&
						Length == Header.DbcsEntriesTotalSize)
					{
						//
						// set up various values used for displaying the font.
						//
						DbcsLangId						= Header.LanguageId;
						CharacterImageHeight			= Header.CharacterImageHeight;
						CharacterTopPad					= Header.CharacterTopPad;
						CharacterBottomPad				= Header.CharacterBottomPad;
						CharacterCellHeight				= CharacterImageHeight + CharacterTopPad + CharacterBottomPad;
						SbcsCharCount					= Header.NumSbcsChars;
						DbcsCharCount					= Header.NumDbcsChars;
						ScreenWidthCells				= VIDEO_WIDTH_PIXELS / Header.CharacterImageSbcsWidth;
						ScreenHeightCells				= VIDEO_HEIGHT_SCAN_LINES / CharacterCellHeight;

						RtlMoveMemory(LeadByteTable,Header.DbcsLeadTable,(MAX_DBCS_RANGE + 1)*2);

						//
						// wwitch the display into 640x480 graphics mode and clear it.
						//
						ExternalServicesTable->HardwareCursor(0x80000000,0x12);

						TextClearDisplay();
					}
				}
			}
		}
	}

	if(FileId)
		BlClose(FileId);
}

//
// shutdown
//
VOID TextGrTerminate()
{
	if(DbcsLangId)
		DbcsLangId										= 0;

	//
	// switches the display into 80x25 text mode.
	//
	extern BOOLEAN GraphicsMode;
	if(!GraphicsMode)
		ExternalServicesTable->HardwareCursor(0x80000000,0x3);
}

//
// scrolls the display up one line
//
VOID TextGrScrollDisplay()
{
	PUCHAR Source										= Add2Ptr(VIDEO_BUFFER_VA,VIDEO_BYTES_PER_TEXT_ROW,PUCHAR);
	PUCHAR Dest											= reinterpret_cast<PUCHAR>(VIDEO_BUFFER_VA);
	ULONG n												= VIDEO_BYTES_PER_TEXT_ROW * (ScreenHeightCells - 1);

	for(ULONG i = 0; i < n; i ++)
		*Dest++											= *Source++;

	//
	// write blanks in the bottom line, using the current attribute.
	//
	ULONG OldX;
	ULONG OldY;
	TextGetCursorPosition(&OldX,&OldY);

	TextSetCursorPosition(0,ScreenHeightCells - 1);

	UCHAR temp											= ' ';
	for(ULONG i = 0; i < ScreenWidthCells; i ++)
		TextGrCharOut(&temp);

	TextSetCursorPosition(OldX,OldY);
}

//
// display sbcs
//
VOID GrWriteSBCSChar(__in UCHAR c)
{
	switch(c)
	{
	case '\n':
		if(TextRow == ScreenHeightCells - 1)
		{
			TextGrScrollDisplay();
			TextSetCursorPosition(0,TextRow);
		}
		else
		{
			TextSetCursorPosition(0,TextRow + 1);
		}
		break;

	case '\r':
		break;

	case '\t':
		{
			UCHAR temp									= ' ';
			USHORT u									= 8 - (TextColumn % 8);
			USHORT u2									= u;
			while(u--)
				TextGrCharOut(&temp);

			TextSetCursorPosition(TextColumn + u2 - 1,TextRow);
		}
		break;

	default:
		{
			//
			// get font image for SBCS char.
			//
			PUCHAR pImage								= GrGetSBCSFontImage(c);

			//
			// check for special graphics characters. Add top and bottom extra pixels accordingly
			// (otherwise the grids don't connect properly, because of top and bottom spacing).
			//
			if (c == 0x2 || c == 0x1 || c == 0x16)
				GrDisplayMBCSChar(pImage,SBCSWIDTH,0x00,0x24);
			else if (c == 0x4 || c == 0x3 || c == 0x15)
				GrDisplayMBCSChar(pImage,SBCSWIDTH,0x24,0x00);
			else if (c == 0x5 || c == 10 || c == 0x17 || c == 0x19)
				GrDisplayMBCSChar(pImage,SBCSWIDTH,0x24,0x24);
			else
				GrDisplayMBCSChar(pImage,SBCSWIDTH,0x00,0x00);
		}
		break;
	}
}

//
// displays a mixed byte string
//
ULONG GrWriteMBCSString(__in PUCHAR String,__in ULONG MaxChars)
{
	ULONG BytesWritten									= 0;

	//
	// while string is not NULL, get font image and display it.
	//
	while(*String && MaxChars--)
	{
		//
		// determine if char is SBCS or DBCS, get the correct font image, and display it.
		//
		if(GrIsDBCSLeadByte(*String))
		{
			USHORT DBCSChar								= *String ++ << 8;
			DBCSChar									|= *String ++;
			PUCHAR pImage								= GrGetDBCSFontImage(DBCSChar);

			GrDisplayMBCSChar(pImage,DBCSWIDTH,0x00,0x00);

			BytesWritten								+= 1;
		}
		else
		{
			GrWriteSBCSChar(*String ++);
		}

		BytesWritten									+= 1;
	}

	return BytesWritten;
}

//
// output string
//
VOID TextGrStringOut(__in PUCHAR String)
{
	GrWriteMBCSString(String,0xffffffff);
}

//
// output char
//
PUCHAR TextGrCharOut(__in PUCHAR pc)
{
	return pc + GrWriteMBCSString(pc,1);
}

//
// clear to end of line
//
VOID TextGrClearToEndOfLine()
{
	//
	// fill with blanks up to char before cursor position.
	//
	UCHAR temp											= ' ';
	ULONG OldX;
	ULONG OldY;
	TextGetCursorPosition(&OldX,&OldY);

	for(ULONG u = TextColumn; u < ScreenWidthCells; u ++)
		TextGrCharOut(&temp);

	TextSetCursorPosition(OldX,OldY);
}

//
// clear from start of line
//
VOID TextGrClearFromStartOfLine()
{
	//
	// fill with blanks up to char before cursor position.
	//
	UCHAR temp											= ' ';
	ULONG OldX;
	ULONG OldY;
	TextGetCursorPosition(&OldX,&OldY);
	TextSetCursorPosition(0,OldY);

	for(ULONG u = 0; u < TextColumn; u ++)
		TextGrCharOut(&temp);

	TextSetCursorPosition(OldX,OldY);
}

//
// clear to end of display
//
VOID TextGrClearToEndOfDisplay()
{
	//
	// clear current line
	//
	TextGrClearToEndOfLine();

	//
	// clear the remaining lines
	//
	PUCHAR p											= Add2Ptr(VIDEO_BUFFER_VA,(TextRow + 1)*VIDEO_BYTES_PER_TEXT_ROW,PUCHAR);
	for(USHORT y = TextRow + 1; y < ScreenHeightCells; y ++)
	{
		for(USHORT x = 0; x < VIDEO_BYTES_PER_TEXT_ROW; x ++)
			*p++										= 0;
	}
}

//
// utf8 encoding
//
VOID UTF8Encode(__in WCHAR Unicode,__out PUCHAR UTF8Buffer)
{
	UTF8Buffer[0]										= 0;
	UTF8Buffer[1]										= 0;

	if(Unicode & 0xff80)
	{
		UTF8Buffer[2]									= static_cast<UCHAR>(Unicode & 0x3f) | 0x80;

		if(Unicode & 0xf700)
		{
			UTF8Buffer[1]								= (static_cast<UCHAR>(Unicode >> 6) & 0x3f) | 0x80;
			UTF8Buffer[0]								= (static_cast<UCHAR>(Unicode >> 12) & 0x0f) | 0x0e;
		}
		else
		{
			UTF8Buffer[1]								= (static_cast<UCHAR>(Unicode >> 6) & 0x1f) | 0xc0;
		}
	}
	else
	{
		UTF8Buffer[2]									= static_cast<UCHAR>(Unicode & 0x7f);
	}
}

//
// convert to utf8
//
VOID GetDBCSUtf8Translation(__in PUCHAR InputBuffer,__out PUCHAR UTF8Buffer)
{
	LONG Min											= 0;
	LONG Max											= DbcsCharCount;
	LONG Multiplier										= 2 * CharacterImageHeight + 4;
	USHORT Code											= (InputBuffer[0] << 8) | InputBuffer[1];
	UTF8Buffer[0]										= 0;
	UTF8Buffer[1]										= 0;
	UTF8Buffer[2]										= 0;

	//
	// binary search for the image.
	//
	while(Max >= Min)
	{
		LONG Mid										= (Max + Min) / 2;
		LONG Index										= Mid * Multiplier;

		USHORT Value										= (DbcsImages[Index] << 8) | DbcsImages[Index + 1];

		if(Code == Value)
			return UTF8Encode((DbcsImages[Index + 0x23] << 8) | DbcsImages[Index + 0x23],UTF8Buffer);

		if(Code < Value)
			Max											= Mid - 1;
		else
			Min											= Mid + 1;
	}
}

//
// convert to utf8
//
VOID GetSBCSUtf8Translation(__in PUCHAR InputBuffer,__out PUCHAR UTF8Buffer)
{
	LONG Min											= 0;
	LONG Max											= SbcsCharCount;
	LONG Multiplier										= CharacterImageHeight + 3;
	UCHAR Code											= *InputBuffer;
	UTF8Buffer[0]										= 0;
	UTF8Buffer[1]										= 0;
	UTF8Buffer[2]										= 0;

	//
	// binary search for the image.
	//
	while(Max >= Min)
	{
		LONG Mid										= (Max + Min) / 2;
		LONG Index										= Mid * Multiplier;

		if(Code == SbcsImages[Index])
			return UTF8Encode((SbcsImages[Index + 0x12] << 8) | SbcsImages[Index + 0x11],UTF8Buffer);

		if(Code < SbcsImages[Index])
			Max											= Mid - 1;
		else
			Min											= Mid + 1;
	}
}
