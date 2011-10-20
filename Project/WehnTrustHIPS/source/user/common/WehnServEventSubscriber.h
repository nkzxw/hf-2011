#ifndef _WEHNTRUST_COMMON_WEHNSERV_WEHNSERVEVENTSUBSCRIBER_H
#define _WEHNTRUST_COMMON_WEHNSERV_WEHNSERVEVENTSUBSCRIBER_H

#ifdef __cplusplus 

//
// This class acts as an interface to receiving various event notifications from
// the WehnServ service.  An example event that a subscriber would receive is
// when an exploitation attempt occurs.
//
class WehnServEventSubscriber
{
	public:
		WehnServEventSubscriber();
		virtual ~WehnServEventSubscriber();

		////
		//
		// Registration
		//
		////

		//
		// This method registers for receiving event notifications.  If the
		// operation fails, FALSE is returned.
		//
		BOOLEAN RegisterEventSubscriber();

		//
		// This method deregisters with WehnServ.
		//
		VOID DeregisterEventSubscriber();

		////
		//
		// Abstract interface that is meant to be overridden.
		//
		////

		//
		// This method is called when an exploitation event is received.
		//
		virtual VOID OnExploitationEvent(
				IN ULONG ProcessId,
				IN PEXPLOIT_INFORMATION ExploitInformation);

	protected:

		////
		//
		// Event monitoring thread entry point.
		//
		////

		static ULONG MonitorEventsSt(
				IN WehnServEventSubscriber *Subscriber);
		ULONG MonitorEvents();

		////
		//
		// Attributes
		//
		////

		//
		// Thread handle for the monitoring of events.
		//
		HANDLE EventMonitorThread;

		//
		// Indicates to the monitor thread that it should terminate.
		//
		BOOLEAN Terminate;

};

#endif

#endif
