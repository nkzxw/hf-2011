#ifndef _WEHNTRUST_COMMON_NATIVE_H
#define _WEHNTRUST_COMMON_NATIVE_H

//
// This class acts as a wrapper around a handful of useful NT native API
// routines.
//
class Native
{
	public:

		//
		// This routine provides the caller with the executable name that is
		// associated with the supplied process identifier.  The output
		// ProcessFilePath pointer must be freed with LocalFree once it is no
		// longer being used.
		//
		static ULONG GetProcessImageFileName(
				IN ULONG ProcessId,
				OUT PWCHAR ProcessFileName,
				IN ULONG ProcessFileNameLength,
				OUT PWCHAR *ProcessFilePath);

	protected:
};

#endif
