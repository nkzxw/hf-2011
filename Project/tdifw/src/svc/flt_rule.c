/* Copyright (c) 2002-2005 Vladislav Goncharov.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */
 
// $Id: flt_rule.c,v 1.9 2003/09/04 15:20:10 dev Exp $

/*
 * Rule parser
 */

#include <winsock.h>
#include <stdlib.h>

#include "flt_rule.h"
#include "ipc.h"
#include "tdifw_svc.h"

struct str_value {
	const	char *str;
	int		value;
};

static BOOL		str_value(const struct str_value *sv, const char *str, int *result);
static int		get_amp(char *str, u_long *addr, u_long *mask, u_short *port, u_short *port2);

static char g_delim[] = " \t";

static struct str_value filter_sv[] = {
	{"ALLOW",	FILTER_ALLOW},
	{"DENY",	FILTER_DENY},
	{NULL,		0}
};

static struct str_value proto_sv[] = {
	{"TCP",		IPPROTO_TCP},
	{"UDP",		IPPROTO_UDP},
	{"RawIP",	IPPROTO_IP},
	{"*",		IPPROTO_ANY},
	{NULL,		0}
};

static struct str_value direction_sv[] = {
	{"IN",		DIRECTION_IN},
	{"OUT",		DIRECTION_OUT},
	{"*",		DIRECTION_ANY},
	{NULL,		0}
};

/*
 * rule is like this:
 *
 * [<name>:] ALLOW|DENY TCP|UDP|RawIP|* IN|OUT|* FROM <addr> TO <addr> [NOLOG|COUNT]
 */
int
parse_rule(char *str, struct flt_rule *rule)
{
	char *p = str, *p2;
	int v;

	memset(rule, 0, sizeof(*rule));

	// by default log using of all rules!
	rule->log = RULE_LOG_LOG;

	// skip whitespace
	while (strchr(g_delim, *p) != NULL)
		p++;

	/* ALLOW|DENY */

	if (*p == '\0') {
		error("PARSE\tparse_rule: filter ALLOW or DENY is missing");
		return 0;
	}

	p2 = strpbrk(p, g_delim);
	if (p2 != NULL) {
		*(p2++) = '\0';
		while (strchr(g_delim, *p2) != NULL)
			p2++;
	}

	if (p[strlen(p) - 1] == ':') {
		// got name!

		p[strlen(p) - 1] = '\0';	// kill ending ':'

		strncpy(rule->rule_id, p, RULE_ID_SIZE);
		// string can be not zero-terminated

		p = p2;

		/* ALLOW|DENY again */

		if (p == NULL) {
			error("PARSE\tparse_rule: filter ALLOW or DENY is missing");
			return 0;
		}

		p2 = strpbrk(p, g_delim);
		if (p2 != NULL) {
			*(p2++) = '\0';
			while (strchr(g_delim, *p2) != NULL)
				p2++;
		}

	}

	if (!str_value(filter_sv, p, &v)) {
		error("PARSE\tparse_rule: \"%s\" is not ALLOW or DENY filter", p);
		return 0;
	}
	rule->result = v;

	p = p2;

	/* TCP|UDP|RawIP|* */

	if (p == NULL) {
		error("PARSE\tparse_rule: protocol TCP, UDP, RawIP or * is missing");
		return 0;
	}

	p2 = strpbrk(p, g_delim);
	if (p2 != NULL) {
		*(p2++) = '\0';
		while (strchr(g_delim, *p2) != NULL)
			p2++;
	}

	if (!str_value(proto_sv, p, &v)) {
		error("PARSE\tparse_rule: \"%s\" is not TCP, UDP, RawIP protocol or *", p);
		return 0;
	}
	rule->proto = v;
	
	p = p2;

	/* IN|OUT|* */

	if (p == NULL) {
		error("PARSE\tparse_rule: direction IN, OUT or * is missing");
		return 0;
	}

	p2 = strpbrk(p, g_delim);
	if (p2 != NULL) {
		*(p2++) = '\0';
		while (strchr(g_delim, *p2) != NULL)
			p2++;
	}

	if (!str_value(direction_sv, p, &v)) {
		error("PARSE\tparse_rule: \"%s\" is not IN, OUT or * direction", p);
		return 0;
	}
	rule->direction = v;
	
	p = p2;

	/* FROM */

	if (p == NULL) {
		error("PARSE\tparse_rule: keyword FROM is missing");
		return 0;
	}

	p2 = strpbrk(p, g_delim);
	if (p2 != NULL) {
		*(p2++) = '\0';
		while (strchr(g_delim, *p2) != NULL)
			p2++;
	}

	if (_stricmp(p, "FROM") != 0) {
		error("PARSE\tparse_rule: \"%s\" is not FROM keyword", p);
		return 0;
	}

	p = p2;

	/* <addr-from> */

	if (p == NULL) {
		error("PARSE\tparse_rule: from address is missing");
		return 0;
	}

	p2 = strpbrk(p, g_delim);
	if (p2 != NULL) {
		*(p2++) = '\0';
		while (strchr(g_delim, *p2) != NULL)
			p2++;
	}

	if (!get_amp(p, &rule->addr_from, &rule->mask_from, &rule->port_from, &rule->port2_from)) {
		error("PARSE\tparse_rule: invalid from address \"%s\"", p);
		return 0;
	}

	p = p2;

	/* TO */

	if (p == NULL) {
		error("PARSE\tparse_rule: keyword TO is missing");
		return 0;
	}

	p2 = strpbrk(p, g_delim);
	if (p2 != NULL) {
		*(p2++) = '\0';
		while (strchr(g_delim, *p2) != NULL)
			p2++;
	}

	if (_stricmp(p, "TO") != 0) {
		error("PARSE\tparse_rule: \"%s\" is not TO keyword");
		return 0;
	}

	p = p2;

	/* <addr-to> */

	if (p == NULL) {
		error("PARSE\tparse_rule: to address is missing");
		return 0;
	}

	p2 = strpbrk(p, g_delim);
	if (p2 != NULL) {
		*(p2++) = '\0';
		while (strchr(g_delim, *p2) != NULL)
			p2++;
	}

	if (!get_amp(p, &rule->addr_to, &rule->mask_to, &rule->port_to, &rule->port2_to)) {
		error("PARSE\tparse_rule: invalid to address \"%s\"", p);
		return 0;
	}

	p = p2;

	/* NOLOG|COUNT */
	if (p != NULL) {

		if (_stricmp(p, "NOLOG") == 0)
			rule->log = RULE_LOG_NOLOG;
		else if (_stricmp(p, "COUNT") == 0)
			rule->log = RULE_LOG_COUNT;
		else {
			error("PARSE\tparse_rule: invalid to address \"%s\"", p);
			return 0;
		}

	}

	return 1;
}

