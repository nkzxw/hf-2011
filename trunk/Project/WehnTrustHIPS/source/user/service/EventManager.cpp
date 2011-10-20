#include "WehnServ.h"

VOID EventManager::SetWehnServ(
		WehnServ *InContext)
{
	Context = InContext;
}

VOID EventManager::RegisterEventClient(
		IN PPIPE_CLIENT Client)
{
	EventClientList.push_back(
			Client);

	Client->MonitoringEvents = TRUE;
}

VOID EventManager::DeregisterEventClient(
		IN PPIPE_CLIENT Client)
{
}

VOID EventManager::NotifyExploitationEvent(
		IN ULONG ProcessId,
		IN PEXPLOIT_INFORMATION ExploitInformation)
{
	std::list<PPIPE_CLIENT>::iterator It;
	WEHNSERV_RESPONSE                 Response;

	Response.Event.Type = WehnServNotifyExploitationEvent;
	Response.Event.ProcessId = ProcessId;

	CopyMemory(
			&Response.Event.ExploitInformation,
			ExploitInformation,
			sizeof(EXPLOIT_INFORMATION));

	for (It = EventClientList.begin();
	     It != EventClientList.end();
	     It++)
	{
		Context->SendResponseToPipeClient(
				(*It),
				&Response);
	}
}
