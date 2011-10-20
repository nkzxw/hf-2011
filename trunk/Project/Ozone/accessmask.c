/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		accessmask.c
 *
 * Abstract:
 *
 *		This module implements various ACCESS_MASK decoding routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 18-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "accessmask.h"



/*
 * Get_FILE_OperationType()
 *
 * Description:
 *		This function decodes file operation types such as GENERIC_READ and DELETE and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_FILE_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;
//	int			FileAll = MAXIMUM_ALLOWED | GENERIC_ALL | STANDARD_RIGHTS_REQUIRED |
//							STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL | ACCESS_SYSTEM_SECURITY; //FILE_ALL_ACCESS


	if ( IS_BIT_SET(DesiredAccess, GENERIC_READ)  ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL)  ||
		 IS_BIT_SET(DesiredAccess, FILE_READ_DATA)  ||
//		 IS_BIT_SET(DesiredAccess, FILE_READ_ACCESS)  ||
//		 IS_BIT_SET(DesiredAccess, FILE_LIST_DIRECTORY)  ||
		 IS_BIT_SET(DesiredAccess, FILE_READ_ATTRIBUTES)  ||
		 IS_BIT_SET(DesiredAccess, FILE_READ_EA)  ||
		 IS_BIT_SET(DesiredAccess, SYNCHRONIZE) ||
		 IS_BIT_SET(DesiredAccess, STANDARD_RIGHTS_READ) ||
		 IS_BIT_SET(DesiredAccess, FILE_ALL_ACCESS) ||
		 DesiredAccess == 0)

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, GENERIC_WRITE)  ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL)  ||
		 IS_BIT_SET(DesiredAccess, FILE_WRITE_DATA)  ||
//		 IS_BIT_SET(DesiredAccess, FILE_WRITE_ACCESS)  ||
//		 IS_BIT_SET(DesiredAccess, FILE_ADD_FILE)  ||
		 IS_BIT_SET(DesiredAccess, FILE_WRITE_ATTRIBUTES)  ||
		 IS_BIT_SET(DesiredAccess, FILE_WRITE_EA)  ||
		 IS_BIT_SET(DesiredAccess, FILE_APPEND_DATA)  ||
//		 IS_BIT_SET(DesiredAccess, FILE_ADD_SUBDIRECTORY)  ||
//		 IS_BIT_SET(DesiredAccess, FILE_CREATE_PIPE_INSTANCE)  ||
		 IS_BIT_SET(DesiredAccess, FILE_DELETE_CHILD)  ||
		 IS_BIT_SET(DesiredAccess, DELETE)  || //XXX it's own category now?
		 IS_BIT_SET(DesiredAccess, WRITE_DAC)  ||
		 IS_BIT_SET(DesiredAccess, WRITE_OWNER)  ||
		 IS_BIT_SET(DesiredAccess, FILE_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if ( IS_BIT_SET(DesiredAccess, GENERIC_EXECUTE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL)  ||
		 IS_BIT_SET(DesiredAccess, FILE_EXECUTE) ||
//		 IS_BIT_SET(DesiredAccess, FILE_TRAVERSE) ||
		 IS_BIT_SET(DesiredAccess, FILE_ALL_ACCESS) )

		 OperationType |= OP_EXECUTE;

	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_FILE_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * DecodeFileOperationType()
 *
 * Description:
 *		This function decodes file operation types such as GENERIC_READ and DELETE and prints them out (for debugging)
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		Nothing.
 */

