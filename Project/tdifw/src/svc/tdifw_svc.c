/* Copyright (c) 2002-2005 Vladislav Goncharov.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */
 
// $Id: tdifw_svc.c,v 1.23 2003/09/04 15:20:10 dev Exp $

/*
 * TdiFw helper service
 */

#include <windows.h>
#include <winioctl.h>
#include <winsock.h>
#include <mmsystem.h>
#include <stdio.h>
#include <time.h>

#include "iphlpapi.h"

#include "flt_rule.h"
#include "ipc.h"
#include "net.h"
#include "tdifw_svc.h"
#include "thread.h"

#include "msg.h"

#define DISP_BUF_SIZE	0x10000

HANDLE g_device = INVALID_HANDLE_VALUE;

static HANDLE g_event = NULL;
static HANDLE g_exit_event = NULL;

static HANDLE g_pipe = INVALID_HANDLE_VALUE;

static HANDLE g_dispatcher = NULL;
static char *g_disp_buf = NULL;

static DWORD WINAPI dispatcher(LPVOID param);
static DWORD WINAPI restart_thread(LPVOID param);

static void dispatch_request(struct flt_request *request);

static FILE *g_logfile = NULL;
static const char *g_config_file = NULL;

static BOOL		load_config(const char *config);
static BOOL		read_config(const char *config);
static BOOL		add_rules_name(const char *main_name, const char *config, int chain);
static void		add_rules(const char *config, char *buf, const char *name, int chain);
static BOOL		get_pname_by_pid(u_long pid, char *buf, int buf_size);
static void		prepare_addr(char *buf, int size, u_long addr);

static void		my_GetLongPathName(LPCSTR lpszShortPath, LPSTR lpszLongPath,
								   DWORD cchBuffer);

static int		compare_ln(const void *arg1, const void *arg2);
static int		compare_tn(const void *arg1, const void *arg2);

static void		get_traffic_stats(unsigned __int64 *stats);

// some config switches
static BOOL g_eventlog_allow = FALSE;
static BOOL g_eventlog_deny = FALSE;
static BOOL g_eventlog_error = FALSE;

// wave files to play :-)
static char wave_deny_in[MAX_PATH];
static char wave_deny_out[MAX_PATH];

static void log_msg(const char *msg, int type);

enum {
	MSGTYPE_ALLOW,
	MSGTYPE_DENY,
	MSGTYPE_ERROR
};

static char g_device_name[] = "\\\\.\\tdifw";
static char g_nfo_device_name[] = "\\\\.\\tdifw_nfo";

// for dynamic linking with psapi.dll

static void		link_psapi(void);

typedef BOOL WINAPI EnumProcesses_t(DWORD *, DWORD, DWORD *);
typedef BOOL WINAPI EnumProcessModules_t(HANDLE, HMODULE*, DWORD, DWORD*);
typedef BOOL WINAPI GetModuleFileNameEx_t(HANDLE, HMODULE, LPTSTR, DWORD);

static HMODULE g_psapi = NULL;
static EnumProcesses_t *pEnumProcesses = NULL;
static EnumProcessModules_t *pEnumProcessModules = NULL;
static GetModuleFileNameEx_t *pGetModuleFileNameEx = NULL;

// for routing code

static ULONG	get_if_ip(ULONG remote_ip);
static ULONG	route_ip(ULONG remote_ip);
static ULONG	get_if_index_ip(ULONG if_index);

// for SID stuff

static BOOL		load_users(const char *config);
static void		get_sid_mask(const char *config, const char *name, UCHAR *sid_mask);
static BOOL		check_for_name(const char *config, const char *section, const char *value, const char *name);

static const char *g_tcp_states[] = {
	"?",
	"SYN_SENT",
	"SYN_RCVD",
	"ESTABLISHED(in)",
	"ESTABLISHED(out)",
	"FIN_WAIT1",
	"FIN_WAIT2",
	"TIME_WAIT",
	"CLOSE_WAIT",
	"LAST_ACK",
	"CLOSED"
};

int
start(const char *config)
{
	int result = FALSE;
	WSADATA wsd;
	DWORD thread_id;

	WSAStartup(MAKEWORD(1, 1), &wsd);

	// try to dynamically link psapi.dll
	link_psapi();

	/* connect with driver */
	
	g_device = CreateFile(g_device_name, GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (g_device == INVALID_HANDLE_VALUE) {
		winerr(g_device_name);
		goto done;
	}

	/* open event for driver communication */

	g_event = OpenEvent(SYNCHRONIZE, FALSE, "tdifw_request");
	if (g_event == NULL) {
		winerr("start: OpenEvent");
		goto done;
	}

	/* load config & rules */

	if (!load_config(config))
		goto done;

	/* start dispatcher thread */

	g_exit_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_exit_event == NULL) {
		winerr("start: CreateEvent");
		goto done;
	}

	g_disp_buf = (char *)malloc(DISP_BUF_SIZE);
	if (g_disp_buf == NULL) {
		liberr("start: malloc");
		goto done;
	}

	log_msg("START", MSGTYPE_ALLOW);

	g_dispatcher = lib_CreateThread(NULL, 0, dispatcher, (LPVOID)config, 0, &thread_id);
	if (g_dispatcher == NULL) {
		winerr("start: lib_CreateThread");
		goto done;
	}
	
	result = TRUE;

done:
	if (!result)
		stop();

	return result;
}

void
stop(void)
{
	// collect statistics
	if (g_logfile != NULL) {
		// output traffic statistic!
		unsigned __int64 traffic[TRAFFIC_MAX];
		char msg[200];

		get_traffic_stats(traffic);

		sprintf(msg, "TRAFFIC\t%I64u/%I64u\t%I64u/%I64u",
			traffic[TRAFFIC_TOTAL_OUT], traffic[TRAFFIC_TOTAL_IN],
			traffic[TRAFFIC_COUNTED_OUT], traffic[TRAFFIC_COUNTED_IN]);
		
		log_msg(msg, MSGTYPE_ALLOW);
	}

	// disconnect from driver
	if (g_device != INVALID_HANDLE_VALUE) {
		HANDLE old_h = g_device;
		g_device = INVALID_HANDLE_VALUE;
		CancelIo(old_h);
		CloseHandle(old_h);
	}

	// stop dispatcher thread

	if (g_exit_event != NULL)
		SetEvent(g_exit_event);
	
	if (g_dispatcher != NULL) {
		WaitForSingleObject(g_dispatcher, INFINITE);
		g_dispatcher = NULL;
	}

	// close logfile
	if (g_logfile != NULL) {
		log_msg("STOP", MSGTYPE_ALLOW);

		fprintf(g_logfile, "--- end ---\n");
		fclose(g_logfile);
		g_logfile = NULL;
	}

	if (g_exit_event != NULL)
		CloseHandle(g_exit_event);
	if (g_event != NULL)
		CloseHandle(g_event);

	if (g_disp_buf != NULL) {
		free(g_disp_buf);
		g_disp_buf = NULL;
	}
}

