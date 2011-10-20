/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		misc.c
 *
 * Abstract:
 *
 *		This module implements various miscellaneous routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 24-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "misc.h"
#include "process.h"
#include "userland.h"
#include "policy.h"
#include "token.h"


BOOLEAN		BootingUp = FALSE;


/*
 * atoi()
 *
 * Description:
 *		Convert a given string to an integer.
 *
 * Parameters:
 *		buf - Pointer to a source character string. 
 *
 * Returns:
 *		String's integer value (0 in case of an error).
 */

INT32
atoi(IN PCHAR buf)
{
    int ret = 0;


    while (*buf >= '0' && *buf <= '9')
	{
       ret *= 10;
       ret += *buf - '0';
       buf++;
    }


    return ret;
}



/*
 * itoa()
 *
 * Description:
 *		Convert an integer to a string.
 *
 * Parameters:
 *		value - Number to be converted. 
 *		string - String result. 
 *		radix - Base of value; must be in the range 2 – 36. 
 *
 * Returns:
 *		Pointer to a string.
 */

PCHAR
itoa(int value, char *string, unsigned int radix)
{
	static const char	base36[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	PCHAR				s;
	int					tmp = value;
	int					digits = 1;
	

	ASSERT(radix > 1 && radix < (int) sizeof base36);
	ASSERT(string);
	ASSERT(value >= 0);

	while ((tmp /= (int)radix) != 0)
		++digits;
   
	s = string;
   
	string += digits; 
	*string = '\0';

	do {

		*--string = base36[value % radix];

    } while ((value /= radix) != 0);


	return string;
}



/*
 * ntohl()
 *
 * Description:
 *		Convert a ULONG from TCP/IP network order to host byte order (which is little-endian on Intel processors).
 *
 * Parameters:
 *		netlong - 32-bit number in TCP/IP network byte order. 
 *
 * Returns:
 *		The value in host byte order.
 *		If the netlong parameter was already in host byte order, then no operation is performed.
 */

ULONG
ntohl(IN ULONG netlong)
{
	ULONG	result = 0;

	((char *)&result)[0] = ((char *)&netlong)[3];
	((char *)&result)[1] = ((char *)&netlong)[2];
	((char *)&result)[2] = ((char *)&netlong)[1];
	((char *)&result)[3] = ((char *)&netlong)[0];

	return result;
}



/*
 * ntohs()
 *
 * Description:
 *		Convert a USHORT from TCP/IP network byte order to host byte order (which is little-endian on Intel processors).
 *
 * Parameters:
 *		netshort - 16-bit number in TCP/IP network byte order.
 *
 * Returns:
 *		The value in host byte order.
 *		If the netshort parameter was already in host byte order, then no operation is performed.
 */

USHORT
ntohs(IN USHORT netshort)
{
	USHORT	result = 0;

	((char *)&result)[0] = ((char *)&netshort)[1];
	((char *)&result)[1] = ((char *)&netshort)[0];

	return result;
}


/*
 * htonl()
 *
 * Description:
 *		Converts a ULONG from host to TCP/IP network byte order (which is big-endian).
 *
 * Parameters:
 *		hostlong - 32-bit number in host byte order.
 *
 * Returns:
 *		The value in TCP/IP's network byte order.
 */

ULONG
htonl(ULONG hostlong)
{
	ULONG	r1, r2, r3, r4;

	r1 = (hostlong >> 24);
	r2 = (hostlong << 24);
	r3 = (hostlong & 0xff00) << 8;
	r4 = (hostlong >> 8) & 0xff00;
	
	return (r1 | r2 | r3 | r4);
}



/*
 * inet_aton()
 *
 * Description:
 *		Convert a ULONG from host to TCP/IP network byte order (which is big-endian).
 *
 * Parameters:
 *		hostlong - 32-bit number in host byte order.
 *
 * Returns:
 *		returns the value in TCP/IP's network byte order.
 */

#define	_islower(c)		( ((c) >= 'a' && (c) <= 'f')  )
#define	_isxdigit(c)	( ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F') )
#define	_isascii(c)		( (c) <= 0177 )
#define	_isdigit(c)		( ((c) >= '0' && (c) <= '9') )
#define	_isspace(c)		( ((c) == ' ' ) )

//XXX verify... can invalid IPs be accepted as valid ones?


/*
 * inet_addr() XXX rewrite
 *
 * Description:
 *		Convert a string containing an (IPv4) Internet Protocol dotted address into a ULONG (network order).
 *
 * Parameters:
 *		cp - Null-terminated character string representing a number expressed in the Internet
 *			 standard ".'' (dotted) notation.
 *
 * Returns:
 *		A ULONG value containing a suitable binary representation of the Internet address given.
 */

ULONG
inet_addr(PCCHAR cp)
{
	ULONG			val;
	int				base, n;
	char			c;
	unsigned int	parts[4];
	unsigned int	*pp = parts;

	c = *cp;
	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, isdigit=decimal.
		 */
		if (!_isdigit(c))
			return (0);

		val = 0; base = 10;
//XXX get rid of multiple base support?
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X')
				base = 16, c = *++cp;
			else
				base = 8;
		}

		for (;;) {
			if (_isascii(c) && _isdigit(c)) {
				val = (val * base) + (c - '0');
				c = *++cp;
			} else if (base == 16 && _isascii(c) && _isxdigit(c)) {
				val = (val << 4) |
					(c + 10 - (_islower(c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}

		if (c == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16 bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3)
				return (0);
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (c != '\0' && (!_isascii(c) || !_isspace(c)))
		return (0);
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 0:
		return (0);		/* initial nondigit */

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}

	return htonl(val);
}



/*
 * inet_ntoa()
 *
 * Description:
 *		Convert an (IPv4) Internet network address into a string in Internet standard dotted format.
 *
 * Parameters:
 *		ina - ULONG that represents an Internet host address. 
 *
 * Returns:
 *		A character pointer to a static buffer containing the text address in standard ".'' notation.
 */

//XXX not SMP safe
PCHAR
inet_ntoa2(ULONG ina)
{
	static char		buf[4*sizeof "123"];
	unsigned char	*ucp = (unsigned char *)&ina;

	sprintf(buf, "%d.%d.%d.%d",
					ucp[0] & 0xff,
					ucp[1] & 0xff,
					ucp[2] & 0xff,
					ucp[3] & 0xff);

	return buf;
}

/* XXX move to netmisc.c or network.c */
VOID
inet_ntoa(ULONG ina, PCHAR buf)
{
	unsigned char	*ucp = (unsigned char *)&ina;

	sprintf(buf, "%d.%d.%d.%d",
					ucp[0] & 0xff,
					ucp[1] & 0xff,
					ucp[2] & 0xff,
					ucp[3] & 0xff);
}



/*
 * VerifyUnicodeString()
 *
 * Description:
 *		Make sure Unicode String buffer is readable.
 *
 * Parameters:
 *		InputUnicodeString - input unicode buffer.
 *		OutputUnicodeString - output buffer.
 *
 * Returns:
 *		TRUE to indicate success, FALSE is failed.
 */

#define	FINISH_VerifyUnicodeString(msg)					\
	do {												\
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, msg);		\
		return(FALSE);									\
	} while(0)

BOOLEAN
VerifyUnicodeString(IN PUNICODE_STRING InputUnicodeString, OUT PUNICODE_STRING OutputUnicodeString)
{
	if (! ARGUMENT_PRESENT(InputUnicodeString) )

		FINISH_VerifyUnicodeString(("VerifyUnicodeString: Unicode string is NULL\n"));


	/* this code is duplicated in pathproc.c!GetPathFromHandle for efficiency */
	__try
	{
		if (KeGetPreviousMode() != KernelMode)
		{
			ProbeForRead(InputUnicodeString, sizeof(UNICODE_STRING), sizeof(ULONG));

			*OutputUnicodeString = ProbeAndReadUnicodeString(InputUnicodeString);
		}
		else
		{
			*OutputUnicodeString = *InputUnicodeString;
		}


		if (OutputUnicodeString->Length == 0)

			return FALSE;


		if (((OutputUnicodeString->Length & (sizeof(WCHAR) - 1)) != 0) ||
			(OutputUnicodeString->Length > bMAX_PATH - sizeof(WCHAR)) )

			FINISH_VerifyUnicodeString(("VerifyUnicodeString: invalid wchar string length = %d\n", OutputUnicodeString->Length));


		if (KeGetPreviousMode() != KernelMode)

			ProbeForRead(OutputUnicodeString->Buffer, OutputUnicodeString->Length, sizeof(WCHAR));
	}

	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		NTSTATUS status = GetExceptionCode();

		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("VerifyUnicodeString: caught an exception. status = 0x%x\n", status));

		return(FALSE);
	}

	return(TRUE);
}



