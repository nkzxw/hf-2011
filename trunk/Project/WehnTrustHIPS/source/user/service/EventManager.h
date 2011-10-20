#ifndef _WEHNTRUST_WEHNSERV_EVENTMANAGER_H
#define _WEHNTRUST_WEHNSERV_EVENTMANAGER_H

class WehnServ;

class EventManager
{
	public:

		VOID SetWehnServ(
				IN WehnServ *Context);

		VOID RegisterEventClient(
				IN PPIPE_CLIENT EventClient);
		VOID DeregisterEventClient(
				IN PPIPE_CLIENT EventClient);

		VOID NotifyExploitationEvent(
				IN ULONG ProcessId,
				IN PEXPLOIT_INFORMATION ExploitInformation);
	protected:

		std::list<PPIPE_CLIENT> EventClientList;
		WehnServ                *Context;
};

#endif
