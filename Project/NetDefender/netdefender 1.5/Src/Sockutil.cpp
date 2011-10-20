#include "stdafx.h"
#include "sockutil.h"
#include <stdlib.h>
#include <string.h>

/*++

Routine Description:

    Convert the ip to the format we have to compare

Arguments:

    ip - string that represent a ip address


Return Value:

	ip

--*/
unsigned long inet_addr(const char *sIp)
{
	int				octets[4];
	int				i;
	const char		*auxCad = sIp;
	unsigned long	lIp = 0;

	//we extract each octet of the ip address
	//atoi will get characters until it found a non numeric character(in our case '.')
	for(i = 0; i < 4; i++)
	{
		octets[i] = atoi(auxCad);

		if(octets[i] < 0 || octets[i] > 255)
		{
			return(0);
		}

		lIp |= (octets[i] << (i * 8));

		//update auxCad to point to the next octet
		auxCad = strchr(auxCad, '.');

		if(auxCad == NULL && i != 3)
		{
			return(0);
		}

		auxCad++;
	}

	return(lIp);
}

/*++

Routine Description:

    Convert the port to the format we have to compare

Arguments:

    port - port to convert


Return Value:

	port converted

--*/
unsigned short htons(unsigned short port)
{
	unsigned short	portRet;

	portRet = ((port << 8) | (port >> 8));

	return(portRet);
}
