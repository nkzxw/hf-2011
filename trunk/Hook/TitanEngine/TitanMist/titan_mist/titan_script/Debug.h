#pragma once

#include <intrin.h>
#include <string>

using std::string;

bool assert_popup(const char* file, unsigned int line, const char* function, const char* condition);

#ifdef _DEBUG
    #define ASSERT(cond) \
		do \
		{ \
			if(!(cond)) \
			{ \
				if(assert_popup(__FILE__, __LINE__, __FUNCTION__, #cond)) \
					__debugbreak(); \
			} \
		} while(0)
#else
	#define ASSERT(cond) \
		do { (void)sizeof(cond); } while(0)
#endif

void DBG_LOG(const string& string);

#define DBG_LOG_VAR(x) DBG_LOG(string(#x) + ": " + x)