void
DecodeFileOperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;
	int			FileAll = MAXIMUM_ALLOWED | GENERIC_ALL | STANDARD_RIGHTS_REQUIRED |
							STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL | ACCESS_SYSTEM_SECURITY;


	if ( (DesiredAccess & GENERIC_READ) ==  GENERIC_READ)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GENERIC_READ %x\n", GENERIC_READ));

	if ( (DesiredAccess & GENERIC_WRITE) ==  GENERIC_WRITE)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GENERIC_WRITE %x\n", GENERIC_WRITE));

	if ( (DesiredAccess & GENERIC_EXECUTE) ==  GENERIC_EXECUTE)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GENERIC_EXECUTE %x\n", GENERIC_EXECUTE));

	if ( (DesiredAccess & STANDARD_RIGHTS_READ) ==  STANDARD_RIGHTS_READ)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("STANDARD_RIGHTS_READ %x\n", STANDARD_RIGHTS_READ));

	if ( (DesiredAccess & SYNCHRONIZE) ==  SYNCHRONIZE)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("SYNCHRONIZE %x\n", SYNCHRONIZE));

	if ( (DesiredAccess & MAXIMUM_ALLOWED) ==  MAXIMUM_ALLOWED)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("MAXIMUM_ALLOWED %x\n", MAXIMUM_ALLOWED));

	if ( (DesiredAccess & GENERIC_ALL) ==  GENERIC_ALL)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GENERIC_ALL %x\n", GENERIC_ALL));

	if ( (DesiredAccess & STANDARD_RIGHTS_REQUIRED) ==  STANDARD_RIGHTS_REQUIRED)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("STANDARD_RIGHTS_REQUIRED %x\n", STANDARD_RIGHTS_REQUIRED));

	if ( (DesiredAccess & STANDARD_RIGHTS_ALL) ==  STANDARD_RIGHTS_ALL)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("STANDARD_RIGHTS_ALL %x\n", STANDARD_RIGHTS_ALL));

	if ( (DesiredAccess & SPECIFIC_RIGHTS_ALL) ==  SPECIFIC_RIGHTS_ALL)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("SPECIFIC_RIGHTS_ALL %x\n", SPECIFIC_RIGHTS_ALL));

	if ( (DesiredAccess & ACCESS_SYSTEM_SECURITY) ==  ACCESS_SYSTEM_SECURITY)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("ACCESS_SYSTEM_SECURITY %x\n", ACCESS_SYSTEM_SECURITY));

	if ( (DesiredAccess & WRITE_OWNER) ==  WRITE_OWNER)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("WRITE_OWNER %x\n", WRITE_OWNER));

	if ( (DesiredAccess & WRITE_DAC) ==  WRITE_DAC)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("WRITE_DAC %x\n", WRITE_DAC));

	if ( (DesiredAccess & DELETE) ==  DELETE)
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("DELETE %x\n", DELETE));


	if ( IS_BIT_SET(DesiredAccess, GENERIC_READ) ||
		 IS_BIT_SET(DesiredAccess, (STANDARD_RIGHTS_READ | SYNCHRONIZE )) ||
		 IS_BIT_SET(DesiredAccess, FileAll) )

		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("OP_READ\n"));


	if ( IS_BIT_SET(DesiredAccess, GENERIC_WRITE) ||
		 IS_BIT_SET(DesiredAccess, (DELETE | WRITE_DAC | WRITE_OWNER)) ||
		 IS_BIT_SET(DesiredAccess, FileAll) )

		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("OP_WRITE\n"));


	if ( IS_BIT_SET(DesiredAccess, GENERIC_EXECUTE) ||
		 IS_BIT_SET(DesiredAccess, FileAll) )

		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("OP_EXECUTE\n"));
}