/*
 * VerifyPwstr()
 *
 * Description:
 *		Make sure PWSTR buffer is readable.
 *
 * Parameters:
 *		InputString - input PWSTR buffer.
 *		InputStringLength - input string length.
 *
 * Returns:
 *		TRUE to indicate success, FALSE is failed.
 */

#define	FINISH_VerifyPwstr(msg)							\
	do {												\
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, msg);		\
		return(FALSE);									\
	} while(0)

BOOLEAN
VerifyPwstr(IN PWSTR InputString, IN ULONG InputStringLength)
{
	if (! ARGUMENT_PRESENT(InputString) )

		FINISH_VerifyPwstr(("VerifyPwstr: Input string is NULL\n"));


	if ((InputStringLength == 0) ||
		((InputStringLength & (sizeof(WCHAR) - 1)) != 0) ||
		(InputStringLength > bMAX_PATH - sizeof(WCHAR)) )

		FINISH_VerifyPwstr(("VerifyPwstr: invalid wchar string length = %d\n", InputStringLength));


	if (KeGetPreviousMode() != KernelMode)
	{
		__try
		{
			ProbeForRead(InputString, InputStringLength, sizeof(WCHAR));
		}

		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			NTSTATUS status = GetExceptionCode();

			LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("VerifyPwstr: caught an exception. address=%x, status = 0x%x\n", InputString, status));

			return(FALSE);
		}
	}


	return(TRUE);
}



