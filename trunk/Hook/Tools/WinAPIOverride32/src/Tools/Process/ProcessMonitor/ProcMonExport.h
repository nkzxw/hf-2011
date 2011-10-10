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
// Object: Needed const and structs shared between driver and applications
//-----------------------------------------------------------------------------


// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
#define FILE_DEVICE     0x00008000

// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
#define IOCTL_BASE_INDEX  0x800

/////////////////////////////////
// Define our own private IOCTL
/////////////////////////////////

// to start monitoring
#define IOCTL_START_MONITORING      (unsigned long)CTL_CODE(FILE_DEVICE , \
                                                         IOCTL_BASE_INDEX,  \
                                                         METHOD_BUFFERED,     \
                                                         FILE_READ_DATA | FILE_WRITE_DATA)
// to stop monitoring
#define IOCTL_STOP_MONITORING       (unsigned long)CTL_CODE(FILE_DEVICE , \
                                                         IOCTL_BASE_INDEX+1,  \
                                                         METHOD_BUFFERED,     \
                                                         FILE_READ_DATA | FILE_WRITE_DATA)

// to get process info, overlapped mode supported
#define IOCTL_GET_PROCINFO          (unsigned long)CTL_CODE(FILE_DEVICE,  \
                                                         IOCTL_BASE_INDEX+2,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_READ_DATA | FILE_WRITE_DATA)
// equivalent to CancelIo() API, but can be call from any thread
#define IOCTL_CANCEL_IO             (unsigned long)CTL_CODE(FILE_DEVICE,  \
                                                         IOCTL_BASE_INDEX+3,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_READ_DATA | FILE_WRITE_DATA)


// Structure for process callback information
typedef struct tagProcessCallbackInfo
{
    HANDLE  hParentId;
    HANDLE  hProcessId;
    BOOLEAN bCreate;
} PROCESS_CALLBACK_INFO, *PPROCESS_CALLBACK_INFO;