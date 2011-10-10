#include "OsVersion.h"


COsVersion::COsVersion(void)
{
    this->dwVersion = 0; 
    this->dwMajorVersion = 0;
    this->dwMinorVersion = 0; 
    this->dwBuild = 0;

    dwVersion = ::GetVersion();

    // Get the Windows version.
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    // Get the build number.
    if (dwVersion < 0x80000000)              
        dwBuild = (DWORD)(HIWORD(dwVersion));

}


COsVersion::~COsVersion(void)
{
}
