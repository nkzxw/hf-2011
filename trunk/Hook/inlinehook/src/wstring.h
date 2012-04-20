/*
wstring.h

diStorm3 - Powerful disassembler for X86/AMD64
http://ragestorm.net/distorm/
distorm at gmail dot com
Copyright (C) 2011  Gil Dabah

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/


#ifndef WSTRING_H
#define WSTRING_H

#include "config.h"

void strclear_WS(_WString* s);
void chrcat_WS(_WString* s, uint8_t ch);
void strcpylen_WS(_WString* s, const int8_t* buf, unsigned int len);
void strcatlen_WS(_WString* s, const int8_t* buf, unsigned int len);
void strcat_WS(_WString* s, const _WString* s2);

/*
* Warning, this macro should be used only when the compiler knows the size of string in advance!
* This macro is used in order to spare the call to strlen when the strings are known already.
* Note: sizeof includes NULL terminated character.
*/
#define strcat_WSN(s, t) strcatlen_WS((s), ((const int8_t*)t), sizeof((t))-1)
#define strcpy_WSN(s, t) strcpylen_WS((s), ((const int8_t*)t), sizeof((t))-1)

#endif /* WSTRING_H */
