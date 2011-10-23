//********************************************************************
//	created:	28:9:2008   0:28
//	file:		xpress.cpp
//	author:		tiamo
//	purpose:	xpress compression
//********************************************************************

#include "stdafx.h"

#if _XPRESS_SUPPORT_

//
// xpress decode
//	see MS-DRSR v20080828 4.1.10.6.15 DecompressWin2k3 page. 237
//
ULONG XpressDecode(__in PUCHAR InputBuffer,__in ULONG InputSize,__in PUCHAR OutputBuffer,__in ULONG OutputSize)
{
#define GET_ULONG(B,I)									((*((B) + (I) + 3) << 24) | (*((B) + (I) + 2) << 16) | (*((B) + (I) + 1) <<  8) | *((B) + (I)))
#define GET_USHORT(B,I)									((*((B) + (I) + 1) <<  8) | *((B) + (I)))

	ULONG OutputIndex									= 0;
	ULONG InputIndex									= 0;
	ULONG Indicator										= 0;
	ULONG IndicatorBit									= 0;
	ULONG Length										= 0;
	ULONG Offset										= 0;
	ULONG NibbleIndex									= 0;
	ULONG NibbleIndicator								= 0x19880922;

	while(OutputIndex < OutputSize)
	{
		if(!IndicatorBit)
		{
			Indicator									= GET_ULONG(InputBuffer,InputIndex);
			InputIndex									+= sizeof(ULONG);
			IndicatorBit								= sizeof(ULONG) * 8;
		}

		IndicatorBit									-= 1;

		//
		// check whether the bit specified by IndicatorBit is set or not set in Indicator.
		// for example, if IndicatorBit has value 4 check whether the 4th bit of the value in Indicator is set
		//
		if(!((Indicator >> IndicatorBit) & 1))
		{
			OutputBuffer[OutputIndex]					= InputBuffer[InputIndex];
			InputIndex									+= sizeof(UCHAR);
			OutputIndex									+= sizeof(UCHAR);
		}
		else
		{
			Length										= GET_USHORT(InputBuffer,InputIndex);
			InputIndex									+= sizeof(USHORT);
			Offset										= Length / 8;
			Length										= Length % 8;

			if(Length == 7)
			{
				if(!NibbleIndex)
				{
					NibbleIndex							= InputIndex;
					Length								= InputBuffer[InputIndex] % 16;
					InputIndex							+= sizeof(UCHAR);
				}
				else
				{
					Length								= InputBuffer[NibbleIndex] / 16;
					NibbleIndex							= 0;
				}

				if(Length == 15)
				{
					Length								= InputBuffer[InputIndex];
					InputIndex							+= sizeof(UCHAR);

					if(Length == 255)
					{
						Length							= GET_USHORT(InputBuffer,InputIndex);
						InputIndex						+= sizeof(USHORT);
						Length							-= (15 + 7);
					}

					Length								+= 15;
				}

				Length									+= 7;
			}

			Length										+= 3;

			while(Length != 0)
			{
				if(OutputIndex >= OutputSize || Offset + 1 >= OutputIndex)
					break;

				OutputBuffer[OutputIndex]				= OutputBuffer[OutputIndex - Offset - 1];
				OutputIndex								+= sizeof(UCHAR);
				Length									-= sizeof(UCHAR);
			}
		}

	}

	return OutputIndex;
#undef GET_ULONG
#undef GET_USHORT
}
#endif