#pragma once

#include "HelperFunctions.h"

#include <algorithm>
#include <cstdlib>
#include "globals.h"

#include "Debug.h"

using std::string;
using std::reverse;
using std::min;
using std::max;

// Number conversion

const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

rulong char2rul(char c)
{
	c = toupper(c);
	for(int i = 0; digits[i]; i++)
	{
		if(c == digits[i])
			return i;
	}
	return -1;
}

rulong str2rul(const string& str, uint base)
{
rulong num = 0;
rulong charnum;

	if(base < 2 || base > 32)
		return 0;

	for(size_t i = 0; i < str.length(); i++)
	{
		charnum = char2rul(str[i]);
		if(charnum < 0 || charnum >= base)
			break;

		num *= base;
		num += charnum;
	}
	return num;

/*
#ifdef _WIN64
	return _strtoui64(string.c_str(), NULL, Base);
#else
	return strtoul(string.c_str(), NULL, Base);
#endif
*/
}

rulong hexstr2rul(const string& string)
{
	return str2rul(string, 16);
}

rulong decstr2rul(const string& string)
{
	return str2rul(string, 10);
}

double str2dbl(const string &s)
{
	return strtod(s.c_str(), NULL);
}

string rul2str(rulong x, uint base, uint fill)
{
string out;
uint i = 0;

	if(base < 2 || base > 32)
		return "";

	do
	{
		out.insert(0, 1, digits[x % base]);
		x /= base;
		i++;
	}
	while(x || i < fill);

	return out;

	/*
#ifdef _WIN64
	_ui64toa(x, buffer, Base);
#else
	_ultoa(x, buffer, Base);
#endif
	string out = buffer;
	if(fill > out.size())
		out.insert(0, fill-out.size(), '0');
	return out;
	*/
}

string rul2hexstr(rulong x, uint fill)
{
	return rul2str(x, 16, fill);
}

string rul2decstr(rulong x, uint fill)
{
	return rul2str(x, 10, fill);
}

string dbl2str(double x)
{
char buffer[400]; //??

	sprintf(buffer, "%f", x);
	//_gcvt(x, _countof(buffer)-3, buffer);
	return buffer;
}

// Number manipulation

rulong reverse(rulong dw)
{
	byte * pdw = (byte *)&dw;
	reverse(pdw, pdw+sizeof(dw));
	return dw;
}

rulong resize(rulong dw, size_t size)
{
	if(size > 0 && size < sizeof(dw))
	{
		dw &= ((1 << (size*8)) - 1);
	}
	return dw;
}

rulong round_up(rulong dw, rulong val)
{
	rulong mod = dw % val;
	if(mod)
		dw += (val - mod);
	return dw;
}

rulong round_down(rulong dw, rulong val)
{
	return (dw - (dw % val));
}

// Memory functions

string bytes2hexstr(const byte* bytes, size_t size)
{
string out;

	out.reserve(size*2);
	for(size_t i = 0; i < size; i++)
	{
		out += rul2hexstr(bytes[i], 2);
	}
	return out;
}

size_t hexstr2bytes(const string &s, byte* bytes, size_t size)
{
	size = min(size, s.length()/2);
	memset(bytes, 0, size);
	for(size_t i = 0; i < size; i++)
	{
		string sub = s.substr(i * 2, 2);
		if(sub[0] == '?') sub[0] = '0';
		if(sub[1] == '?') sub[1] = '0';
		bytes[i] = hexstr2rul(sub);
	}
	return size;
}

size_t hexstr2bytemask(const string& s, byte* mask, size_t size)
{
	size = min(size, s.length()/2);
	memset(mask, 0, size);
	for(size_t i = 0, e = s.length(); i < e; i++)
	{
		if(s[i] != '?')
			mask[i/2] |= (0xF0 >> ((i%2)*4));
	}
	return size;
}

bool memcmp_mask(const byte* b1, const byte* b2, const byte* mask, size_t size)
{
	for(size_t i = 0; i < size; i++)
	{
		if((b1[i] & mask[i]) != (b2[i] & mask[i]))
			return false;
	}
	return true;
}

bool memcpy_mask(byte* b1, const byte* b2, const byte* mask, size_t size)
{
	for(size_t i = 0; i < size; i++)
	{
		b1[i] = (b1[i] & ~mask[i]) | (b2[i] & mask[i]);
	}
	return true;
}

// Script stuff

bool is_hex(const string& s)
{
    return (s.length() && s.find_first_not_of("0123456789abcdefABCDEF") == string::npos); // '-' ?
}

bool is_hexwild(const string& s)
{
	return (s.length() && s.find_first_not_of("?0123456789abcdefABCDEF") == string::npos); // '-' ?
}

bool is_dec(const string &s) 
{
	size_t len = s.length();
	return (len >= 2 && s[len-1] == '.' && s.find_first_not_of("0123456789") == len-1);
}

bool is_float(const string &s) 
{
	size_t p = s.find('.');
	if(p == string::npos || p+1 == s.length())
		return false;

	return (s.substr(0, p).find_first_not_of("0123456789") == string::npos &&
	        s.substr(p+1, string::npos).find_first_not_of("0123456789") == string::npos);
}

