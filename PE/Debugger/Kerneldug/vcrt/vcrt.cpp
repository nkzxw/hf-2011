#include <ntifs.h>
#include <string.h>
#define NTSTRSAFE_LIB_IMPL
#include <ntstrsafe.h>

int DriverEntry (void*, void*)
{
	return (int) 0xc0000001;
}

#define ishex(c) ( ((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F') )
BOOLEAN isstrhex (char *str)
{
	ULONG len = strlen(str);
	for (ULONG i=0; i<len; i++)
	{
		if (!ishex(str[i]))
			return FALSE;
	}

	return TRUE;
}

UCHAR hexchr (char hx)
{
	if (hx >= '0' && hx <= '9')
		return hx - '0';

	if (hx >= 'a' && hx <= 'f')
		return (hx - 'a') + 10;

	if (hx >= 'A' && hx <= 'F')
		return (hx - 'A') + 10;

	return 0;
}

ULONG hextol (char *hex)
{
	int len = strlen(hex);
	ULONG accum = 0;

	for (int i=len-1; i>=0; i--)
	{
		// accum += value * ( pow (16, i) )
		
		// pow (16,i) == (1 << (i*4))

		accum += hexchr(hex[i]) * (1 << ((len-i-1)*4));
	}

	return accum;
}

int explode (char *input, char* symset, char** output, int maxarray)
{
	int nItems = 0;
	int len = strlen(input);
	char *prevItem = input;

	for (char *sp = input; sp < input+len+1; sp++)
	{
		if (strchr (symset, *sp) || *sp == 0)
		{
			//
			// Found separator
			//

			output [nItems++] = prevItem;

			if (nItems == maxarray || *(sp+1) == 0)
				return nItems;

			*sp = 0;

			while (strchr (symset, *(++sp)))
				;

			prevItem = sp;
		}
	}

	return nItems;
}