void
wait(void)
{
	if (g_exit_event != NULL)
		WaitForSingleObject(g_exit_event, INFINITE);
}

/* output functions */

void
error(const char *fmt, ...)
{
	va_list ap;
	char message[1024];

	// prepare message

	va_start(ap, fmt);

	if (_vsnprintf(message, sizeof(message), fmt, ap) == -1)
		message[sizeof(message) - 1] = '\0';

	va_end(ap);

	// got message
	log_msg(message, MSGTYPE_ERROR);
}

void
winerr(const char *fn)
{
	char *win_msg = NULL;
	DWORD code = GetLastError();

	if (code == 0)
		error("WINERR\t%s:", fn);
	else {
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM,NULL,code,
			MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)&win_msg,0,NULL);

		if (win_msg != NULL) {
			// kill \r\n
			int len = strlen(win_msg);
			
			if (len >= 1)
				win_msg[len-1]=0;
			if (len >= 2)
				win_msg[len-2]=0;
			
			error("WINERR\t%s: %s", fn, win_msg);

			LocalFree(win_msg);
		} else
			error("WINERR\t%s: %d (0x%x)", fn, code, code);
	}

	// save error code
	SetLastError(code);
}

void
liberr(const char *fn)
{
	int code = errno;
	if (code != 0)
		error("LIBERR\t%s: %s", fn, strerror(code));
	else
		error("LIBERR\t%s:", fn);
}

/* dispatcher thread */
DWORD WINAPI
dispatcher(LPVOID param)
{
	char *config = (char *)param;
	HANDLE handles[2];
	DWORD i, n;

	handles[0] = g_event;
	handles[1] = g_exit_event;

	for (;;) {
		if (!DeviceIoControl(g_device, IOCTL_CMD_GETREQUEST, NULL, 0,
			g_disp_buf, DISP_BUF_SIZE, &n, NULL)) {
			winerr("dispatcher: DeviceIoControl");
			break;
		}

		if (n == 0) {
			DWORD wait;
			
			// if working with log file flush it!
			if (g_logfile != NULL)
				fflush(g_logfile);

			// wait for data
			wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
			if (wait == WAIT_OBJECT_0 + 1)
				break;
			else if (wait != WAIT_OBJECT_0) {
				winerr("dispatcher: WaitForSingleObject");
				break;
			}
			continue;
		}

		for (i = 0; i < n;) {
			struct flt_request *request;
			
			if (n - i < sizeof(*request))
				break;

			request = (struct flt_request *)(g_disp_buf + i);

			dispatch_request(request);
			
			i += request->struct_size;
		}
	}

	if (g_device != INVALID_HANDLE_VALUE) {
		DWORD thread_id;

		error("dispatcher: unexpected exit!");
		
		// restart tdifw_svc
		lib_CreateThread(NULL, 0, restart_thread, config, 0, &thread_id);
	}

	return 0;
}

DWORD
WINAPI restart_thread(LPVOID param)
{
	char *config = (char *)param;

	error("restarting...");
	
	stop();
	start(config);
	
	return 0;
}

