#include "WehnServ.h"
#include <time.h>
#include <wchar.h>

ScoreKeeper::ScoreKeeper()
: CurrentStateId(0),
  ExpirationCount(0)
{
}

ScoreKeeper::~ScoreKeeper()
{
	//
	// Flush all application states.
	//
	ExpireStaleStates(
			TRUE);
}

ULONG ScoreKeeper::GetTimeStamp()
{
	return (ULONG)time(NULL);
}

BOOLEAN ScoreKeeper::GetApplicationStateId(
		IN LPCWSTR InImageFilePath,
		OUT PULONG StateId)
{
	ApplicationState *State;
	BOOLEAN          Result = TRUE;
	
	State = GetApplicationState(
			InImageFilePath,
			NULL);

	if (State)
		*StateId = State->GetStateId();
	else
		Result = FALSE;

	return Result;
}

BOOLEAN ScoreKeeper::GetApplicationStateExitMethod(
		IN ULONG StateId,
		OUT PSELECTED_EXIT_METHOD ExitMethod)
{
	ApplicationState *State;
	BOOLEAN          Result = TRUE;
	
	State = GetApplicationState(
			NULL,
			StateId);

	if (State)
		*ExitMethod = State->GetExitMethod();
	else
		Result = FALSE;

	return Result;
}

BOOLEAN ScoreKeeper::IncrementExploitAttempts(
		IN ULONG StateId)
{
	ApplicationState *State;
	BOOLEAN          Result = TRUE;
	
	State = GetApplicationState(
			NULL,
			StateId);

	if (State)
		State->IncrementExploitAttempts();
	else
		Result = FALSE;

	return Result;
}

////
//
// Protected Methods
//
////

ApplicationState *ScoreKeeper::GetApplicationState(
		IN LPCWSTR InImageFilePath OPTIONAL,
		IN ULONG StateId OPTIONAL)
{
	std::map<ULONG, ApplicationState *>::iterator It;
	ApplicationState *State = NULL;

	if (++ExpirationCount % 100 == 0)
		ExpireStaleStates();

	//
	// If a state identifier was supplied, use that in preference.
	//
	if (StateId)
	{
		if ((It = ApplicationStateHash.find(
				StateId)) != ApplicationStateHash.end())
			State = (*It).second;
	}
	//
	// Otherwise, try to use the image file path if it was provided.
	//
	else if (InImageFilePath)
	{
		for (It = ApplicationStateHash.begin();
		     It != ApplicationStateHash.end();
		     It++)
		{
			if (!StrCmpW(
					(*It).second->GetImageFilePath(),
					InImageFilePath))
			{
				State = (*It).second;
				break;
			}
		}
	}

	//
	// If we couldn't find the state and an image file path was supplied, create
	// a new one.
	//
	if ((!State) &&
	    (InImageFilePath))
	{
		try
		{
			State = new ApplicationState(
					++CurrentStateId,
					InImageFilePath);
		} catch (...)
		{
		}

		//
		// Add its association to the hash.
		//
		if (State)
			ApplicationStateHash[State->GetStateId()] = State;
	}

	return State;
}

VOID ScoreKeeper::ExpireStaleStates(
		IN BOOLEAN FlushAll)
{
	std::map<ULONG, ApplicationState *>::iterator It;
	ULONG CurrentTimeStamp = GetTimeStamp();

	for (It = ApplicationStateHash.begin();
	     It != ApplicationStateHash.end();
	    )
	{
		//
		// If we're not removing all entries and this entry is not expired, don't
		// remove it.
		//
		if (!FlushAll)
		{
			if ((CurrentTimeStamp - (*It).second->GetLastTouchTime()) < APPLICATION_EXPIRATION_TIME_WINDOW)
			{
				It++;
				continue;
			}
		}

		//
		// If we get here, we're removing the entry.
		//
		ApplicationState *State = (*It).second;

		ApplicationStateHash.erase(It);

		delete State;

		It = ApplicationStateHash.begin();
	}
}
