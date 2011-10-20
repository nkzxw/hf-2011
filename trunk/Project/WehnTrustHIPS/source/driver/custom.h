/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_CUSTOM_H
#define _WEHNTRUST_DRIVER_CUSTOM_H

NTSTATUS ExecuteCustomSectionAction(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject OPTIONAL,
		IN PVOID ImageBase,
		IN ULONG ViewSize);

BOOLEAN IsAtiHackRequired();

#endif
