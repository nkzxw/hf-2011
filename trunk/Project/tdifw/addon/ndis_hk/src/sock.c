// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: sock.c,v 1.1.1.1 2002/08/08 12:49:37 dev Exp $

#include <stdlib.h>
#include <string.h>
#include "sock.h"

u_long
ntohl (u_long netlong)
{
	u_long result = 0;
	((char *)&result)[0] = ((char *)&netlong)[3];
	((char *)&result)[1] = ((char *)&netlong)[2];
	((char *)&result)[2] = ((char *)&netlong)[1];
	((char *)&result)[3] = ((char *)&netlong)[0];
	return result;
}

u_short
ntohs (u_short netshort)
{
	u_short result = 0;
	((char *)&result)[0] = ((char *)&netshort)[1];
	((char *)&result)[1] = ((char *)&netshort)[0];
	return result;
}

u_long
inet_addr(const char *cp)
{
	int a, b, c, d;
	const char *p;

	p = cp;

	a = atoi(p);

	p = strchr(p, '.');
	if (p == NULL)
		return INADDR_NONE;
	
	b = atoi(++p);

	p = strchr(p, '.');
	if (p == NULL)
		return INADDR_NONE;

	c = atoi(++p);

	p = strchr(p, '.');
	if (p == NULL)
		return INADDR_NONE;

	d = atoi(++p);

	if (a < 0 || a > 255 ||
		b < 0 || b > 255 ||
		c < 0 || c > 255 ||
		d < 0 || d > 255)
		return 0;

	return (a) | (b << 8) | (c << 16) | (d << 24);
}

u_long
htonl(u_long netlong)
{
	// just reverse byte order
	return ntohl(netlong);
}

u_short
htons(u_short netshort)
{
	// just reverse byte order
	return ntohs(netshort);
}