/* log filter request from filter driver */
void
dispatch_request(struct flt_request *request)
{
	u_long from_ip, to_ip;
	u_short from_port, to_port;
	char msg[1024], pname_buf[MAX_PATH], addr_from[100], addr_to[100], *pname, uname[200];

	if (request->log_skipped != 0) {
		if (_snprintf(msg, sizeof(msg), "SKIP\t%u", request->log_skipped) == -1)
			msg[sizeof(msg) - 1] = '\0';

		log_msg(msg, MSGTYPE_ERROR);	
	}

	if (request->pid != (ULONG)-1) {
		if (request->pname == NULL) {
			// try to resolve pid to pname
			if (get_pname_by_pid(request->pid, pname_buf + sizeof(DWORD), sizeof(pname_buf) - sizeof(DWORD))) {
				// send message to driver to send process name next time
				DWORD n;

				pname = pname_buf + sizeof(DWORD);
				*(DWORD *)pname_buf = request->pid;

				if (!DeviceIoControl(g_device, IOCTL_CMD_SETPNAME, pname_buf,
					sizeof(DWORD) + strlen(pname) + 1,
					NULL, 0, &n, NULL))
					winerr("DeviceIoControl");

			} else {
				error("PROCESS\tCan't resolve pid %u!", request->pid);
				sprintf(pname_buf, "pid:%u", request->pid);
				pname = pname_buf;
			}
		
		} else
			pname = (char *)&request[1];
	} else
		pname = "";

	if (request->sid_a != NULL) {
		SID_AND_ATTRIBUTES *sa;
		char user[100], domain[100];
		DWORD size1, size2;
		SID_NAME_USE type;

		if (request->pname != NULL) {
			char *buf = (char *)&request[1];
			buf += strlen(buf) + 1;
			sa = (SID_AND_ATTRIBUTES *)buf;
		} else
			sa = (SID_AND_ATTRIBUTES *)&request[1];

		// convert sid from relative pointer to absolute
		sa->Sid = (PSID)((char *)sa + (ULONG)sa->Sid);

		size1 = sizeof(user);
		size2 = sizeof(domain);

		if (LookupAccountSid(NULL, sa->Sid, user, &size1, domain, &size2, &type)) {

			if (_snprintf(uname, sizeof(uname), "{%s\\%s}", domain, user) == -1)
				uname[sizeof(uname) - 1] = '\0';

		} else
			strcpy(uname, "{??\\??}");		// TODO: convert SID to string

	} else
		uname[0] = '\0';
	
	// check is it request type "TYPE_RESOLVE_PID"
	if (request->type == TYPE_RESOLVE_PID) {

		if (_snprintf(msg, sizeof(msg), "PROCESS\t%u\t%s\t%s", request->pid, pname, uname) == -1)
			msg[sizeof(msg) - 1] = '\0';

		log_msg(msg, MSGTYPE_ALLOW);	
		
	} else if (request->type == TYPE_LISTEN || request->type == TYPE_NOT_LISTEN) {

		// using from_ip & from_port as listen_ip & listen_port
		from_ip = ((struct sockaddr_in *)&request->addr.from)->sin_addr.s_addr;
		from_port = ntohs(((struct sockaddr_in *)&request->addr.from)->sin_port);

		prepare_addr(addr_from, sizeof(addr_from), from_ip);
		sprintf(addr_from + strlen(addr_from), ":%d", from_port);

		if (_snprintf(msg, sizeof(msg),
			"%s\tTCP\t%s\t%s\t%s",
			(request->type == TYPE_LISTEN) ? "LISTEN" : "NOT_LISTEN",
			addr_from, pname, uname) == -1)
			msg[sizeof(msg) - 1] = '\0';

		log_msg(msg, MSGTYPE_ALLOW);	

	} else {
	
		// prepare message
		
		from_ip = ((struct sockaddr_in *)&request->addr.from)->sin_addr.s_addr;
		from_port = ntohs(((struct sockaddr_in *)&request->addr.from)->sin_port);
		to_ip = ((struct sockaddr_in *)&request->addr.to)->sin_addr.s_addr;
		to_port = ntohs(((struct sockaddr_in *)&request->addr.to)->sin_port);

		// prepare address "from" & "to"
		if (from_ip == 0) {
			// try to route "to" addr to get "from"
			from_ip = get_if_ip(to_ip);
		}
		prepare_addr(addr_from, sizeof(addr_from), from_ip);

		if (to_ip == 0) {
			// some kind of "reverse route" :-)
			to_ip = get_if_ip(from_ip);
		}
		prepare_addr(addr_to, sizeof(addr_to), to_ip);

		// add ports if nonzero
		if (from_port != 0)
			sprintf(addr_from + strlen(addr_from), ":%d", from_port);
		if (to_port != 0)
			sprintf(addr_to + strlen(addr_to), ":%d", to_port);

		if (request->result == FILTER_ALLOW || request->result == FILTER_DENY ||
			request->result == FILTER_DISCONNECT) {
			// log it! (TDI message)
			char tdi_msg[100 + RULE_ID_SIZE], size_str[64], *direction;

			switch (request->result) {
			case FILTER_ALLOW:
				strcpy(tdi_msg, "ALLOW");
				break;
			case FILTER_DENY:
				strcpy(tdi_msg, "DENY");
				break;
			default:
				strcpy(tdi_msg, "CLOSED");
			}

			if (request->result != FILTER_DISCONNECT) {
				int size;

				size_str[0] = '\0';

				switch (request->type) {
				case TYPE_CONNECT_CANCELED:
					strcat(tdi_msg, "(CANCELED)");
					break;
				case TYPE_CONNECT_RESET:
					strcat(tdi_msg, "(RESET)");
					break;
				case TYPE_CONNECT_TIMEOUT:
					strcat(tdi_msg, "(TIMEOUT)");
					break;
				case TYPE_CONNECT_UNREACH:
					strcat(tdi_msg, "(UNREACH)");
					break;
				case TYPE_CONNECT_ERROR:
					sprintf(tdi_msg + strlen(tdi_msg), "(ERR:%x)", request->status);
					break;
				case TYPE_DATAGRAM:
					size = (request->direction == DIRECTION_IN) ? request->log_bytes_in : request->log_bytes_out;
					if (size != (ULONG)-1)
						sprintf(size_str, "%u", size);

					break;
				}

				strcat(tdi_msg, "\t[");
				size = strlen(tdi_msg);
				memcpy(tdi_msg + size, request->log_rule_id, RULE_ID_SIZE);
				tdi_msg[size + RULE_ID_SIZE + 1] = '\0';	// string can be not zero-terminated
				strcat(tdi_msg, "]");
			
			} else
				sprintf(size_str, "%u/%u", request->log_bytes_out, request->log_bytes_in);

			switch (request->direction) {
			case DIRECTION_IN:
				direction = "IN";
				break;
			case DIRECTION_OUT:
				direction = "OUT";
				break;
			case DIRECTION_ANY:
				direction = "*";
				break;
			default:
				direction = "?";
			}

			if (_snprintf(msg, sizeof(msg),
				"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
				tdi_msg,
				(request->proto == IPPROTO_TCP) ? "TCP" :
					(request->proto == IPPROTO_UDP ? "UDP" : "RawIP"),
				direction,
				addr_from, addr_to, size_str, pname, uname) == -1)
				msg[sizeof(msg) - 1] = '\0';

			// sound it!
			if (request->result == FILTER_DENY) {

				// TODO: try to sound it by voice of user!

				if (request->direction == DIRECTION_IN && wave_deny_in[0] != '\0')
					PlaySound(wave_deny_in, NULL, SND_FILENAME | SND_ASYNC);
				if (request->direction == DIRECTION_OUT && wave_deny_out[0] != '\0')
					PlaySound(wave_deny_out, NULL, SND_FILENAME | SND_ASYNC);
			}

			log_msg(msg, request->result == FILTER_ALLOW ? MSGTYPE_ALLOW : MSGTYPE_DENY);	

		} else if (request->result == FILTER_PACKET_LOG || request->result == FILTER_PACKET_BAD) {
			char flags[50], proto[50];

			// prepare TCP flags or ICMP type
			if (request->result == FILTER_PACKET_LOG) {
				if (request->proto == IPPROTO_TCP) {
					UCHAR th_flags = request->packet.tcp_flags;

					flags[0] = (th_flags & TH_FIN) ? 'F' : '-';
					flags[1] = (th_flags & TH_SYN) ? 'S' : '-';
					flags[2] = (th_flags & TH_RST) ? 'R' : '-';
					flags[3] = (th_flags & TH_PUSH) ? 'P' : '-';
					flags[4] = (th_flags & TH_ACK) ? 'A' : '-';
					flags[5] = (th_flags & TH_URG) ? 'U' : '-';
					flags[6] = '\0';
				
				} else if (request->proto == IPPROTO_ICMP)
					sprintf(flags, "%d.%d", request->packet.icmp_type, request->packet.icmp_code);
				else
					flags[0] = '\0';
			} else
				flags[0] = '\0';

			switch (request->proto) {
			case IPPROTO_TCP:
				if (request->packet.tcp_state > TCP_STATE_NONE &&
					request->packet.tcp_state < TCP_STATE_MAX)
					sprintf(proto, "TCP(%s)", g_tcp_states[request->packet.tcp_state]);
				else
					strcpy(proto, "TCP");
				break;
			case IPPROTO_UDP:
				strcpy(proto, "UDP");
				break;
			case IPPROTO_ICMP:
				strcpy(proto, "ICMP");
				break;
			default:
				sprintf(proto, "%d", request->proto);
			}


			// log it! (packet filter message)
			if (_snprintf(msg, sizeof(msg),
				"%s\t%s\t%s\t%s\t%s\t%u\t%s",
				(request->result == FILTER_PACKET_LOG) ? "PACKET" : "BAD_PACKET",
				proto,
				(request->direction == DIRECTION_IN) ? "IN" : "OUT",
				addr_from, addr_to,
				(request->direction == DIRECTION_IN) ? request->log_bytes_in : request->log_bytes_out,
				flags) == -1)
				msg[sizeof(msg) - 1] = '\0';

			log_msg(msg, request->result == FILTER_PACKET_LOG ? MSGTYPE_ALLOW : MSGTYPE_DENY);	
		
		}
	}
}

