#include "checksum.h"
//-----------------------------------------------------------------------------
// Name: CheckChecksum
// Object: Verify checksum
// Parameters :
//     in : PBYTE Buffer : buffer to check checksum
//          DWORD BufferSize : buffer size in byte
//          BOOL bBufferIsLittleEndian : true if computer is little endian
//          DWORD CRCToCheck : checksum to compare with real one
//     out :
//     return : TRUE if checksum is ok
//              FALSE if bad checksum
//-----------------------------------------------------------------------------
BOOL CChecksum::CheckChecksum(PBYTE Buffer,DWORD BufferSize,DWORD CRCToCheck,BOOL bBufferIsLittleEndian)
{
    return (CRCToCheck==CChecksum::ComputChecksum(Buffer,BufferSize,bBufferIsLittleEndian));
}
//-----------------------------------------------------------------------------
// Name: ComputChecksum
// Object: comput checksum
// Parameters :
//     in : PBYTE Buffer : buffer to comput checksum
//          DWORD BufferSize : buffer size in byte
//          BOOL bBufferIsLittleEndian : true if computer is little endian
//     out :
//     return : checksum value
//-----------------------------------------------------------------------------
DWORD CChecksum::ComputChecksum(PBYTE Buffer,DWORD BufferSize,BOOL bBufferIsLittleEndian)
{
    DWORD dwChecksum=0;
    DWORD cnt;

    if (!Buffer)
        return 0;

    if (BufferSize%2==0)
    {
        if (bBufferIsLittleEndian)
        {
            for (cnt=0;cnt<BufferSize;cnt+= 2) 
                dwChecksum +=(unsigned short) ((Buffer[cnt+1]<<8)+Buffer[cnt]);
        }
        else
        {
            for (cnt=0;cnt<BufferSize;cnt+= 2) 
                dwChecksum +=(unsigned short) ((Buffer[cnt]<<8)+Buffer[cnt+1]);
        }
    }
    else
    {
        if (bBufferIsLittleEndian)
        {
            for (cnt=0;cnt<BufferSize;cnt+= 2) 
                dwChecksum +=(unsigned short) ((Buffer[cnt+1]<<8)+Buffer[cnt]);
        }
        else
        {
            for (cnt=0;cnt<BufferSize;cnt+= 2) 
                dwChecksum +=(unsigned short) ((Buffer[cnt]<<8)+Buffer[cnt+1]);
        }
        dwChecksum+=Buffer[BufferSize-1];
    }

    dwChecksum = (dwChecksum >> 16) + (dwChecksum & 0xffff);
    dwChecksum += (dwChecksum >> 16);
    dwChecksum=~dwChecksum;

    return dwChecksum;
}
