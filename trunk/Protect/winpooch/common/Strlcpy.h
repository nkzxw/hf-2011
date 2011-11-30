#ifndef _STRLCPY_H
#define _STRLCPY_H

#include <string.h>

size_t strlcpy (char *dst, const char *src, size_t size) ;

size_t wcslcpy (wchar_t *dst, const wchar_t *src, size_t size) ;

#ifdef _UNICODE
#define _tcslcpy	wcslcpy
#else
#define _tcslcpy	strlcpy
#endif


#endif