/*
 * Get_NAMEDPIPE_OperationType()
 *
 * Description:
 *		This function decodes named pipe operation types such as GENERIC_READ and DELETE and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_NAMEDPIPE_OperationType(ACCESS_MASK DesiredAccess)
{
	return  Get_FILE_OperationType(DesiredAccess);
}



/*
 * Get_MAILSLOT_OperationType()
 *
 * Description:
 *		This function decodes mailslot operation types such as GENERIC_READ and DELETE and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_MAILSLOT_OperationType(ACCESS_MASK DesiredAccess)
{
	return Get_FILE_OperationType(DesiredAccess);
}



/*
 * Get_REGISTRY_OperationType()
 *
 * Description:
 *		This function decodes registry operation types such as KEY_QUERY_VALUE and DELETE and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_REGISTRY_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, GENERIC_READ) ||
		 IS_BIT_SET(DesiredAccess, KEY_QUERY_VALUE) ||
		 IS_BIT_SET(DesiredAccess, KEY_ENUMERATE_SUB_KEYS) ||
		 IS_BIT_SET(DesiredAccess, MAXIMUM_ALLOWED) ||
		 IS_BIT_SET(DesiredAccess, SYNCHRONIZE) ||
		 IS_BIT_SET(DesiredAccess, ACCESS_SYSTEM_SECURITY) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) ||
		 IS_BIT_SET(DesiredAccess, KEY_ALL_ACCESS) ||
		 DesiredAccess == 0)

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, GENERIC_WRITE) ||
		 IS_BIT_SET(DesiredAccess, KEY_SET_VALUE) ||
		 IS_BIT_SET(DesiredAccess, KEY_CREATE_SUB_KEY) ||
		 IS_BIT_SET(DesiredAccess, KEY_CREATE_LINK) ||
		 IS_BIT_SET(DesiredAccess, WRITE_OWNER) ||
		 IS_BIT_SET(DesiredAccess, WRITE_DAC) ||
		 IS_BIT_SET(DesiredAccess, DELETE) ||
		 IS_BIT_SET(DesiredAccess, MAXIMUM_ALLOWED) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) ||
		 IS_BIT_SET(DesiredAccess, KEY_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if ( IS_BIT_SET(DesiredAccess, GENERIC_EXECUTE) ||
		 IS_BIT_SET(DesiredAccess, KEY_NOTIFY) ||
		 IS_BIT_SET(DesiredAccess, MAXIMUM_ALLOWED) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) ||
		 IS_BIT_SET(DesiredAccess, KEY_ALL_ACCESS) )

		 OperationType |= OP_EXECUTE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_REGISTRY_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_EVENT_OperationType()
 *
 * Description:
 *		This function decodes event operation types such as EVENT_QUERY_STATE and GENERIC_WRITE and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_EVENT_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, EVENT_QUERY_STATE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_READ) ||
		 IS_BIT_SET(DesiredAccess, STANDARD_RIGHTS_READ) ||
		 IS_BIT_SET(DesiredAccess, SYNCHRONIZE) ||
		 IS_BIT_SET(DesiredAccess, EVENT_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, EVENT_MODIFY_STATE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_WRITE) ||
		 IS_BIT_SET(DesiredAccess, WRITE_DAC) ||
		 IS_BIT_SET(DesiredAccess, WRITE_OWNER) ||
		 IS_BIT_SET(DesiredAccess, EVENT_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_EVENT_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_SEMAPHORE_OperationType()
 *
 * Description:
 *		This function decodes semaphore operation types such as SEMAPHORE_QUERY_STATE and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_SEMAPHORE_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, SEMAPHORE_QUERY_STATE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_READ) ||
		 IS_BIT_SET(DesiredAccess, READ_CONTROL) ||
		 IS_BIT_SET(DesiredAccess, SEMAPHORE_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, SEMAPHORE_MODIFY_STATE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_WRITE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) ||
		 IS_BIT_SET(DesiredAccess, WRITE_DAC) ||
		 IS_BIT_SET(DesiredAccess, SEMAPHORE_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_SEMAPHORE_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_SECTION_OperationType()
 *
 * Description:
 *		This function decodes section operation types such as SECTION_QUERY and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_SECTION_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, SECTION_QUERY) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_READ) ||
		 IS_BIT_SET(DesiredAccess, READ_CONTROL) ||
		 IS_BIT_SET(DesiredAccess, SECTION_MAP_READ) ||
		 IS_BIT_SET(DesiredAccess, SECTION_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, SECTION_EXTEND_SIZE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_WRITE) ||
		 IS_BIT_SET(DesiredAccess, WRITE_DAC) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) ||
		 IS_BIT_SET(DesiredAccess, SECTION_MAP_WRITE) ||
		 IS_BIT_SET(DesiredAccess, SECTION_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if ( IS_BIT_SET(DesiredAccess, SECTION_MAP_EXECUTE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) ||
		 IS_BIT_SET(DesiredAccess, SECTION_ALL_ACCESS) )

		 OperationType |= OP_EXECUTE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_SECTION_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_JOB_OperationType()
 *
 * Description:
 *		This function decodes job object operation types such as JOB_OBJECT_QUERY and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_JOB_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, JOB_OBJECT_QUERY) ||
		 IS_BIT_SET(DesiredAccess, JOB_OBJECT_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, JOB_OBJECT_ASSIGN_PROCESS) ||
		 IS_BIT_SET(DesiredAccess, JOB_OBJECT_SET_ATTRIBUTES) ||
		 IS_BIT_SET(DesiredAccess, JOB_OBJECT_TERMINATE) ||
		 IS_BIT_SET(DesiredAccess, JOB_OBJECT_SET_SECURITY_ATTRIBUTES) ||
		 IS_BIT_SET(DesiredAccess, JOB_OBJECT_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_JOB_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_MUTANT_OperationType()
 *
 * Description:
 *		This function decodes mutant operation types such as JOB_OBJECT_QUERY and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_MUTANT_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, MUTANT_QUERY_STATE) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_READ) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) ||
		 IS_BIT_SET(DesiredAccess, READ_CONTROL) ||
		 IS_BIT_SET(DesiredAccess, SYNCHRONIZE) ||
		 IS_BIT_SET(DesiredAccess, MUTANT_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, MUTANT_ALL_ACCESS) ||
		 IS_BIT_SET(DesiredAccess, WRITE_OWNER) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_WRITE) ||
		 IS_BIT_SET(DesiredAccess, WRITE_DAC) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_ALL) )

		OperationType |= OP_WRITE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_MUTANT_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_SYMLINK_OperationType()
 *
 * Description:
 *		This function decodes symbolic link operation types such as SYMBOLIC_LINK_QUERY and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_SYMLINK_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, SYMBOLIC_LINK_QUERY) ||
		 IS_BIT_SET(DesiredAccess, GENERIC_READ) ||
		 IS_BIT_SET(DesiredAccess, SYNCHRONIZE) ||
		 IS_BIT_SET(DesiredAccess, SYMBOLIC_LINK_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, DELETE) ||
		 IS_BIT_SET(DesiredAccess, MAXIMUM_ALLOWED) ||
		 IS_BIT_SET(DesiredAccess, SYMBOLIC_LINK_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_SYMLINK_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_TIMER_OperationType()
 *
 * Description:
 *		This function decodes timer operation types such as JOB_OBJECT_QUERY and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_TIMER_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, TIMER_QUERY_STATE) ||
		 IS_BIT_SET(DesiredAccess, TIMER_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, TIMER_MODIFY_STATE) ||
		 IS_BIT_SET(DesiredAccess, TIMER_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_TIMER_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



/*
 * Get_PORT_OperationType()
 *
 * Description:
 *		This function decodes port operation types and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		OP_WRITE.
 */