/*
 * write message to console, log file or probably to eventlog
 * don't call any error functions! avoid infinite recurse.
 */
void
log_msg(const char *msg, int type)
{
	static int file_day = 0;	// for midnight checking
	static ULONG event_num = 1;	// number of event in log file

	// if working in console mode write message to console
	if (g_console) {
		if (type == MSGTYPE_ERROR)
			fprintf(stderr, "%s\n", msg);
		else
			printf("%s\n", msg);
	}


	// write to eventlog if filter is set
	if ((type == MSGTYPE_ALLOW && g_eventlog_allow) ||
		(type == MSGTYPE_DENY && g_eventlog_deny) ||
		(type == MSGTYPE_ERROR && g_eventlog_error)) {

		HANDLE hEventSource;
		const char *strings;

		// write message to event log
        // Use event logging to log the error.
        //
        hEventSource = RegisterEventSource(NULL, "tdifw_svc");

        if (hEventSource != NULL) {
			WORD event_type;

			strings = msg;		// write message without timestamp

			event_type = (type == MSGTYPE_ALLOW) ? EVENTLOG_AUDIT_SUCCESS :
				(type == MSGTYPE_DENY ? EVENTLOG_AUDIT_FAILURE : EVENTLOG_ERROR_TYPE);

            if (!ReportEvent(hEventSource, // handle of event source
					event_type,
					0,                    // event category
					MSG,                  // event ID
					NULL,                 // current user's SID
					1,                    // strings in lpszStrings
					0,                    // no bytes of raw data
					&strings,             // array of error strings
					NULL)) {              // no raw data
				
#ifdef _DEBUG
				MessageBeep(0);
				OutputDebugString("log_msg: ReportEvent\n");
#endif
			}

            DeregisterEventSource(hEventSource);
		}
	} else {

		// write message to log

		time_t tv;
		struct tm *tm;
		char tm_msg[1024];

		time(&tv);
		tm = localtime(&tv);
		if (tm == NULL) {
#ifdef _DEBUG
			MessageBeep(0);
			OutputDebugString("log_msg: localtime\n");
#endif
			return;
		}

		if (g_logfile == NULL || tm->tm_mday != file_day) {
			char fname[MAX_PATH];

			if (g_logfile != NULL) {
				// wow! we're working at midnight! change log file!
				fprintf(g_logfile, "--- midnight ---\n");
				fclose(g_logfile);
				g_logfile = NULL;
			}

			file_day = tm->tm_mday;
			
			// open logfile
			
			if (_snprintf(fname, sizeof(fname),
				"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.log",
				getenv("SystemRoot"),
				tm->tm_year + 1900,
				tm->tm_mon + 1,
				tm->tm_mday) == -1) {

#ifdef _DEBUG
				MessageBeep(0);
				OutputDebugString("log_msg: _snprintf overflow!\n");
#endif
				return;
			}

			g_logfile = fopen(fname, "a");
			if (g_logfile == NULL) {
#ifdef _DEBUG
				MessageBeep(0);
				OutputDebugString("log_msg: fopen error!\n");
#endif
				return;
			}

			fprintf(g_logfile, "--- begin ---\n");
		}

		if (_snprintf(tm_msg, sizeof(tm_msg),
			"%010u %02d:%02d:%02d\t%s",
			event_num++, tm->tm_hour, tm->tm_min, tm->tm_sec, msg) == -1)
			tm_msg[sizeof(tm_msg) - 1] = '\0';

		// write it to log file
		fprintf(g_logfile, "%s\n", tm_msg);
		fflush(g_logfile);
	
	}
}

#define MAX_SECTION_SIZE	(16 * 1024)