bool is_bytestring(const string& s)
{
	size_t len = s.length();
	return (len >= 2 && !(len % 2) && s[0] == '#' && s[len-1] == '#' && is_hexwild(s.substr(1, len-2)));
}

bool is_string(const string& s)
{
	size_t len = s.length();
	return (len > 2 && s[0] == '"' && s.find('"', 1) == len-1);
}

bool is_memory(const string& s)
{
	size_t len = s.length();
	return (len > 2 && s[0] == '[' && s[len-1] == ']');
}

// String manipulation

string tolower(const string& in)
{
string out;

	size_t size = in.length();
	out.reserve(size);
	for(size_t i = 0; i < size; i++)
		out += tolower(in[i]);
	return out;
}

string toupper(const string& in)
{
string out;

	size_t size = in.length();
	out.reserve(size);
	for(size_t i = 0; i < size; i++)
		out += toupper(in[i]);
	return out;
}

string reverse(const string& in)
{
string out = in;

	reverse(out.begin(), out.end());
	return out;
}

string trim(const string& s) // Thanks to A. Focht for this one
{
size_t left, right;

	if((left = s.find_first_not_of(whitespaces)) != string::npos) 
	{
		right = s.find_last_not_of(whitespaces);
		return s.substr(left, right-left+1);
	}
	return "";
}

bool split(vector<string>& vec, const string& str, char delim)  
{
vector<size_t> pos;
bool inQuotes = false;

	for(size_t i = 0; i < str.size(); i++)
	{
		if(str[i] == '"')
			inQuotes = !inQuotes;

		if(str[i] == delim && !inQuotes)
			pos.push_back(i);
	}

	size_t start = 0, end;

	for(size_t i = 0; i < pos.size(); i++)
	{
		end = pos[i];
		vec.push_back(trim(str.substr(start, end - start)));
		start = end + 1;
	}

	if(start < str.size())
		vec.push_back(trim(str.substr(start)));

	return true;
}

bool IsQuotedString(const string& s, char cstart, char cend)
{
	if(!cend)
		cend = cstart;

	return (s.length() >= 2 && s[0] == cstart && s[s.length()-1] == cend);
}

string UnquoteString(const string &s, char cstart, char cend)
{
string result;

	if(!cend)
		cend = cstart;

	if(!IsQuotedString(s, cstart, cend))
		return s;

	result = s.substr(1, s.length()-2);
	if(cstart == '"') 
		ReplaceString(result, "\\r\\n", "\r\n");
	return result;
}

void ReplaceString(string &s, const char* what, const char* with)
{
size_t p = 0;
size_t la = strlen(what);
size_t li = strlen(with);

	while((p = s.find(what, p)) != string::npos) 
	{
		s.replace(p, la, with, li);
		p += li;
	}
}

void ReplaceString(wstring &s, const wchar_t* what, const wchar_t* with)
{
size_t p = 0;
size_t la = wcslen(what);
size_t li = wcslen(with);

	while((p = s.find(what, p)) != wstring::npos) 
	{
		s.replace(p, la, with, li);
		p += li;
	}
}

//Remove characters in string for display
string CleanString(const string& s)
{
string str = s;

	for(size_t p = 0; p < s.length(); p++)
	{
		if(!str[p])
			str[p] = ' ';
	}
	return str;
}

// Chracter conversion

wstring ascii2unicode(const string& s)
{
wstring result;

	size_t len = s.length();
	wchar_t* buf = new wchar_t[len+1];
	int written = MultiByteToWideChar(CP_ACP, 0, s.c_str(), len, buf, len);
	buf[written] = L'\0';
	result = buf;
	delete[] buf;
	return result;
}

string unicode2ascii(const wstring& s)
{
string result;

	size_t len = s.length();
	char* buf = new char[len+1];
	int written = WideCharToMultiByte(CP_ACP, 0, s.c_str(), len, buf, len, NULL, NULL);
	buf[written] = '\0';
	result = buf;
	delete[] buf;
	return result;
}

// File functions

bool isfullpath(const string& path)
{
	return (path.find(":\\") != string::npos);
}

bool isfullpath(const wstring& path)
{
	return (path.find(L":\\") != wstring::npos);
}

string pathfixup(const string& path, bool isfolder)
{
string out = path;

	if(out.length())
	{
		ReplaceString(out, "/", "\\");
		if(isfolder && *out.rbegin() != '\\')
			out += '\\';
	}
	return out;
}

wstring pathfixup(const wstring& path, bool isfolder)
{
wstring out = path;

	if(out.length())
	{
		ReplaceString(out, L"/", L"\\");
		if(isfolder && *out.rbegin() != L'\\')
			out += L'\\';
	}
	return out;
}

string folderfrompath(const string& path)
{
	size_t p = path.rfind('\\');

	if(p == string::npos || p == path.length()-1)
		return path;
	else
		return path.substr(0, p+1);
}

wstring folderfrompath(const wstring& path)
{
	size_t p = path.rfind(L'\\');

	if(p == wstring::npos || p == path.length()-1)
		return path;
	else
		return path.substr(0, p+1);
}

