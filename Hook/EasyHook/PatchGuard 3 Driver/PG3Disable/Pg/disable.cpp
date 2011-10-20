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

#include "EasyHookDriver.h"

#ifndef _M_X64
	#error "This driver part is intended for 64-bit builds only."
#endif

/************************************************************************
*************************************** PgDisablePatchGuard
*************************************************************************

Description:

	Will disable PatchGuard 3 by directly manipulating the
	windows machine code...

*/
BOOLEAN PgDisablePatchGuard(PDEVICE_OBJECT InDevice)
{
	MEMORY_PATCH_INFO*		PatchArray = (MEMORY_PATCH_INFO*)ExAllocatePool(PagedPool, sizeof(MEMORY_PATCH_INFO) * 9);

	__try
	{
		/*
			Enumerate PatchGuard fingerprint locations...
		*/
		PatchArray[0] = VistaAll_KiCustomAccessRoutinePatch();

		PatchArray[1] = VistaSp1_KiTimerListExpirePatch();
		PatchArray[2] = VistaSp1_KiRetireDpcListPatch();

		PatchArray[3] = VistaSp0_KiTimerListExpire_01_Patch();
		PatchArray[4] = VistaSp0_KiTimerListExpire_02_Patch();
		PatchArray[5] = VistaSp0_KiRetireDpcListPatch();
		PatchArray[6] = VistaAll_FingerprintPatch(FALSE);
		PatchArray[7] = VistaSp0_ExpWorkerThreadPatch();
		PatchArray[8] = VistaSp1_ExpWorkerThreadPatch();

		PgProcessMemoryPatchList(PatchArray, 9);

		// only patch if supported...
		if(!VistaAll_IsPatchable(&PatchArray[0]))
			return FALSE;

		if(VistaSP1_IsPatchable(
				&PatchArray[1],
				&PatchArray[2],
				&PatchArray[8]))
		{
			/*
				Outcomment this if you want to risk a BSOD in some cases.
			*/
			if(PatchArray[6].LocCount < 10)
				return FALSE;

			// we are running on Service Pack 1...
			VistaAll_BeginPatches(&PatchArray[0]);

			VistaSP1_ApplyPatches(
				&PatchArray[1],
				&PatchArray[2],
				&PatchArray[0],
				&PatchArray[8]);

			VistaAll_EndPatches();
		}
		else if(VistaSP0_IsPatchable(
				&PatchArray[3],
				&PatchArray[4],
				&PatchArray[5],
				&PatchArray[7]))
		{
			/*
				Outcomment this if you want to risk a BSOD in some cases.
			*/
			if(PatchArray[6].LocCount < 11)
				return FALSE;

			// we are running on Service Pack 0...
			VistaAll_BeginPatches(&PatchArray[0]);

			VistaSP0_ApplyPatches(
				&PatchArray[3],
				&PatchArray[4],
				&PatchArray[5],
				&PatchArray[0],
				&PatchArray[7]);

			VistaAll_EndPatches();
		}
		else
		{
			// not supported or already patched...
			return FALSE;
		}

		/*
			Prevent unloading...
		*/
		ObReferenceObject(InDevice);

		return TRUE;
	}
	__finally
	{
		ExFreePool(PatchArray);
	}
}