UCHAR
Get_PORT_OperationType(ACCESS_MASK DesiredAccess)
{
	return OP_WRITE;
}



/*
 * Get_DIROBJ_OperationType()
 *
 * Description:
 *		This function decodes directory operation types such as DIRECTORY_QUERY and converts them to
 *		3 internal operations: OP_READ, OP_WRITE and OP_EXECUTE.
 *
 * Parameters:
 *		DesiredAccess - ACCESS_MASK structure (a doubleword value containing standard, specific, and generic rights).
 *
 * Returns:
 *		A combination of OP_READ, OP_WRITE & OP_EXECUTE flags set depending on the DesiredAccess argument.
 */

UCHAR
Get_DIROBJ_OperationType(ACCESS_MASK DesiredAccess)
{
	UCHAR		OperationType = 0;


	if ( IS_BIT_SET(DesiredAccess, DIRECTORY_QUERY) ||
		 IS_BIT_SET(DesiredAccess, DIRECTORY_TRAVERSE) ||
		 IS_BIT_SET(DesiredAccess, DIRECTORY_ALL_ACCESS) )

		OperationType |= OP_READ;


	if ( IS_BIT_SET(DesiredAccess, DIRECTORY_CREATE_OBJECT) ||
		 IS_BIT_SET(DesiredAccess, DIRECTORY_CREATE_SUBDIRECTORY) ||
		 IS_BIT_SET(DesiredAccess, DIRECTORY_ALL_ACCESS) )

		OperationType |= OP_WRITE;


	if (OperationType == 0)
//		OperationType = OP_READ | OP_WRITE | OP_EXECUTE;
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Get_DIROBJ_OperationType: Unknown desired access mask %x\n", DesiredAccess));


	return OperationType;
}