BOOL
load_config(const char *config)
{
	char *section = NULL, *p;
	DWORD n, pids[100];
	BOOL result = FALSE;
	int line, chain, i;

	if (!read_config(config))
		return FALSE;

	// save config to global variable (for get_host_by_name)
	g_config_file = config;

	// load information about users
	if (!load_users(config)) {
		error("start: Fatal error! Can't load users information!\n");
		goto done;
	}

	// parse & add rules default and process-related

	section = (char *)malloc(MAX_SECTION_SIZE);
	if (section == NULL) {
		liberr("malloc");
		goto done;
	}

	GetPrivateProfileSection("_main_", section, MAX_SECTION_SIZE, config);
	
	// get lines
	chain = 0;
	for (p = section, line = 1; *p != '\0'; p += strlen(p) + 1, line++) {
		char *p2;

		if (*p == ';' || *p == '#')
			continue;		// comment

		p2 = strchr(p, '=');
		if (p2 == NULL) {
			error("%s:[_main_]:%d: invalid line format (no '=' character)", config, line);
			continue;
		}

		if (chain >= MAX_CHAINS_COUNT) {
			error("%s:[_main_]:%d: too many rules lines!", config, line);
			break;
		}

		*p2 = '\0';		// temporary kill '='
		add_rules_name(p, config, chain);
		*p2 = '=';		// recover '=' to make strlen(p) works right

		chain++;
	}

	// try to get names for existing processes
	if (pEnumProcesses != NULL && pEnumProcesses(pids, sizeof(pids), &n)) {
		DWORD i;

		n /= sizeof(DWORD);

		for (i = 0; i < n; i++) {
			char pname[MAX_PATH];
			if (get_pname_by_pid(pids[i], pname + sizeof(DWORD), sizeof(pname) - sizeof(DWORD))) {
				// send information to driver about pid and pname
				
				DWORD nn;
				
				*(DWORD *)pname = pids[i];

				if (!DeviceIoControl(g_device, IOCTL_CMD_SETPNAME, pname,
					sizeof(DWORD) + strlen(pname + sizeof(DWORD)) + 1,
					NULL, 0, &nn, NULL))
					winerr("DeviceIoControl");
			}
		}
	}

	// activate all chains!
	for (i = 0; i < chain; i++) {
		// activate chain #i
		if (!DeviceIoControl(g_device, IOCTL_CMD_ACTIVATECHAIN, &i, sizeof(i),
				NULL, 0, &n, NULL))
			winerr("start: DeviceIoControl");
	}

	result = TRUE;
done:
	free(section);
	return result;
}

BOOL
read_config(const char *config)
{
	static char config_name[] = "_config_";
	static char signature_name[] = "_signature_";
	static char signature_value[] = "$tdi_fw$";

	char buf[100];

	// first, check config file signature
	GetPrivateProfileString(signature_name, signature_name, "", buf, sizeof(buf), config);
	if (strcmp(buf, signature_value) != 0) {
		error("\"%s\": invalid configuration file", config);
		return FALSE;
	}

	// get g_eventlog_xxx values

	g_eventlog_allow = GetPrivateProfileInt(config_name, "eventlog_allow", FALSE, config);
	g_eventlog_deny = GetPrivateProfileInt(config_name, "eventlog_deny", FALSE, config);
	g_eventlog_error = GetPrivateProfileInt(config_name, "eventlog_error", FALSE, config);

	// read wave_deny_in/out values

	GetPrivateProfileString(config_name, "wave_deny_in", "", wave_deny_in, sizeof(wave_deny_in), config);
	GetPrivateProfileString(config_name, "wave_deny_out", "", wave_deny_out, sizeof(wave_deny_out), config);

	return TRUE;
}

BOOL
add_rules_name(const char *main_name, const char *config, int chain)
{
	char buf[1024], *p, *p2, *section = NULL;
	BOOL result = FALSE;
	DWORD n;

	section = (char *)malloc(MAX_SECTION_SIZE);
	if (section == NULL) {
		liberr("malloc");
		goto done;
	}

	/* 1. get ruleset string */

	GetPrivateProfileString("_main_", main_name, "", buf, sizeof(buf), config);

	if (*buf == '\0')
		goto done;		// no rules

	// reset all rules for chain
	if (!DeviceIoControl(g_device, IOCTL_CMD_CLEARCHAIN, &chain, sizeof(chain),
		NULL, 0, &n, NULL)) {
		winerr("DeviceIoControl");
		goto done;
	}

	if (chain != 0) {
		// set chain name
		int len = sizeof(int) + MAX_PATH;
		char *data = (char *)malloc(len);
		if (data == NULL) {
			liberr("malloc");
			goto done;
		}

		*(int *)data = chain;

		// try to extract environment variables from p to main_name
		ExpandEnvironmentStrings(main_name, data + sizeof(int), MAX_PATH);

		len = sizeof(int) + strlen(data + sizeof(int)) + 1;
		
		if (!DeviceIoControl(g_device, IOCTL_CMD_SETCHAINPNAME, data, len,
			NULL, 0, &n, NULL)) {
			winerr("DeviceIoControl");
			free(data);
			goto done;
		}
		free(data);
	}

	/* 2. for each word in main_name string */

	p = buf;
	
	while (p != NULL) {

		p2 = strchr(p, ' ');
		if (p2 != NULL) {
			while (*p2 == ' ')
				*(p2++) = '\0';
		}
		
		if (*p != '\0') {
			// get section by name in p
			GetPrivateProfileSection(p, section, MAX_SECTION_SIZE, config);
			if (*section == '\0')
				error("\"%s\": unexistant or empty section %s", config, p);
			else
				add_rules(config, section, p, chain);
		}

		p = p2;
	}

	result = TRUE;

done:
	free(section);
	return result;
}

void
add_rules(const char *config, char *buf, const char *name, int chain)
{
	char *p, *p2;
	int n_str;
	DWORD n;
	UCHAR sid_mask[MAX_SIDS_COUNT / 8];

	get_sid_mask(config, name, sid_mask);

	for (p = buf, n_str = 1; *p != '\0'; p = p2, n_str++) {
		struct flt_rule rule;
		char num[10];

		p2 = p + strlen(p) + 1;

		if (*buf == ';' || *buf == '\0')
			continue;		// empty line or comment

		memset(&rule, 0, sizeof(rule));
		
		// parse it!
		if (!parse_rule(p, &rule)) {
			error("Error in line #%d of section [%s]", n_str, name);
			continue;
		}

		// set chain
		rule.chain = chain;

		// set SID mask
		memcpy(rule.sid_mask, sid_mask, sizeof(rule.sid_mask));

		if (rule.rule_id[0] == '\0') {
			// set default rule name: name of section + n_str
			strncpy(rule.rule_id, name, RULE_ID_SIZE);	// string can be not zero-terminated
			sprintf(num, ":%d", n_str);
			if (strlen(name) + strlen(num) < RULE_ID_SIZE)
				memcpy(rule.rule_id + strlen(name), num, strlen(num));	// string can be not zero-terminated
			else
				memcpy(rule.rule_id + RULE_ID_SIZE - strlen(num), num, strlen(num));
		}

		// append rule
		if (!DeviceIoControl(g_device, IOCTL_CMD_APPENDRULE, &rule, sizeof(rule),
				NULL, 0, &n, NULL)) {
			winerr("start: DeviceIoControl");
			break;
		}
	}
}

