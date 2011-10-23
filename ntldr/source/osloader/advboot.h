//********************************************************************
//	created:	18:8:2008   20:08
//	file:		advboot.h
//	author:		tiamo
//	purpose:	advanced boot
//********************************************************************

#pragma once

//
// get advanced boot option
//
ULONG BlGetAdvancedBootOption();

//
// do load processing
//
VOID BlDoAdvancedBootLoadProcessing(__in ULONG BootIndex);

//
// get boot load options
//
PCHAR BlGetAdvancedBootLoadOptions(__in ULONG BootIndex);

//
// get boot id
//
ULONG BlGetAdvancedBootID(__in ULONG BootIndex);

//
// get display string
//
PCHAR BlGetAdvancedBootDisplayString(__in ULONG BootIndex);

//
// auto advanced boot
//
VOID BlAutoAdvancedBoot(__inout PCHAR* LoaderOptions,__in ULONG LastBootStatus,__in ULONG DefaultAdvancedBootIndex);

//
// show advanced boot menu
//
ULONG BlDoAdvancedBoot(__in ULONG TitleMessageId,__in ULONG DefaultSelectedMenuIndex,__in ULONG Arg8,__in ULONG Timeout);