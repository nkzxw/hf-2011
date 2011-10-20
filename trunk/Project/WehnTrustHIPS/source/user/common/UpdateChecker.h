#ifndef _WEHNTRUST_COMMON_UPDATECHECKER_H
#define _WEHNTRUST_COMMON_UPDATECHECKER_H

typedef VOID (*UPDATE_NOTIFICATION_ROUTINE)(
		IN PVOID UserParameter,
		IN CHAR VersionAvailable[32]);

//
// This class checks to see if there are any updates available
//
class UpdateChecker
{
	public:
		UpdateChecker(
				IN PVOID UserParameter,
				IN UPDATE_NOTIFICATION_ROUTINE Routine);

		static BOOLEAN CheckForUpdates(
				OUT CHAR VersionAvailable[32]);

		static HANDLE StartPeriodicChecker(
				IN PVOID UserParameter,
				IN UPDATE_NOTIFICATION_ROUTINE Routine);
		static VOID CancelPeriodicChecker(
				IN HANDLE CheckerHandle);
	protected:

		static ULONG PeriodicCheckerThread(
				IN UpdateChecker *Instance);

		PVOID                       UserParameter;
		UPDATE_NOTIFICATION_ROUTINE Routine;
};

#endif
