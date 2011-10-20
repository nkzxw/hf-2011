/*
Released under MIT License

Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Visit http://www.codeplex.com/easyhook for more information.
*/


#ifndef _DRIVERSHARED_H_
#define _DRIVERSHARED_H_

#pragma warning (disable:4100) // unreference formal parameter
#pragma warning(disable:4201) // nameless struct/union

#define EASYHOOK_WIN32_DEVICE_NAME		L"\\\\.\\EasyHook"
#define EASYHOOK_DEVICE_NAME			L"\\Device\\EasyHook"
#define EASYHOOK_DOS_DEVICE_NAME		L"\\DosDevices\\EasyHook"
#define FILE_DEVICE_EASYHOOK			0x893F
#define EASYHOOK_IOCTL_BASE				0x800

#define CTL_CODE_EASYHOOK(i)	\
	CTL_CODE(FILE_DEVICE_EASYHOOK, EASYHOOK_IOCTL_BASE+i, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


int __stdcall GetInstructionLength_x64(void* InPtr, int InType);


/*
	IO command will return STATUS_SUCCESS if PatchGuard has been disabled.
	Any other status code may be returned. In such a case you should invoke
	IOCTL_PATCHGUARD_QUERY, because an error is also indicated if PatchGuard
	is already disabled...
*/
#define IOCTL_PATCHGUARD_DISABLE			CTL_CODE_EASYHOOK(0) // ==> PgDisablePatchGuard()
#define IOCTL_PATCHGUARD_DUMP				CTL_CODE_EASYHOOK(1) // ==> PgDumpTimerTable()
#define IOCTL_PATCHGUARD_PROBE				CTL_CODE_EASYHOOK(2) // ==> PgInstallTestHook()



#endif