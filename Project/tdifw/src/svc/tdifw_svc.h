// $Id: tdifw_svc.h,v 1.5 2003/05/14 16:34:01 dev Exp $

#ifndef _tdifw_svc_h_
#define _tdifw_svc_h_

#include "ipc.h"

int		start(const char *config);
void	stop(void);

void	wait(void);

void	error(const char *fmt, ...);
void	winerr(const char *fn);
void	liberr(const char *fn);

#ifdef _DEBUG
#	define DEBUG(_x_)	error _x_
#else
#	define DEBUG(_x_)
#endif

// debug heap support
#ifdef _MSC_VER
#	define _CRTDBG_MAP_ALLOC
#	include <crtdbg.h>
#endif

#ifdef _MSC_VER
#	define _LEAK_CHECK  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF)
#else
#	define _LEAK_CHECK
#endif

extern BOOL g_console;

ULONG	get_host_by_name(const char *hostname, char *net_mask);

void enum_listen(void);
void enum_connect(void);

#endif
