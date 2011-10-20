/*
Released under MIT License

Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Visit http://www.codeplex.com/easyhook for more information.
*/
#include "EasyHookDriver.h"

#ifndef _M_X64
	#error "This driver part is intended for 64-bit builds only."
#endif

/****************************************************************************
*************************************** PgIsPatchGuardContext
*****************************************************************************

Description:

	Distinguishes between common DPC parameters and PatchGuard contexts.
	This is done my using a combination of pseudo random value detection
	and attribute filtering.

*/
BOOLEAN CheckSubValue(ULONGLONG InValue)
{
	ULONG			i;
	ULONG			Result;
	UCHAR*			Chars = (UCHAR*)&InValue;

	// random values will have a result around 120...
	Result = 0;

	for(i = 0; i < 8; i++)
	{
		Result += ((Chars[i] & 0xF0) >> 4) + (Chars[i] & 0x0F);
	}

	// the maximum value is 240, so this should be safe...
	if(Result < 70)
		return TRUE;

	return FALSE;
}

BOOLEAN PgIsPatchGuardContext(void* Ptr)
{
	ULONGLONG		Value = (ULONGLONG)Ptr;
	UCHAR*			Chars = (UCHAR*)&Value;
	LONG			i;

	// this is a requirement for a canonical pointer...
	if((Value & 0xFFFF000000000000) == 0xFFFF000000000000)
		return FALSE;

	if((Value & 0xFFFF000000000000) == 0)
		return FALSE;

	// sieve out other common values...
	if(CheckSubValue(Value) || CheckSubValue(~Value))
		return FALSE;

	if(Ptr == NULL)
		return FALSE;

	//This must be the last check and filters latin-char UTF16 strings...
	for(i = 7; i >= 0; i -= 2)
	{
		if(Chars[i] != 0)
			return TRUE;
	}

	// this should only return true if the pointer is a unicode string!!!
	return FALSE;
}