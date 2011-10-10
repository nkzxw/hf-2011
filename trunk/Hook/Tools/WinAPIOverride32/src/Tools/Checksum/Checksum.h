#ifndef CCHECKSUM_INCLUDE
#define CCHECKSUM_INCLUDE

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

#ifndef PBYTE
typedef unsigned char *PBYTE;
#endif

class CChecksum
{
public:
    static BOOL CheckChecksum(PBYTE Buffer,DWORD BufferSize,DWORD CRCToCheck,BOOL bBufferIsLittleEndian);
    static DWORD ComputChecksum(PBYTE Buffer,DWORD BufferSize,BOOL bBufferIsLittleEndian);
};

#endif