/*
 * ReadStringRegistryValue()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		TRUE to indicate success, FALSE is failed.
 */

#define	FINISH_ReadStringRegistryValue(msg)				\
	do {												\
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, msg);		\
		if (vpip) ExFreePoolWithTag(vpip, _POOL_TAG);	\
		if (hKey) ZwClose(hKey);						\
		return(NULL);									\
	} while(0)

PKEY_VALUE_PARTIAL_INFORMATION
ReadStringRegistryValue(IN PWSTR RegistryPath, IN PWSTR KeyName, OUT ULONG *VPIPSize)
{
	OBJECT_ATTRIBUTES				oa;
	HANDLE							hKey = 0;
	UNICODE_STRING					usRegistryPath, usKeyName;
	ULONG							size;
	NTSTATUS						status;
	PKEY_VALUE_PARTIAL_INFORMATION	vpip = NULL;
	PCHAR							FunctionName = "ReadStringRegistryValue";


	if (RegistryPath == NULL || KeyName == NULL)

		FINISH_ReadStringRegistryValue(("%s: Received NULL parameter. %x %x\n", RegistryPath, KeyName));


	RtlInitUnicodeString(&usRegistryPath, RegistryPath);

	InitializeObjectAttributes(&oa, &usRegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	status = ZwOpenKey(&hKey, KEY_QUERY_VALUE , &oa);
	if (! NT_SUCCESS(status))

		FINISH_ReadStringRegistryValue(("%s: ZwOpenKey('%S') failed with status=%x\n", FunctionName, RegistryPath, status));


	RtlInitUnicodeString(&usKeyName, KeyName);

	status = ZwQueryValueKey(hKey, &usKeyName, KeyValuePartialInformation, NULL, 0, &size);
	if (status == STATUS_OBJECT_NAME_NOT_FOUND || size == 0)

		FINISH_ReadStringRegistryValue(("%s: ZwQueryValueKey(%S) failed with status=%x\n", FunctionName, KeyName, status));


	vpip = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePoolWithTag(PagedPool, size, _POOL_TAG);

	if (!vpip)

		FINISH_ReadStringRegistryValue(("%s: out of memory\n", FunctionName));


	status = ZwQueryValueKey(hKey, &usKeyName, KeyValuePartialInformation, vpip, size, &size);
	if (! NT_SUCCESS(status))

		FINISH_ReadStringRegistryValue(("%s: ZwQueryValueKey2(%S) failed with status=%x\n", FunctionName, KeyName, status));


	if (vpip->Type != REG_SZ)

		FINISH_ReadStringRegistryValue(("%s: Wrong Type: %x\n", FunctionName, vpip->Type));


	ZwClose(hKey);


	*VPIPSize = size;


	return vpip;
}



/*
 * ReadStringRegistryValueA()
 *
 * Description:
 *		Read an ASCII value from the registry.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		TRUE to indicate success, FALSE is failed.
 */

BOOLEAN
ReadStringRegistryValueA(IN PWSTR RegistryPath, IN PWSTR KeyName, OUT PCHAR Buffer, IN USHORT BufferSize)
{
	PKEY_VALUE_PARTIAL_INFORMATION	vpip;
	ULONG							size;


	ASSERT(Buffer);


	if ((vpip = ReadStringRegistryValue(RegistryPath, KeyName, &size)) == NULL)
		return FALSE;


	if (_snprintf(Buffer, BufferSize, "%S", (PWSTR) &vpip->Data) < 0)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%s: Buffer is too small %d vs %d\n", BufferSize, size));
		ExFreePoolWithTag(vpip, _POOL_TAG);
		return FALSE;
	}
	Buffer[BufferSize - 1] = 0;


	ExFreePoolWithTag(vpip, _POOL_TAG);


	return TRUE;
}