string filefrompath(const string& path)
{
string out = path;
size_t p;

	if((p = out.rfind('\\')) != string::npos)
		out.erase(0, p+1);

	return out;
}

wstring filefrompath(const wstring& path)
{
wstring out = path;
size_t p;

	if((p = out.rfind(L'\\')) != wstring::npos)
		out.erase(0, p+1);

	return out;
}

vector<string> getlines_file(const wchar_t* file)
{
vector<string> script;

	HANDLE hFile = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(hFile, NULL);
		char* buffer = new char[size];
		DWORD bytes;

		if(ReadFile(hFile, buffer, size, &bytes, NULL))
		{
			script = getlines_buff(buffer, size);
		}
		CloseHandle(hFile);
	}

	return script;
}

vector<string> getlines_buff(const char* content, size_t size)
{
vector<string> script;

	for(size_t i = 0; i < size; i++)
	{
		for(size_t j = i; j < size; j++)
		{
			if(content[j] == '\r' || content[j] == '\n' || content[j] == '\0')
			{
				script.push_back(string(&content[i], j-i));
				i = j;
				break;
			}
			else if(j == size-1) // last line without break
			{
				script.push_back(string(&content[i], j-i+1));
				i = j;
				break;
			}
		}
	}

	return script;
}

// Misc functions

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam) 
{
	*(HWND*)lParam = hwnd;
	return !IsWindowVisible(hwnd);
}

HWND FindHandle(DWORD dwThreadId, const string& wdwClass, long x, long y) 
{
char buffer[256] = {0};
HWND handle, desktop, parent;

	EnumThreadWindows(dwThreadId, &EnumThreadWndProc, (LPARAM)&parent);

	desktop = GetDesktopWindow();

	do
	{
		handle = parent;
		parent = GetParent(handle);
	}
	while(parent && parent != desktop);

	POINT pt = { x, y };

	handle = ChildWindowFromPoint(handle, pt);
	if(handle)
	{
 		GetClassName(handle, buffer, _countof(buffer)); 
		if(!_strnicmp(buffer, wdwClass.c_str(), wdwClass.length()))
			return handle;
	}	
	return 0;
}

ulonglong MyTickCount()
{
	LARGE_INTEGER PerformanceCount = {0}, Frequency = {0};
	ulonglong result;

	if(!QueryPerformanceFrequency(&Frequency) || !Frequency.QuadPart || !QueryPerformanceCounter(&PerformanceCount))
	{
		// millseconds * 1,000 = microseconds
		result = GetTickCount() * 1000;
	}
	else
	{
		// ticks * 1,000,000 / ticks per second = seconds * 1,000,000 = microseconds
		result = (PerformanceCount.QuadPart * 1000000) / Frequency.QuadPart;
	}
	return result;
}

bool GetAppVersionString(const char * LibName, const char * Value, string& Output)
{
DWORD dwHandle, dwLen;
UINT BufLen;
BYTE * lpData;
DWORD * lpTranslate;
const char * lpPropertyBuffer;

	dwLen = GetFileVersionInfoSize(LibName, &dwHandle);
	if(!dwLen) 
		return false;

	lpData = new BYTE[dwLen];

	if(GetFileVersionInfo(LibName, dwHandle, dwLen, lpData))
	{
		// Read the list of languages and code pages.
		if(VerQueryValue(lpData, "\\VarFileInfo\\Translation", (void **)&lpTranslate, &BufLen))
		{
			// Use first language
			string ValueString = "\\StringFileInfo\\" + rul2hexstr(LOWORD(*lpTranslate), 4) + rul2hexstr(HIWORD(*lpTranslate), 4) + "\\" + Value;
			if(VerQueryValue(lpData, ValueString.c_str(), (void **)&lpPropertyBuffer, &BufLen))
			{
				Output = lpPropertyBuffer;
				delete[] lpData;
				return true;
			}
		}
	}

	delete[] lpData;
	return false;
}

/*
bool GetAppVersion(const char * LibName, WORD * MajorVersion, WORD * MinorVersion, WORD * BuildNumber, WORD * RevisionNumber)
{
DWORD dwHandle, dwLen;
UINT BufLen;
BYTE * lpData;
VS_FIXEDFILEINFO * pFileInfo;

	dwLen = GetFileVersionInfoSize(LibName, &dwHandle);
	if(!dwLen) 
		return false;

	lpData = new BYTE[dwLen];

	if(GetFileVersionInfo(LibName, dwHandle, dwLen, lpData))
	{
		if(VerQueryValue(lpData, "\\", (void**)&pFileInfo, &BufLen)) 
		{
			*MajorVersion   = HIWORD(pFileInfo->dwFileVersionMS);
			*MinorVersion   = LOWORD(pFileInfo->dwFileVersionMS);
			*BuildNumber    = HIWORD(pFileInfo->dwFileVersionLS);
			*RevisionNumber = LOWORD(pFileInfo->dwFileVersionLS);
			delete[] lpData;
			return true;
		}
	}

	delete[] lpData;
	return false;
}
*/
