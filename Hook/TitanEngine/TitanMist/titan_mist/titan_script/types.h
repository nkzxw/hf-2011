#pragma once

#include <windows.h>

typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;

typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#ifdef _WIN64
typedef  int64_t  intmax_t;
typedef uint64_t uintmax_t;
#else
typedef  int32_t  intmax_t;
typedef uint32_t uintmax_t;
#endif

typedef uint8_t   byte;
typedef uint16_t  word;
typedef uint32_t dword;
typedef uint64_t qword;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

#ifdef _WIN64
typedef ulonglong rulong;
//typedef long long rint;
#else
typedef ulong rulong;
//typedef long rint;
#endif
