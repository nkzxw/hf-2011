#include "WehnServ.h"

ApplicationState::ApplicationState(
		IN ULONG StateId,
		IN LPCWSTR InImageFilePath)
: StateId(StateId),
  NumberOfExploitAttempts(0)
{
	ImageFilePath = StrDupW(
			InImageFilePath);
	
	LastTouchTime = ScoreKeeper::GetTimeStamp();
}

ApplicationState::~ApplicationState()
{
	if (ImageFilePath)
		LocalFree(
				ImageFilePath);
}

ULONG ApplicationState::GetStateId()
{
	return StateId;
}

LPCWSTR ApplicationState::GetImageFilePath()
{
	return ImageFilePath 
		? ImageFilePath
		: L"";
}

VOID ApplicationState::IncrementExploitAttempts()
{
	LastTouchTime = ScoreKeeper::GetTimeStamp();

	ExploitAttemptTimeStamps[NumberOfExploitAttempts++ % 5] = LastTouchTime;
}

SELECTED_EXIT_METHOD ApplicationState::GetExitMethod()
{
	SELECTED_EXIT_METHOD ExitMethod = UseExitThread;

	//
	// If there have been 10 exploitation attempts, tell the process to exit.
	//
	if (NumberOfExploitAttempts > 10)
		ExitMethod = UseExitProcess;
	//
	// Check to see how periodic the exploit attempts have been.
	//
	else if (NumberOfExploitAttempts > 5)
	{
		ULONG CurrentIndex;
		ULONG Count;
		ULONG WithinWindowCount = 0;

		//
		// By default, we'll assume that we're going to use ExitProcess.
		//
		ExitMethod = UseExitProcess;

		//
		// Walk each previous time stamp comparing the differences with the
		// current to see if they are spread apart by less than the maximum brute
		// force period.
		//
		for (Count = 4, CurrentIndex = NumberOfExploitAttempts - 1;
		     Count > 0;
		     Count--, CurrentIndex--)
		{
			ULONG Difference;
			ULONG Idx1 = (CurrentIndex - 1) % 5;
			ULONG Idx2 = CurrentIndex % 5;

			//
			// Calculate the difference between the two time stamps
			//
			Difference = ExploitAttemptTimeStamps[Idx2] - ExploitAttemptTimeStamps[Idx1];

			//
			// If it's within the brute force period, increment the number of
			// attempts within a brute force window.
			//
			if (Difference < BRUTE_FORCE_PERIOD)
				WithinWindowCount++;
		}

		//
		// If there were fewer than 3 attempts that were within the brute force
		// window, use the ExitThread method since it might be possible that a
		// brute force attempt is not occurring.
		//
		if (WithinWindowCount < 3)
			ExitMethod = UseExitThread;
	}

	//
	// By default we instruct the application to use ExitThread so that it might
	// continue to operate.  This is especially important for things like system
	// services.
	//
	return ExitMethod;
}

ULONG ApplicationState::GetLastTouchTime()
{
	return LastTouchTime;
}