BOOL
str_value(const struct str_value *sv, const char *str, int *result)
{
	while (sv->str != NULL) {
		if (_stricmp(sv->str, str) == 0) {
			*result = sv->value;
			return TRUE;
		}
		sv++;
	}
	return FALSE;
}

int
get_amp(char *string, u_long *addr, u_long *mask, u_short *port, u_short *port2)
{
	char *p, *addr_str = NULL, *mask_str = NULL, *port_str = NULL,
		*port2_str = NULL, net_mask[10];
	int result = 0;
	
	addr_str = string;

	// "/mask"
	p = strchr(string, '/');
	if (p != NULL) {
		*(p++) = '\0';
		mask_str = p;
	}

	// ":port"
	p = strchr(mask_str ? mask_str : string, ':');
	if (p != NULL) {
		*(p++) = '\0';
		port_str = p;
		
		// "-port2"
		p = strchr(port_str, '-');
		if (p != NULL) {
			*(p++) = '\0';
			port2_str = p;
		}
	}

	// <address>[/<mask>]

	*addr = inet_addr(addr_str);
	if (*addr == INADDR_NONE && _stricmp(addr_str, "255.255.255.255") != 0) {

		*addr = get_host_by_name(addr_str, net_mask);
		if (*addr == INADDR_NONE) {
			// XXX try to resolve?
			error("PARSE\tInvalid address: %s", addr_str);
			return 0;
		}

		if (*net_mask != '\0') {
			// got mask in resolving
			mask_str = net_mask;
		}
	
	}

	if (mask_str != NULL) {
		// <mask>
		int n = atoi(mask_str);
		if ((n == 0 && _stricmp(mask_str, "0") != 0) || n < 0 || n > 32) {
			error("PARSE\tInvalid mask: %s", mask_str);
			return 0;
		}
		if (n == 0)
			*mask = 0;
		else {
			int i;
			for (i = 1, *mask = 0x80000000; i < n; i++)
				*mask |= *mask >> 1;
			*mask = htonl(*mask);
		}
	} else
		*mask = INADDR_NONE;	// default mask 255.255.255.255

	if (port_str != NULL) {
		int n = atoi(port_str);
		if ((n == 0 && _stricmp(port_str, "0") != 0) || n < 0 || n > 0xffff) {
			error("PARSE\tInvalid port: %s", port_str);
			return 0;
		}
		*port = (USHORT)n;
	} else
		*port = 0;

	if (port2_str != NULL) {
		int n = atoi(port2_str);
		if ((n == 0 && _stricmp(port2_str, "0") != 0) || n < 0 || n > 0xffff) {
			error("PARSE\tInvalid port2: %s", port2_str);
			return 0;
		}
		*port2 = (USHORT)n;
	} else
		*port2 = 0;

	// make network order
	*port = htons(*port);
	*port2 = htons(*port2);

	return 1;
}
