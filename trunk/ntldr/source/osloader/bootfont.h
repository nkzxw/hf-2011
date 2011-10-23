//********************************************************************
//	created:	13:8:2008   6:56
//	file:		bootfont.h
//	author:		tiamo
//	purpose:	boot font
//********************************************************************

#pragma once

//
// maximum number of dbcs lead byte ranges we support.
//
#define MAX_DBCS_RANGE									5

//
// signature value.
//
#define BOOTFONTBIN_SIGNATURE							0x5465644d

//
// header for the bootfont.bin file.
//
#include "pshpack1.h"
typedef struct _BOOTFONTBIN_HEADER
{
	//
	// signature
	//
	ULONG												Signature;

	//
	// language id of the language supported by this font.
	//
	ULONG												LanguageId;

	//
	// number of sbcs characters contained in the file.
	//
	ULONG												NumSbcsChars;

	//
	// number of dbcs characters contained in the file.
	//
	ULONG												NumDbcsChars;

	//
	// offsets within the file to the images.
	//
	ULONG												SbcsOffset;

	//
	// offsets within the file to the images.
	//
	ULONG												DbcsOffset;

	//
	// total sizes of the images.
	//
	ULONG												SbcsEntriesTotalSize;

	//
	// total sizes of the images.
	//
	ULONG												DbcsEntriesTotalSize;

	//
	// Dbcs lead byte table. Must contain a pair of 0's to indicate the end.
	//
	UCHAR												DbcsLeadTable[(MAX_DBCS_RANGE + 1) * 2];

	//
	// height in scan lines/pixels of the font image
	//
	UCHAR CharacterImageHeight;

	//
	// top padding
	//
	UCHAR												CharacterTopPad;

	//
	// bottom padding
	//
	UCHAR												CharacterBottomPad;

	//
	// width values for a single byte character
	//
	UCHAR												CharacterImageSbcsWidth;

	//
	// width values for a single byte character
	//
	UCHAR												CharacterImageDbcsWidth;
}BOOTFONTBIN_HEADER,*PBOOTFONTBIN_HEADER;

//
// images themselves follow.
//
// first there are SbcsCharacters entries for single-byte chars.the first byte in each entry is the ascii char code.
// the next n bytes are the image. n is dependent on the width and height of an sbcs char.
//
// following these are the dbcs images.the first 2 bytes are the dbcs character code (highbyte lowbyte) and the next n bytes are the image.
// n is dependent on the width and height of a dbcs char.
//
// important note: the characters must be sorted in ascending order!
//

//
// oem font version
//
#define OEM_FONT_VERSION								0x200

//
// oem font type
//
#define OEM_FONT_TYPE									0

//
// oem font italic
//
#define OEM_FONT_ITALIC									0

//
// oem font underline
//
#define OEM_FONT_UNDERLINE								0

//
// oem font strikeout
//
#define OEM_FONT_STRIKEOUT								0

//
// oem font character set
//
#define OEM_FONT_CHARACTER_SET							255

//
// oem font family
//
#define OEM_FONT_FAMILY									(3 << 4)

//
// oem bitmapped font file header structure.
//
typedef struct _OEM_FONT_FILE_HEADER
{
	//
	// version
	//
	USHORT												Version;

	//
	// size
	//
	ULONG												FileSize;

	//
	// copy right string
	//
	UCHAR												Copyright[60];

	//
	// type
	//
	USHORT												Type;

	//
	// points
	//
	USHORT												Points;

	//
	// vert res
	//
	USHORT												VerticleResolution;

	//
	// horz res
	//
	USHORT												HorizontalResolution;

	//
	// ascent
	//
	USHORT												Ascent;

	//
	// internal leading
	//
	USHORT												InternalLeading;

	//
	// external leading
	//
	USHORT												ExternalLeading;

	//
	// italic
	//
	UCHAR												Italic;

	//
	// under line
	//
	UCHAR												Underline;

	//
	// strike out
	//
	UCHAR												StrikeOut;

	//
	// weigth
	//
	USHORT												Weight;

	//
	// character set
	//
	UCHAR												CharacterSet;

	//
	// pixel width
	//
	USHORT												PixelWidth;

	//
	// pixel height
	//
	USHORT												PixelHeight;

	//
	// family
	//
	UCHAR												Family;

	//
	// avg. width
	//
	USHORT												AverageWidth;

	//
	// max width
	//
	USHORT												MaximumWidth;

	//
	// first character
	//
	UCHAR												FirstCharacter;

	//
	// last character
	//
	UCHAR												LastCharacter;

	//
	// default character
	//
	UCHAR												DefaultCharacter;

	//
	// break character
	//
	UCHAR												BreakCharacter;

	//
	// width in bytes
	//
	USHORT												WidthInBytes;

	//
	// device
	//
	ULONG												Device;

	//
	// face
	//
	ULONG												Face;

	//
	// bits pointer
	//
	ULONG												BitsPointer;

	//
	// bits offset
	//
	ULONG												BitsOffset;

	//
	// filler
	//
	UCHAR												Filler;

	//
	// map
	//
	struct
	{
		//
		// width
		//
		USHORT											Width;

		//
		// offset
		//
		USHORT											Offset;
	}Map[1];
}OEM_FONT_FILE_HEADER,*POEM_FONT_FILE_HEADER;

#include "poppack.h"