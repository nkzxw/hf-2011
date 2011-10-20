//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: CAT_EXPLOITATION
//
// MessageText:
//
//  Exploitation
//
#define CAT_EXPLOITATION                 ((ULONG)0x00000001L)

//
// MessageId: MSG_EXP_SEH_OVERWRITE
//
// MessageText:
//
//  WehnTrust has prevented the exploitation of an SEH overflow.
//  %1
//  %2
//
#define MSG_EXP_SEH_OVERWRITE            ((ULONG)0xC0000002L)

//
// MessageId: MSG_EXP_STACK_OVERFLOW
//
// MessageText:
//
//  WehnTrust has prevented the exploitation of a stack overflow.
//  %1
//  %2
//
#define MSG_EXP_STACK_OVERFLOW           ((ULONG)0xC0000003L)

//
// MessageId: MSG_EXP_FORMAT_STRING
//
// MessageText:
//
//  WehnTrust has prevented the exploitation of a format string bug.
//  %1
//  %2
//
#define MSG_EXP_FORMAT_STRING            ((ULONG)0xC0000004L)

