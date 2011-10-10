
#include <windows.h>
#include <stdio.h>
#include "../DiaSDK/include/cvconst.h"

typedef struct MapIa64Reg{
    CV_HREG_e  iCvReg;
    const wchar_t* wszRegName;
}MapIa64Reg;

int cmpIa64regSz( const void* , const void* );
const wchar_t* SzNameC7Reg( USHORT , DWORD );
