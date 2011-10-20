/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		learn.h
 *
 * Abstract:
 *
 *		This module implements various types and definitions used by the policy-auto-generation code.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 24-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */

#ifndef __LEARN_H__
#define __LEARN_H__


#include "policy.h"


extern BOOLEAN			LearningMode;

extern SECURITY_POLICY		NewPolicy;

/* In characters */
#define	MAX_PROCESS_NAME	32

extern WCHAR				ProcessToMonitor[];


BOOLEAN	InitLearningMode();
BOOLEAN	ShutdownLearningMode();
BOOLEAN	AddRule(RULE_TYPE RuleType, PCHAR str, UCHAR OperationType);


#endif	/* __LEARN_H__ */