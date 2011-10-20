/*
 *
 * $Id: log.h,v 1.1 2003/04/12 12:53:09 yjchen Exp $
 *
 * Revision 1.1  2002/09/13 12:53:09  yjchen
 *   1. Message header file for user mode application and kernel
 *      driver communication.
 *
 *
 */
#ifndef __LOG_H__
#define __LOG_H__

//
// Specify Structure Packing
//
#pragma pack(push, 1)

#if defined(_WINDOWS) || defined (_CONSOLE) 
typedef struct _TIME_FIELDS 
{
    USHORT Year;        // range [1601...]
    USHORT Month;       // range [1..12]
    USHORT Day;         // range [1..31]
    USHORT Hour;        // range [0..23]
    USHORT Minute;      // range [0..59]
    USHORT Second;      // range [0..59]
    USHORT Milliseconds;// range [0..999]
    USHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
#endif // _WINDOWS

typedef struct _LOG_ITEM 
{
    TIME_FIELDS Now;

    UINT        iProto;

    ULONG       ulSrcAddress;    
    ULONG       ulDestAddress;
    USHORT      usSrcPort;
    USHORT      usDestPort;  
    
	INT         iSize;

    UCHAR	    ucDirection;    
    UCHAR	    ucAction;

} LOG_ITEM, *PLOG_ITEM;

//
// Restore Default Structure Packing
//
#pragma pack(pop)

#endif	/* __LOG_H__	*/