BOOL
get_pname_by_pid(u_long pid, char *buf, int buf_size)
{
	BOOL result;
	HANDLE h_process;
	HMODULE h_module = NULL;
	DWORD n;

	if (g_psapi == NULL)
		return FALSE;			// failed to load psapi.dll

	// try to resolve pid to pname
	
	h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (h_process != NULL)
		pEnumProcessModules(h_process, &h_module, sizeof(h_module), &n);

	// on error write pid
	if (h_module == NULL ||
		pGetModuleFileNameEx(h_process, h_module, buf, buf_size) == 0) {

		// for "System" process last error value is:
		// * ERROR_PARTIAL_COPY (on 2k)
		// * ERROR_NOACCESS (on NT4)
		// ??? on other (TODO: think about another way)
		if (GetLastError() == ERROR_PARTIAL_COPY ||
			GetLastError() == ERROR_NOACCESS) {
			
			strncpy(buf, "System", buf_size);
			buf[buf_size - 1] = '\0';
			
			result = TRUE;
		
		} else {
			*buf = '\0';
			result = FALSE;
		}

	} else {
		if (strchr(buf, '~') != NULL) {		// XXX is it a right way?
			// try to convert long name to short name
			char long_name[MAX_PATH];
			
			my_GetLongPathName(buf, long_name, sizeof(long_name));
				
			strncpy(buf, long_name, buf_size - 1);
			buf[buf_size - 1] = '\0';
		}

		result = TRUE;
	}

	if (h_process != NULL)
		CloseHandle(h_process);
	return result;
}

void
prepare_addr(char *buf, int size, u_long addr)
{
	// XXX do we need resolving?
	strcpy(buf, inet_ntoa(*(struct in_addr *)&addr));
}

ULONG
get_host_by_name(const char *hostname, char *net_mask)
{
	// resolve address using config file (DNS way is insecure)
	char addr[100], *p;

	GetPrivateProfileString("_hosts_", hostname, "", addr, sizeof(addr), g_config_file);

	if (*addr == '\0')
		return INADDR_NONE;

	p = strchr(addr, '/');
	if (p != NULL) {
		// got net_mask!
		*(p++) = '\0';

		// max size is 2 characters! (/xx)
		strncpy(net_mask, p, 2);
		net_mask[2] = '\0';
	} else
		*net_mask = '\0';

	return inet_addr(addr);
}

// GetLongPathName is in Windows 2000 only so make it for NT4
void
my_GetLongPathName(LPCSTR lpszShortPath, LPSTR lpszLongPath, DWORD cchBuffer)
{
	LPSTR p_l;
	LPCSTR p_s;

	lpszLongPath[0] = lpszShortPath[0];
	lpszLongPath[1] = lpszShortPath[1];
	lpszLongPath[2] = lpszShortPath[2];
	lpszLongPath[3] = '\0';
	
	p_l = &lpszLongPath[3];
	p_s = &lpszShortPath[3];

	for (;;) {
		LPSTR p_s2;
		HANDLE search;
		WIN32_FIND_DATA ffd;
		size_t len;
		BOOL overflow;

		p_s2 = strchr(p_s, '\\');

		if (p_s2 != NULL)
			len = p_s2 - p_s;
		else
			len = strlen(p_s);

		// check the buffer
		if (len + (p_l - lpszLongPath) >= cchBuffer) {
			// buffer overflow: copy and exit
			len = cchBuffer - (p_l - lpszLongPath) - 1;
			overflow = TRUE;
		} else
			overflow = FALSE;
		
		memcpy(p_l, p_s, len);
		p_l[len] = '\0';
	
		if (overflow)
			break;

		search = FindFirstFile(lpszLongPath, &ffd);
		if (search != INVALID_HANDLE_VALUE) {

			len = strlen(ffd.cFileName);
			if (p_s2 != NULL)
				len++;

			// check the buffer
			if (len + (p_l - lpszLongPath) >= cchBuffer) {
				// buffer overflow: copy and exit
				len = cchBuffer - (p_l - lpszLongPath) - 1;
				p_s2 = NULL;
			}

			memcpy(p_l, ffd.cFileName, len);
			
			if (p_s2 != NULL) {
				p_l[len - 1] = '\\';
				p_l[len] = '\0';
			} else
				p_l[len] = '\0';

			FindClose(search);
		}

		if (p_s2 == NULL)
			break;

		p_l += strlen(p_l);
		p_s = p_s2 + 1;
	}

	// ignore status
}

// dynamically try to link with psapi.lib
void
link_psapi(void)
{
	g_psapi = LoadLibrary("psapi.dll");
	if (g_psapi == NULL) {
		// no psapi.dll - log it!
		error("INIT\tCan't load psapi.dll!");
		return;
	}

	pEnumProcesses = (EnumProcesses_t *)GetProcAddress(g_psapi, "EnumProcesses");
	pEnumProcessModules = (EnumProcessModules_t *)GetProcAddress(g_psapi, "EnumProcessModules");
#ifdef UNICODE
	pGetModuleFileNameEx = (GetModuleFileNameEx_t *)GetProcAddress(g_psapi, "GetModuleFileNameExW");
#else
	pGetModuleFileNameEx = (GetModuleFileNameEx_t *)GetProcAddress(g_psapi, "GetModuleFileNameExA");
#endif

	if (pEnumProcesses == NULL || pEnumProcessModules == NULL || pGetModuleFileNameEx == NULL) {
		// invalid psapi.dll?
		error("INIT\tInvalid psapi.dll!");

		FreeLibrary(g_psapi);
		g_psapi = NULL;
	}
}

#define PRINT_IP_ADDR(addr) \
	((UCHAR *)&(addr))[0], ((UCHAR *)&(addr))[1], ((UCHAR *)&(addr))[2], ((UCHAR *)&(addr))[3]

