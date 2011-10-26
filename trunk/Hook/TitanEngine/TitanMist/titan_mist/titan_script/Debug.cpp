#include "Debug.h"

#include <windows.h>
#include "TE_Interface.h"

bool assert_popup(const char* file, unsigned int line, const char* function, const char* condition)
{
char buf[4096];

	wsprintf(buf, "File: %s\r\nLine: %d\r\nFunction: %s\r\nCondition: %s", file, line, function, condition);
	return (IDCANCEL == MessageBox(NULL, buf, "[ASSERT]", MB_ICONEXCLAMATION | MB_OKCANCEL));
}

void DBG_LOG(const string& string)
{
#ifdef _DEBUG
	TE_Log(string.c_str(), TS_LOG_DEBUG);
#endif
}
