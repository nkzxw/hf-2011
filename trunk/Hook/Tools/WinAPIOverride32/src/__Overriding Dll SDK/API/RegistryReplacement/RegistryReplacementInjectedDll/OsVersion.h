#pragma once

#include <Windows.h>


// Windows 7 6.1 
// Windows Server 2008 R2 6.1 
// Windows Server 2008 6.0 
// Windows Vista 6.0 
// Windows Server 2003 R2 5.2 
// Windows Server 2003 5.2 
// Windows XP 5.1 
// Windows 2000 5.0 
class COsVersion
{
public:
    DWORD dwVersion; 
    DWORD dwMajorVersion;
    DWORD dwMinorVersion; 
    DWORD dwBuild;
    BOOL IsXpOrNewer(){return ( (this->dwMajorVersion>5) || ( (this->dwMajorVersion==5) && (this->dwMinorVersion>=1) ) );}
    BOOL IsVistaOrNewer(){return (this->dwMajorVersion>=6);}
    BOOL IsSevenOrNewer(){return ( (this->dwMajorVersion>6) || ( (this->dwMajorVersion==6) && (this->dwMinorVersion>=1) ) );}

    BOOL IsXp(){return ( (this->dwMajorVersion==5) && (this->dwMinorVersion==1) );}
    BOOL IsVista(){return ( (this->dwMajorVersion==6)  && (this->dwMinorVersion==0) ) ;}
    BOOL IsSeven(){return ( (this->dwMajorVersion==6)  && (this->dwMinorVersion==1) ) ;}

    COsVersion(void);
    ~COsVersion(void);
};

