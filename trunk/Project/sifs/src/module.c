#include "fileflt.h"
/* ------------------------------------------------------------------------- */

int module_init(void)
{
	int rc = -1;

	if(TaskInit() == 0)
		rc = 0;
	
	return rc;
}

void module_exit(void)
{
	TaskExit();
}
