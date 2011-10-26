#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "types.h"

using std::string;
using std::wstring;
using std::vector;

// Number conversion

rulong str2rul(const string& str, uint base);
rulong hexstr2rul(const string& str);
rulong decstr2rul(const string& str);
double str2dbl(const string &str);

string rul2str(rulong x, uint base, uint fill = 0);
string rul2hexstr(rulong x, uint fill = 0);
string rul2decstr(rulong x, uint fill = 0);
string dbl2str(double x);

// Number manipulation

rulong reverse(rulong dw);
rulong resize(rulong dw, size_t size);

template<typename T> T rol(T val, size_t count)
{
	size_t bitcount = sizeof(T) * 8;
	count %= bitcount;
	return (val << count) | (val >> (bitcount-count));
}

template<typename T> T ror(T val, size_t count)
{
	size_t bitcount = sizeof(T) * 8;
	count %= bitcount;
	return (val >> count) | (val << (bitcount-count));
}

rulong round_up(rulong dw, rulong val);
rulong round_down(rulong dw, rulong val);

// Memory functions
string bytes2hexstr(const byte* bytes, size_t size);
size_t hexstr2bytes(const string& s, byte* bytes, size_t size);

size_t hexstr2bytemask(const string& s, byte* mask, size_t size);
bool memcmp_mask(const byte* b1, const byte* b2, const byte* mask, size_t size);
bool memcpy_mask(byte* b1, const byte* b2, const byte* mask, size_t size);

// Script stuff

bool is_hex(const string& s);
bool is_hexwild(const string& s);
bool is_dec(const string& s);
bool is_float(const string& s);
bool is_bytestring(const string& s);
bool is_string(const string& s);
bool is_memory(const string& s);

// String manipulation

string tolower(const string& in);
string toupper(const string& in);

string reverse(const string& in);

string trim(const string& sData);
bool split(vector<string>& vec, const string& str, char delim);

bool IsQuotedString(const string& s, char cstart, char cend = 0);
string UnquoteString(const string& s, char cstart, char cend = 0);

void ReplaceString(string &s, const char* what, const char* with);
void ReplaceString(wstring &s, const wchar_t* what, const wchar_t* with);
string CleanString(const string& s);

// Chracter conversion
wstring ascii2unicode(const string& s);
string unicode2ascii(const wstring& s);

// File functions
bool isfullpath(const string& path);
bool isfullpath(const wstring& path);
string pathfixup(const string& path, bool isfolder);
wstring pathfixup(const wstring& path, bool isfolder);
string folderfrompath(const string& path);
wstring folderfrompath(const wstring& path);
string filefrompath(const string& path);
wstring filefrompath(const wstring& path);

vector<string> getlines_file(const wchar_t* file);
vector<string> getlines_buff(const char* content, size_t size);

// Misc functions

HWND FindHandle(DWORD dwThreadId, const string& wdwClass, long x, long y);

// microseconds
ulonglong MyTickCount();

bool GetAppVersionString(const char * LibName, const char * Value, string& Output);
//bool GetAppVersion(const char * LibName, WORD * MajorVersion, WORD * MinorVersion, WORD * BuildNumber, WORD * RevisionNumber);