// enum listening objects
void
enum_listen(void)
{
	ULONG size;
	struct listen_nfo *ln = NULL;
	int i, n;

	// try to dynamically link psapi.dll
	link_psapi();

	/* connect with driver */
	
	g_device = CreateFile(g_nfo_device_name, GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (g_device == INVALID_HANDLE_VALUE) {
		winerr(g_nfo_device_name);
		goto done;
	}

	/* get list of listening objects */

	size = sizeof(*ln) * 0x10000 * 3;	// this size is good enough :-)
	ln = (struct listen_nfo *)malloc(size);
	if (ln == NULL) {
		perror("malloc");
		goto done;
	}

	if (!DeviceIoControl(g_device, IOCTL_CMD_ENUM_LISTEN, NULL, 0,
		ln, size, &size, NULL)) {
		winerr("DeviceIoControl");
		goto done;
	}

	n = size / sizeof(*ln);

	// sort this list!
	qsort(ln, n, sizeof(*ln), compare_ln);

	printf("IPProto\tAddress:Port\tProcess (pid)\n");
	printf("-------\t------------\t---------------------------------------------\n");

	for (i = 0; i < n ; i++) {
		char *proto, pname[MAX_PATH];
		
		if (ln[i].ipproto == IPPROTO_TCP)
			proto = "TCP";
		else if (ln[i].ipproto == IPPROTO_UDP)
			proto = "UDP";
		else if (ln[i].ipproto == IPPROTO_IP)
			proto = "RawIP";
		else
			proto = "?";

		// resolve pid!
		if (!get_pname_by_pid(ln[i].pid, pname, sizeof(pname)))
			pname[0] = '\0';

		printf("%s\t%d.%d.%d.%d:%d\t%s (%d)\n",
			proto, PRINT_IP_ADDR(ln[i].addr), ntohs(ln[i].port), pname, ln[i].pid);
	}

done:
	free(ln);
	if (g_device != INVALID_HANDLE_VALUE)
		CloseHandle(g_device);
}

int
compare_ln(const void *arg1, const void *arg2)
{
	const struct listen_nfo *l1 = (const struct listen_nfo *)arg1;
	const struct listen_nfo *l2 = (const struct listen_nfo *)arg2;
	int result;

	result = l1->ipproto - l2->ipproto;
	if (result == 0) {
		result = ntohl(l2->addr) - ntohl(l1->addr);
		if (result == 0)
			result = ntohs(l1->port) - ntohs(l2->port);
	}
	return result;
}

// enum connections
void
enum_connect(void)
{
	ULONG size;
	struct tcp_conn_nfo *tn = NULL;
	int i, n;
	unsigned __int64 traffic[TRAFFIC_MAX];

	// try to dynamically link psapi.dll
	link_psapi();

	/* connect with driver */
	
	g_device = CreateFile(g_nfo_device_name, GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (g_device == INVALID_HANDLE_VALUE) {
		winerr(g_nfo_device_name);
		goto done;
	}

	/* get list of listening objects */

	size = sizeof(*tn) * 0x10000 * 3;	// this size is good enough :-)
	tn = (struct tcp_conn_nfo *)malloc(size);
	if (tn == NULL) {
		perror("malloc");
		goto done;
	}

	if (!DeviceIoControl(g_device, IOCTL_CMD_ENUM_TCP_CONN, NULL, 0,
		tn, size, &size, NULL)) {
		winerr("DeviceIoControl");
		goto done;
	}

	n = size / sizeof(*tn);

	// sort this list!
	qsort(tn, n, sizeof(*tn), compare_tn);

	for (i = 0; i < n ; i++) {
		char pname[MAX_PATH];

		if (tn[i].state >= TCP_STATE_MAX)
			tn[i].state = 0;
		
		// resolve pid!
		if (!get_pname_by_pid(tn[i].pid, pname, sizeof(pname)))
			pname[0] = '\0';

		printf("%s\t%d.%d.%d.%d:%d\t%d.%d.%d.%d:%d\t%s (%d)\t%u/%u\n",
			g_tcp_states[tn[i].state],
			PRINT_IP_ADDR(tn[i].laddr), ntohs(tn[i].lport),
			PRINT_IP_ADDR(tn[i].raddr), ntohs(tn[i].rport),
			pname, tn[i].pid,
			tn[i].bytes_out, tn[i].bytes_in);
	}

	// output traffic counters
	get_traffic_stats(traffic);

	printf(
		"\n"
		"Traffic counters (out/in):\n"
		"    Total:   %I64u/%I64u\n"
		"    Counted: %I64u/%I64u\n",
		traffic[TRAFFIC_TOTAL_OUT], traffic[TRAFFIC_TOTAL_IN],
		traffic[TRAFFIC_COUNTED_OUT], traffic[TRAFFIC_COUNTED_IN]);

done:
	free(tn);
	if (g_device != INVALID_HANDLE_VALUE)
		CloseHandle(g_device);
}

int
compare_tn(const void *arg1, const void *arg2)
{
	const struct tcp_conn_nfo *t1 = (const struct tcp_conn_nfo *)arg1;
	const struct tcp_conn_nfo *t2 = (const struct tcp_conn_nfo *)arg2;
	int result;

	result = t1->state - t2->state;
	if (result == 0) {
		result = ntohl(t1->laddr) - ntohl(t2->laddr);
		if (result == 0) {
			result = ntohs(t1->lport) - ntohs(t2->lport);
			if (result == 0) {
				result = ntohl(t1->raddr) - ntohl(t2->raddr);
				if (result == 0)
					result = ntohs(t1->rport) - ntohs(t2->rport);
			}
		}
	}

	return result;
}

/*
 * Routing functions help us to get local IP address of outgoing datagrams or connections
 * Warning! This functions are slow!
 * TODO: make it faster!
 */

ULONG
get_if_ip(ULONG remote_ip)
{
	ULONG if_index;
	
	// get interface index by routing table
	if_index = route_ip(remote_ip);
	if (if_index == (ULONG)-1)
		return 0;

	return get_if_index_ip(if_index);
}

ULONG
route_ip(ULONG remote_ip)
{
	ULONG size, i, bestmetric, result;
	MIB_IPFORWARDTABLE *buf;
	
	// get route table
	
	size = 0;
	if (GetIpForwardTable(NULL, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER)
		return (ULONG)-1;

	buf = (MIB_IPFORWARDTABLE *)malloc(size);
	if (buf == NULL)
		return (ULONG)-1;

	if (GetIpForwardTable(buf, &size, FALSE) != NO_ERROR) {
		free(buf);
		return (ULONG)-1;
	}

	// find optimal route

	bestmetric = (ULONG)-1;
	result = (ULONG)-1;

	for (i = 0; i < buf->dwNumEntries; i++) {
		if ((remote_ip & buf->table[i].dwForwardMask) ==
			(buf->table[i].dwForwardDest & buf->table[i].dwForwardMask)) {

			if (bestmetric == (ULONG)-1 || buf->table[i].dwForwardMetric1 > bestmetric) {
				bestmetric = buf->table[i].dwForwardMetric1;
				result = buf->table[i].dwForwardIfIndex;
			}

		}
	}

	free(buf);
	return result;
}

ULONG
get_if_index_ip(ULONG if_index)
{
	ULONG size, i;
	MIB_IPADDRTABLE *buf;
	ULONG result;

	size = 0;
	if (GetIpAddrTable(NULL, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER)
		return (ULONG)-1;

	buf = (MIB_IPADDRTABLE *)malloc(size);
	if (buf == NULL)
		return (ULONG)-1;

	if (GetIpAddrTable(buf, &size, FALSE) != NO_ERROR) {
		free(buf);
		return (ULONG)-1;
	}

    result = 0;
	for (i = 0; i < buf->dwNumEntries; i++)
		if (buf->table[i].dwIndex == if_index) {
			result = buf->table[i].dwAddr;
			break;
		}

	free(buf);
	return result;
}

BOOL
load_users(const char *config)
{
	BOOL result = FALSE;
	char *section, *p, *all_sids = NULL;
	int line, sid_id;
	ULONG all_sids_len = 0, all_sids_size = 0, n;

	section = (char *)malloc(MAX_SECTION_SIZE);
	if (section == NULL) {
		liberr("malloc");
		return FALSE;
	}
	GetPrivateProfileSection("_users_", section, MAX_SECTION_SIZE, config);
	
	// get lines
	sid_id = 0;
	for (p = section, line = 1; *p != '\0'; p += strlen(p) + 1, line++) {
		char *p2, *p3, *authority, *principal, domain[100];
		char *sid_buf;
		DWORD sid_size, domain_size;
		SID_NAME_USE use;

		if (*p == ';')
			continue;		// comment

		p2 = strchr(p, '=');
		if (p2 == NULL) {
			error("%s:[_users_]:%d: invalid line format (no '=' character)", config, line);
			continue;
		}

		*p2 = '\0';		// temporary kill '='

		if (sid_id != 0) {
			// not _default_
			p3 = strchr(p, '\\');
			if (p3 != NULL) {
				*p3 = '\0';

				authority = p;
				if (strcmp(authority, "NT AUTHORITY") == 0)
					authority = NULL;

				principal = p3 + 1;

			} else {
				authority = NULL;
				principal = p;
			}

			// try to get SID size
			sid_size = 0;
			domain_size = 0;

			LookupAccountName(authority, principal, NULL, &sid_size, NULL, &domain_size, &use);
		} else
			sid_size = 0;		// empty SID for default

		if (all_sids_len + sid_size + sizeof(ULONG) >= all_sids_size) {
			all_sids_size += sid_size + sizeof(ULONG) + 4096;
			all_sids = (char *)realloc(all_sids, all_sids_size);
			if (all_sids == NULL) {
				liberr("malloc");
				goto done;
			}
		}
		sid_buf = all_sids + all_sids_len;
		all_sids_len += sid_size + sizeof(ULONG);

		*(ULONG *)sid_buf = sid_size;

		if (sid_id != 0) {
			domain_size = sizeof(domain);

			if (!LookupAccountName(authority, principal,
				(PSID)(sid_buf + sizeof(ULONG)), &sid_size, domain, &domain_size, &use)) {
				winerr("LookupAccountName");
				error("%s:[_users_]:%d: Error getting SID for %s\\%s", config, line,
					(authority != NULL ? authority : "."), principal);
				goto done;
			}
			if (use != SidTypeUser && use != SidTypeWellKnownGroup) {
				error("%s:[_users_]:%d: Invalid SID type (%d) for %s\\%s", config, line, use,
					(authority != NULL ? authority : "."), principal);
				goto done;
			}

			if (p3 != NULL)
				*p3 = '\\';
		}
	
		*p2 = '=';		// recover '=' to make strlen(p) works right
		sid_id++;
	}

	// send all_sids buffer to filter driver!
	result = DeviceIoControl(g_device, IOCTL_CMD_SET_SIDS, all_sids, all_sids_len,
		NULL, 0, &n, NULL);
	if (!result)
		winerr("DeviceIoControl");

done:
	free(all_sids);
	free(section);
	return result;
}

void
get_sid_mask(const char *config, const char *name, UCHAR *sid_mask)
{
	BOOL result = FALSE;
	char *section, *p;
	int line, sid_id;

	memset(sid_mask, 0, MAX_SIDS_COUNT / 8);

	section = (char *)malloc(MAX_SECTION_SIZE);
	if (section == NULL) {
		liberr("malloc");
		return;
	}
	GetPrivateProfileSection("_users_", section, MAX_SECTION_SIZE, config);
	
	// get lines
	sid_id = 0;
	for (p = section, line = 1; *p != '\0'; p += strlen(p) + 1, line++) {
		char *p2;

		if (*p == ';')
			continue;		// comment

		p2 = strchr(p, '=');
		if (p2 == NULL) {
			error("%s:[_users_]:%d: invalid line format (no '=' character)", config, line);
			continue;
		}

		*p2 = '\0';		// temporary kill '='

		if (check_for_name(config, "_users_", p, name))
			sid_mask[sid_id / 8] |= 1 << (sid_id % 8);		// set bit in mask
		
		*p2 = '=';		// recover '=' to make strlen(p) works right

		sid_id++;
	}

	free(section);
}

BOOL
check_for_name(const char *config, const char *section, const char *value, const char *name)
{
	char buf[1024], *p, *p2;

	GetPrivateProfileString(section, value, "", buf, sizeof(buf), config);

	p = buf;

	if (strcmp(p, "*") == 0)
		return TRUE;			// *
	
	while (p != NULL) {

		p2 = strchr(p, ' ');
		if (p2 != NULL) {
			while (*p2 == ' ')
				*(p2++) = '\0';
		}
		
		if (*p != '\0' && strcmp(p, name) == 0)
			return TRUE;

		p = p2;
	}

	return FALSE;
}

void
get_traffic_stats(unsigned __int64 *stats)
{
	DWORD n;
	memset(stats, 0, sizeof(unsigned __int64) * TRAFFIC_MAX);
	DeviceIoControl(g_device, IOCTL_CMD_GET_COUNTERS, NULL, 0, stats, sizeof(unsigned __int64) * TRAFFIC_MAX, &n, NULL);
}
