//////////////////////////////////////////////////////////////////////
//							Sockutil.h								//
//	This file contains the basic declarations of the functions that //
//	are used for the conversion of the network address into the		//
//  internet address and for conversion of port numbers into comp	//
//  readable format													//

//////////////////////////////////////////////////////////////////////
/* This function is to prevent the multiple inculsion of the same 
   file more than once
*/
#if !defined(SOCKUTIL_H)
#define SOCKUTIL_H

unsigned long	inet_addr(const char *sIp);

unsigned short	htons(unsigned short port);
#endif
