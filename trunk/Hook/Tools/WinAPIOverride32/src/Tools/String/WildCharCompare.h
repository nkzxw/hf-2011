/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


THANKS TO OPEN SOURCE COMMUNITY
This code is based on an open source algo, but
as more than once claiming to be author of this, 
I don't know the person to thanks :( 
--> please let original authors names !!!
*/

//-----------------------------------------------------------------------------
// Object: Check if a string match a pattern containing '*' and '?'
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <malloc.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CWildCharCompare
{
public:
    static BOOL WildCmp(TCHAR* Wild,TCHAR* String);
    static BOOL WildICmp(TCHAR* Wild,TCHAR* String);
};