/*
 * ReadStringRegistryValueW()
 *
 * Description:
 *		Read a UNICODE value from the registry.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		TRUE to indicate success, FALSE is failed.
 */

BOOLEAN
ReadStringRegistryValueW(IN PWSTR RegistryPath, IN PWSTR KeyName, OUT PWSTR Buffer, IN USHORT BufferSize)
{
	PKEY_VALUE_PARTIAL_INFORMATION	vpip;
	ULONG							size;


	ASSERT(Buffer);


	if ((vpip = ReadStringRegistryValue(RegistryPath, KeyName, &size)) == NULL)
		return FALSE;


	if ((size - sizeof(KEY_VALUE_PARTIAL_INFORMATION)) + 1 > BufferSize)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%s: Buffer is too small %d vs %d\n", BufferSize, size));
		ExFreePoolWithTag(vpip, _POOL_TAG);
		return FALSE;
	}

	wcsncpy(Buffer, (PWSTR) vpip->Data, BufferSize);
	Buffer[BufferSize - 1] = 0;


	ExFreePoolWithTag(vpip, _POOL_TAG);


	return TRUE;
}



/*
 * ReadSymlinkValue()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		TRUE to indicate success, FALSE is failed.
 */

#define	FINISH_ReadSymlinkValue(msg)					\
	do {												\
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, msg);		\
		if (hLink) ZwClose(hLink);						\
		return(FALSE);									\
	} while(0)

BOOLEAN
ReadSymlinkValue(IN PWSTR SymlinkPath, OUT PCHAR Buffer, IN USHORT BufferSize)
{
	OBJECT_ATTRIBUTES			oa;
	HANDLE						hLink = 0;
	UNICODE_STRING				usLink;
	ANSI_STRING					asLink;
	WCHAR						Link[MAX_PATH];
	ULONG						size;
	NTSTATUS					status;


	RtlInitUnicodeString(&usLink, SymlinkPath);
	InitializeObjectAttributes(&oa, &usLink, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

	status = ZwOpenSymbolicLinkObject(&hLink, GENERIC_READ, &oa);
	if (! NT_SUCCESS(status))

		FINISH_ReadSymlinkValue(("ReadSymlinkValue: Unable to open '%S' link. status=%x\n", SymlinkPath, status));


	usLink.Buffer = Link;
	usLink.MaximumLength = bMAX_PATH;
	usLink.Length = 0;

	status = ZwQuerySymbolicLinkObject(hLink, &usLink, NULL);
	if (! NT_SUCCESS(status))

		FINISH_ReadSymlinkValue(("ReadSymlinkValue: Unable to query SystemRoot link. status=%x\n", status));


	asLink.Length = 0;
	asLink.MaximumLength = BufferSize;
	asLink.Buffer = Buffer;

	RtlUnicodeStringToAnsiString(&asLink, &usLink, FALSE);
	asLink.Buffer[asLink.Length] = '\0';


	ZwClose(hLink);

	return TRUE;
}



/*
 * RunPostBootup()
 *
 * Description:
 *		Initializes any subsystems that require to be initialized after the system finished booting up.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
InitPostBootup()
{
	ASSERT(BootingUp == FALSE);

	ProcessPostBootup();
	LogPostBootup();
	PolicyPostBootup();
	UserlandPostBootup();
}



/*
 * GetCurrentUserSid()
 *
 * Description:
 *		Retrieves current user's SID.
 *
 * Parameters:
 *		Size - Extra number of bytes to allocate [IN]. Total number of bytes allocated [OUT].
 *
 * Returns:
 *		Pointer to an allocated memory block, the actual SID is at offset +Size.
 *		NULL is failed.
 */

typedef struct _TOKEN_GROUPS {
    DWORD GroupCount;
    SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY];
} TOKEN_GROUPS, *PTOKEN_GROUPS;

