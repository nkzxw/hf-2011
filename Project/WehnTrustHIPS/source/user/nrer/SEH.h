/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#ifndef _WEHNTRUST_NRER_SEH_H
#define _WEHNTRUST_NRER_SEH_H

ULONG InstallSehValidationFrame();

BOOLEAN IsSehChainValid(
		OUT PEXPLOIT_INFORMATION ExploitInformation);

#endif
