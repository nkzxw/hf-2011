//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/18
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// Abstract:
//		This module used to manage the ACL memory. First time, 
//		allocate full size of ACL file, and after, dynamic allocate 
//		page when add ACL's record.
//
//
//

#ifndef ACLMANAGE_H
#define ACLMANAGE_H

#define KERNEL_MODE
#include "FILT.H"

extern int checkapp();
extern int checknnb();
extern int checkicmp();
extern int checkweb();
extern int checktorjan();
extern int protectappchange();

#endif //ACLMANAGE_H