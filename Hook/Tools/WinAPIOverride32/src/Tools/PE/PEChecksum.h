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
*/

//-----------------------------------------------------------------------------
// Object: PE helper
//-----------------------------------------------------------------------------

#pragma once
#include "PE.h"
#include <Imagehlp.h>

// Imagehlp lib is requiered
#pragma comment(lib,"Imagehlp")


// this class is just a begin of pe parsing we just interest us to the 3 first struct of an exe file
// that means IMAGE_DOS_HEADER IMAGE_NT_HEADERS PIMAGE_SECTION_HEADER

class CPEChecksum:public CPE
{
public:
    CPEChecksum(TCHAR* filename):CPE(filename){};
    ~CPEChecksum(void){};
    BOOL GetChecksum(DWORD* pCurrentHeaderCheckSum,DWORD* pRealCheckSum);
    BOOL CorrectChecksum();
};
