#ifndef _WEHNTRUST_WEHNSERV_SCOREKEEPER_H
#define _WEHNTRUST_WEHNSERV_SCOREKEEPER_H

#include "ApplicationState.h"

//
// Application states expire after 30 minutes.  This is measured in seconds.
//
#define APPLICATION_EXPIRATION_TIME_WINDOW 1800

//
// This class is responsible for keep tabs on processes that have had
// exploitation attempts caught and to make recommendations on what the best
// course of action is for a crashing process in terms of what exit method
// should be used or whether or not a user-mode application should be notified,
// for instance.
//
class ScoreKeeper
{
	public:
		ScoreKeeper();
		~ScoreKeeper();

		//
		// Returns the current timestamp in seconds since 1970.
		//
		static ULONG GetTimeStamp();

		//
		// This method searches for the state identifier associated with the
		// supplied image file path and returns it in the output parameter.  If
		// one does not exist, a new state identifier is allocated and associated
		// with the process.  After a given interval of time, the state will be
		// invalidated as to prevent over consumption.
		//
		BOOLEAN GetApplicationStateId(
				IN LPCWSTR ImageFilePath,
				OUT PULONG StateId);

		//
		// Provides the caller with the exit method that is associated with the
		// supplied state identifier.
		//
		BOOLEAN GetApplicationStateExitMethod(
				IN ULONG StateId,
				OUT PSELECTED_EXIT_METHOD ExitMethod);

		//
		// Increments the number of exploitation attempts a given application
		// state has.
		//
		BOOLEAN IncrementExploitAttempts(
				IN ULONG StateId);
	protected:

		//
		// Expunges all dead application states that have not been touched in
		// quite a while.
		//
		VOID ExpireStaleStates(
				IN BOOLEAN FlushAll = FALSE);

		//
		// Returns the application state associated with one of the provided
		// operands.
		//
		ApplicationState* GetApplicationState(
				IN LPCWSTR ImageFilePath OPTIONAL,
				IN ULONG StateId OPTIONAL);

		////
		//
		// Attributes
		//
		////

		//
		// The hash associating state identifiers to application state's
		//
		std::map<ULONG, ApplicationState *> ApplicationStateHash;

		//
		// The current state identifier pool.
		//
		ULONG CurrentStateId;

		//
		// This variable counts the number of iterations between expiring stale
		// application states.
		//
		ULONG ExpirationCount;
};

#endif