PCHAR
GetCurrentUserSid(PUSHORT Size)	// IN OUT
{
	HANDLE					TokenHandle;
	NTSTATUS				status;
	ULONG					TokenSize, tmp;
	PCHAR					UserInfo;
	PSID_AND_ATTRIBUTES		psa;
//	PTOKEN_GROUPS			ptg;
//	PSID					ps;


	status = ZwOpenThreadToken(CURRENT_THREAD, TOKEN_QUERY, FALSE, &TokenHandle);

	if (! NT_SUCCESS(status))
	{
		if (status != STATUS_NO_TOKEN)
		{
			LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%d GetCurrentUserSid: ZwOpenThreadToken failed with status %x\n", status));
			return NULL;
		}

		status = ZwOpenProcessToken(CURRENT_PROCESS, TOKEN_QUERY, &TokenHandle);

		if (! NT_SUCCESS(status))
		{
			LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%d GetCurrentUserSid: ZwOpenProcessToken failed with status %x\n", status));
			return NULL;
		}
	}


	/* first, find out the total amount of token information to be returned */

	status = ZwQueryInformationToken(TokenHandle, TokenUser, &TokenSize, 0, &TokenSize);
//	status = ZwQueryInformationToken(TokenHandle, TokenGroups, &TokenSize, 0, &TokenSize);
//	status = ZwQueryInformationToken(TokenHandle, TokenPrimaryGroup, &TokenSize, 0, &TokenSize);
	if (status != STATUS_BUFFER_TOO_SMALL || TokenSize == 0)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GetCurrentUserSid: ZwQueryInformationToken failed. status=%x size=%d\n", status, TokenSize));
		return NULL;
	}


	/* second, allocate the required amount of memory */
	
	UserInfo = ExAllocatePoolWithTag(NonPagedPool, TokenSize + *Size, _POOL_TAG);
	if (UserInfo == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GetCurrentUserSid: out of memory (requested %d bytes)\n", TokenSize + *Size));
		return NULL;
	}


	psa = (PSID_AND_ATTRIBUTES) (UserInfo + *Size);
//	ptg = (PTOKEN_GROUPS) (UserInfo + *Size);
//	ps = (PSID) (UserInfo + *Size);


	/* third, request the token information */

	status = ZwQueryInformationToken(TokenHandle, TokenUser, psa, TokenSize, &tmp);
//	status = ZwQueryInformationToken(TokenHandle, TokenGroups, psa, TokenSize, &tmp);
//	status = ZwQueryInformationToken(TokenHandle, TokenPrimaryGroup, ps, TokenSize, &tmp);
	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GetCurrentUserSid: ZwQueryInformationToken failed with status %x\n", status));
		ExFreePoolWithTag(UserInfo, _POOL_TAG);
		return NULL;
	}

	ZwClose(TokenHandle);


	/* convert absolute SID into a relative one */
	psa->Sid = (PSID) ( (PCHAR) psa->Sid - (PCHAR) psa );
/*
	for (tmp = 0; tmp < ptg->GroupCount; tmp++)
	{
		ptg->Groups[tmp].Sid = (PSID) ( (PCHAR) ptg->Groups[tmp].Sid - (PCHAR) ptg->Groups );
	}
*/
//	* (PULONG) ps = 4;

	/* return size value */
	*Size += (USHORT) TokenSize;

	return UserInfo;
}



/*
 * NOTE: Adapted from "The VTrace Tool: Building a System Tracer for Windows NT and Windows 2000" -- MSDN Magazine, October 2000
 * NOTE2: This function should only be called on Windows 2000+.
 *		  It's not necessary on Windows NT, and, besides, on Windows NT 4.0 SP3 and earlier,
 *		  MmMapLockedPages() does not give the result we need.
 */

PVOID
ExchangeReadOnlyMemoryPointer(IN OUT PVOID *Target, IN PVOID Value)
{
	PMDL			MDL;
	PVOID			MappedAddress, ReturnValue;


	MDL = IoAllocateMdl(Target, sizeof(Value), FALSE, FALSE, NULL);
	
	if (MDL == NULL)
		return NULL;


//	MmBuildMdlForNonPagedPool(MDL);


	/*
	 * Calls to MmProbeAndLockPages must be enclosed in a try/except block.
	 * If the pages do not support the specified operation, the routine raises the
	 * STATUS_ACCESS_VIOLATION exception.
	 */

	__try
	{
		MmProbeAndLockPages(MDL, KernelMode, IoModifyAccess);
	}

	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("ExchangeReadOnlyMemoryPointer(%x, %x): Caught an exception\n", Target, Value));
		IoFreeMdl(MDL);
		return NULL;
	}


//	MappedAddress = MmMapLockedPages(MDL, KernelMode);
	MappedAddress = MmMapLockedPagesSpecifyCache(MDL, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);

	if (MappedAddress == NULL)
	{
		MmUnlockPages(MDL);
		IoFreeMdl(MDL);
		return NULL;
	}


	ReturnValue = InterlockedExchangePointer(MappedAddress, Value);


	MmUnmapLockedPages(MappedAddress, MDL);
	MmUnlockPages(MDL);
	IoFreeMdl(MDL);


	return ReturnValue;
}
