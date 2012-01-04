#ifndef ___symMgr___
#define ___symMgr___


ULONG	getSymbols();

typedef struct HX_DYNC_FUNCTION
{
		char FunName[50];
		DWORD FunVar;
		DWORD FunAddr;
		DWORD	bNeedHook;	//this DWORD is for the inline hook purpose
		DWORD	delta;
}HX_DYNC_FUNCTION,*PHX_DYNC_FUNCTION;

void get_dync_funs_hook(HX_DYNC_FUNCTION **pdync_funs_hook, DWORD &Size);
void get_dync_funs(HX_DYNC_FUNCTION **pdync_funs, DWORD &Size);
void get_dync_funs_hook_old(HX_DYNC_FUNCTION **pdync_funs, DWORD &Size);

void get_dync_New2my(HX_DYNC_FUNCTION **pdync_funs_hook, DWORD &Size);

#endif