#ifndef __FLT_TASK_I_H__
#define __FLT_TASK_I_H__

#define TASK_NAME_MAX_LEN     		20     
#define NT_TASK_NAME_MAX_LEN  	16   
#define TASK_RULE_DATA_MAX_LEN 	32

typedef enum _TASK_ATTR_INDEX {

	TASK_ATTR_FLAGS1,
	TASK_ATTR_FLAGS2,
	TASK_ATTR_DNCF
}TASK_ATTR_INDEX, *PTASK_ATTR_INDEX;

typedef struct _FLT_TASK_CONTEXT
{
	LIST_ENTRY	task_next;

	LONG		reference;
	KSPIN_LOCK  lock;
	KIRQL		irq;
	
	KEVENT		*event;
	HANDLE		task_id;
	HANDLE		parent_id;

	// fcrypt
	CHAR		flag1 : 4; /* 是否是可信进程 : 3为特殊进程 */
	CHAR		flag2 : 4; /* 是否加载HOOK DLL : 3:3;3:5;3:6为特殊标志*/

	UCHAR		dncf:1, /* 不允许更改flag */ 
				state:1;

	CHAR		reverse[2];

	// hscan
	ULONG		data_len;
	CHAR		data[TASK_RULE_DATA_MAX_LEN + 1];

	CHAR		reverse1[3];

}FLT_TASK_CONTEXT, *PFLT_TASK_CONTEXT;

#define FLT_TASK_CONTEXT_LIST_GLOBAL  0x01
#define FLT_TASK_CONTEXT_LIST_HSCAN    0x02

extern int process_session_init(void);
extern void process_session_exit(void);

extern int process_session_add_system_task(HANDLE pid, HANDLE parent_id);
extern ULONG process_session_get_system_task_state(HANDLE pid);
extern int process_session_set_explorer_hook(HANDLE pid, BOOLEAN hook);
extern int process_session_check_son_of_service(HANDLE pid);
extern int process_session_remove_system_task(HANDLE pid, HANDLE parent_id);
extern int process_session_check_validate(void);

extern int task_user_init(void);
extern void task_user_exit(void);
extern void task_user_hold_task_context(PFLT_TASK_CONTEXT task_context);
extern void task_user_up_task_context(PFLT_TASK_CONTEXT task_context);
extern PFLT_TASK_CONTEXT task_user_find_task_context(HANDLE task_id);
extern int task_user_add_task_context(HANDLE task_id, HANDLE parent_id, PCHAR data, ULONG data_len);
extern int task_user_del_task_context(PFLT_TASK_CONTEXT task_context);
extern int task_user_add_hscan_task_context(HANDLE task_id, HANDLE parent_id, PCHAR data, ULONG data_len);
extern int task_user_del_hscan_task_context(HANDLE task_id);
extern LONG task_user_get_hscan_task_count(void);
extern ULONG task_user_get_task_attribute(PFLT_TASK_CONTEXT task_context);
extern int task_user_set_task_attribute(PFLT_TASK_CONTEXT task_context, CHAR flag, CHAR type);
extern void task_user_lock_task_context(PFLT_TASK_CONTEXT task_context);
extern void task_user_unlock_task_context(PFLT_TASK_CONTEXT task_context);
extern void task_user_clear_hscan_task_context(void);
extern void task_user_restart_scan_host(void);

int
TaskRuleInit(
	VOID
	);

VOID
TaskRuleExit(
	VOID
	);

FLT_TASK_SLOT_INDEX
TaskRuleGetSlotIndex(
	__in HANDLE TaskId
	);

int
TaskRuleGetImageHash(
	__in HANDLE Pid,
	__out PUCHAR Md5Hash
	);

int
TaskRuleCheckFCRuleValidate(
	VOID
	);

int
TaskRuleCheckNDRuleValidate(
	VOID
	);

int
TaskRuleCheckHSRuleValidate(
	VOID
	);

int
TaskRuleCheckFCValidate(
	__in HANDLE ProcessId,
	__in PVOID Key,
	__in LONG  KeyLen
	);

int
TaskRuleCheckNDValidate(
	__in HANDLE ProcessId,
	__in PVOID Key,
	__in LONG  KeyLen
	);

int
TaskRuleCheckHSValidate(
	__in HANDLE ProcessId,
	__in PVOID Key,
	__in LONG  KeyLen
	);

VOID
TaskRuleClearHSRule(
	VOID
	);

LONG
TaskRuleGetHSState(
	VOID
	);

int
TaskRuleAddNDRule(
	__in PVOID Key,
	__in LONG KeyLen
	);

VOID
TaskRuleClearNDRule(
	VOID
	);

int
TaskRuleAddFCRule(
	__in PVOID Key,
	__in LONG KeyLen
	);

VOID
TaskRuleClearFCRule(
	VOID
	);

int
TaskRuleAddHSRule(
	__in PVOID Key,
	__in LONG KeyLen
	);

VOID
TaskRuleClearHSRule(
	VOID
	);

int 
TaskGetNameById(
	__in HANDLE TaskId, 
	__out PCHAR TaskName
	);

int 
TaskCheckTaskByName(
	__in HANDLE proc_id, 
	__in CHAR *proc_name
	);

#endif /* __FLT_TASK_I_H__ */
