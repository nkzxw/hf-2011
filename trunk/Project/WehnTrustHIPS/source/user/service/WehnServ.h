/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNSERV_WEHNSERV_H
#define _WEHNTRUST_WEHNSERV_WEHNSERV_H

#pragma warning(disable : 4786)

#include <windows.h>
#include <winioctl.h>
#include <list>
#include <map>
#define  APP_LOG_FILE "WehnServ.log"
#include "../Common/Common.h"
#include "ServLog.h"
#include "ScoreKeeper.h"

//
// The number of pipe instances to allocate.
//
#define PIPE_INSTANCES 6

typedef enum _PIPE_CLIENT_STATE
{
	PipeClientConnecting,
	PipeClientReading,
	PipeClientWriting
} PIPE_CLIENT_STATE, *PPIPE_CLIENT_STATE;

typedef struct _PIPE_CLIENT
{
	HANDLE            Pipe;
	OVERLAPPED        Ov;
	WEHNSERV_REQUEST  Request;
	WEHNSERV_RESPONSE Response;
	PIPE_CLIENT_STATE State;
	BOOLEAN           Pending;
	BOOLEAN           MonitoringEvents;
} PIPE_CLIENT, *PPIPE_CLIENT;

#include "EventManager.h"

//
// This class represents the WehnTrust logging and notification service 
// object.
//
class WehnServ
{
	public:
		WehnServ();
		~WehnServ();

		//
		// Service dispatcher routines
		//
		static VOID WINAPI ServiceEntrySt(
				IN UINT Ac, 
				IN LPCTSTR Av);
		static VOID WINAPI ServiceCtrlSt(
				IN ULONG Code);

		//
		// Object instance initialization that occurs after the service has
		// started.
		//
		VOID Initialize();

		//
		// Executes the main thread of the service.
		//
		VOID Run();

		//
		// Returns the ScoreKeeper reference.
		//
		ScoreKeeper &GetScoreKeeper();

		//
		// Sends the client's response buffer to the client.
		//
		VOID SendResponseToPipeClient(
				IN PPIPE_CLIENT Client,
				IN PWEHNSERV_RESPONSE Response OPTIONAL = NULL);
		
	protected:

		//
		// Processes a signaled event for a given pipe client.
		//
		VOID ProcessPipeClient(
				IN PPIPE_CLIENT Client);

		//
		// Reconnect the pipe client.
		//
		VOID ReconnectPipeClient(
				IN PPIPE_CLIENT Client,
				IN BOOLEAN Disconnect = FALSE);

		//
		// Handles the processing of a given packet.
		//
		BOOLEAN DispatchPacket(
				IN PPIPE_CLIENT Client,
				IN ULONG ProcessId,
				IN PWEHNSERV_REQUEST Request,
				OUT PWEHNSERV_RESPONSE Response);

		//
		// SCM status notification.
		//
		VOID ReportStatusToScm(
				IN ULONG CurrentState,
				IN ULONG ErrorCode = NO_ERROR,
				IN ULONG WaitHint = 0);

		//
		// Returns the process identifier associated with the named pipe client.
		//
		ULONG GetPipeClientProcessId(
				IN PPIPE_CLIENT Client);

		//
		// Destroys all open pipe clients.
		//
		VOID FlushPipeClients();

		////
		//
		// Attributes
		//
		////

		//
		// Service control manager notification checkpoint.
		//
		ULONG Checkpoint;

		//
		// Service control handle.
		//
		SERVICE_STATUS_HANDLE StatusHandle;

		//
		// Service status.
		//
		SERVICE_STATUS ServiceStatus;

		//
		// The event that is used to stop the service.
		//
		HANDLE StopEvent;

		//
		// This event is used to cause the WehnServ process to refresh its active
		// log sinks.
		//
		HANDLE RefreshEvent;

		//
		// The array of pipe clients.
		//
		PIPE_CLIENT Pipes[PIPE_INSTANCES];

		//
		// The score keeper instance that keeps track of crashing processes.
		//
		ScoreKeeper Keeper;

		//
		// The event manager context.
		//
		EventManager EvManager;
};

#endif
