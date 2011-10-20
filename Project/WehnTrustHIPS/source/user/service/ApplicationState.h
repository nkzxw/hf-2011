#ifndef _WEHNTRUST_WEHNSERV_APPLICATIONSTATE_H
#define _WEHNTRUST_WEHNSERV_APPLICATIONSTATE_H

//
// 5 minutes is the maximum brute force period.
//
#define BRUTE_FORCE_PERIOD 300

//
// This class represents the state of a given application.
//
class ApplicationState
{
	public:
		ApplicationState(
				IN ULONG StateId,
				IN LPCWSTR ImageFilePath);
		~ApplicationState();

		//
		// Returns the state identifier associated with this application state.
		//
		ULONG GetStateId();

		//
		// Returns the image file path associated with this application state.
		//
		LPCWSTR GetImageFilePath();

		//
		// Increments the total number of exploit attempts found to be associated
		// with this application.
		//
		VOID IncrementExploitAttempts();

		//
		// Returns the currently decided upon view of the best exit method for
		// this application based on recent exploitation attempts.
		//
		SELECTED_EXIT_METHOD GetExitMethod();

		//
		// Returns the timestamp of the last time the application state was
		// modified.
		//
		ULONG GetLastTouchTime();
	protected:

		////
		//
		// Attributes
		//
		////
		
		//
		// The state identifier for this application state.
		//
		ULONG StateId;

		//
		// The image file associated with this application state.
		//
		LPWSTR ImageFilePath;

		//
		// Holds the timestamp of when the application state was last touched.
		//
		ULONG LastTouchTime;

		//
		// The list of time stamps that an exploitation attempt occurred out, the
		// most recent being at the front.  This provides an easy glance to see if
		// there have been a number of exploit attempts in recent time.
		//
		ULONG ExploitAttemptTimeStamps[5];

		//
		// The total number of exploit attempts that this process has been
		// subjected to.
		//
		ULONG NumberOfExploitAttempts;
};

#endif
