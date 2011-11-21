//=============================================================================================
/*
	GUI Resurce, GuiRes.h

	Project	: Xfilter 1.0
	Author	: Tony Zhu
	Create Date	: 2001/08/03
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 Passeck Technology.
	All Rights Reserved.

	WARNNING: 
*/

#ifndef	GUIRES_H
#define GUIRES_H

#define LANGUAGE_CHINESE
#ifdef LANGUAGE_CHINESE

#define GUI_LANGUAGE				_T("RPC")
#ifdef ZIGUANG
#define GUI_VERSION					_T("רҵ��")
#else
#define GUI_VERSION					_T("��׼��")
#endif // ZIGUANG
#define USER_REGISTER_QUESTION		_T("�Ѷ����˷���ǽ���Ŀǰ��û��ע�ᣬ����Ҫ���ע����\n\n\n")
#define USER_REGISTER_ALREDAY		_T("�Ѿ�ע�����Ҫ�޸���ǰ��ע����Ϣ������ע����\n\n\n")
#define UPDATE_VERSION_ALREDAY_NEW	_T("������Ѿ������°汾��������¡�")
#define UPDATE_VERSION_NEW			_T("��ǰ�汾��%s�����°汾��%s����ӭ�������°汾��")
#define UPDATE_VERSION_CANNOT		_T("�������ӵ����硣")

static TCHAR* MON_BUTTON_SCROOL_CAPTION[] = {
	_T("�Զ�����"),
	_T("ֹͣ����"),
};
static TCHAR* MON_BUTTON_MONITOR_CAPTION[] = {
	_T("��ʼ����"),
	_T("ֹͣ����"),
};

#define MON_STATUS_SPACE		_T("                    ")
#define MON_STATUS_FORMAT		_T("״̬��%s��%s")
static TCHAR* MON_STATUS_MONITOR[] = {
	_T("������"),
	_T("����"),
};
static TCHAR* MON_STATUS_SCROLL[] = {
	_T("������"),
	_T("����"),
};


#define XFILTER_SERVICE_DLL_NAME	_T("XFILTER.DLL")
#define XFILTER_HELP_FILE_NAME		_T("XFILTER.CHM")
#define XFILTER_MEMORY_ACL_FILE		_T("XFILTER_MEMORY_ACL_FILE")

#ifdef LANGUAGE_CHINESE
#ifdef ZIGUANG
#define WINDOW_CAPTION				_T("�Ѷ����˷���ǽ - רҵ��")
#else
#define WINDOW_CAPTION				_T("�Ѷ����˷���ǽ - ��׼��")
#endif // ZIGUANG
#else
#define WINDOW_CAPTION				_T("XFILTER PERSONAL FIREWALL 2")
#endif

#define BUTTON_CAPTION_HIDE			_T("����")
#define BUTTON_CAPTION_CLOSE		_T("�ر�")
#define BUTTON_CAPTION_MIN			_T("��С��")
#define BUTTON_CAPTION_TOPMOST		_T("���ڶ���")

#define BUTTON_CAPTION_GREEN		_T("ȫ������")
#define BUTTON_CAPTION_YELLOW		_T("����")
#define BUTTON_CAPTION_RED			_T("ȫ���ܾ�")
static TCHAR* WORK_MODE_STATUS[] = {
	BUTTON_CAPTION_GREEN,
	BUTTON_CAPTION_YELLOW,
	BUTTON_CAPTION_RED,
};

#define ACL_CAPTION_ADD				_T("���ӿعܹ���")
#define ACL_CAPTION_EDIT			_T("�޸Ŀعܹ���")
#define ACL_CAPTION_APP_ADD			_T("����Ӧ�ó���عܹ���")
#define ACL_CAPTION_APP_EDIT		_T("�޸�Ӧ�ó���عܹ���")
#define ACL_CAPTION_WEB_ADD			_T("������վ���˹���")
#define ACL_CAPTION_WEB_EDIT		_T("�޸���վ���˹���")
#define ACL_CAPTION_NNB_ADD			_T("���������ھӿعܹ���")
#define ACL_CAPTION_NNB_EDIT		_T("�޸������ھӿعܹ���")
#define ACL_CAPTION_ICMP_ADD		_T("����ICMP�عܹ���")
#define ACL_CAPTION_ICMP_EDIT		_T("�޸�ICMP�عܹ���")
#define ACL_CAPTION_NET_ADD			_T("����IP��ַ�μ�¼")
#define ACL_CAPTION_NET_EDIT		_T("�޸�IP��ַ�μ�¼")

#define ACL_QUERY_CAPTION_APP		_T("Ӧ�ó������")
#define ACL_QUERY_CAPTION_APP_DRV	_T("Ӧ�ó������")
#define ACL_QUERY_CAPTION_WEB		_T("վ�����")
#define ACL_QUERY_CAPTION_NNB		_T("�����ھӹ���")
#define ACL_QUERY_CAPTION_ICMP		_T("ICMP ����")

#define ACL_QUERY_CAPTION_INFO_APP			_T("�Ա�������������δ���")
#define ACL_QUERY_CAPTION_INFO_APP_DRV		_T("���������Ѿܾ����Ժ�Դ�������������δ���")
#define ACL_QUERY_CAPTION_INFO_WEB			_T("�Ա�������������δ���")
#define ACL_QUERY_CAPTION_INFO_NNB			_T("���������Ѿܾ����Ժ�Դ�������������δ���")
#define ACL_QUERY_CAPTION_INFO_ICMP			_T("���������Ѿܾ����Ժ�Դ�������������δ���")


static TCHAR *ACL_CAPTION[] = {
	_T("�ܿ����� - Ӧ�ó���عܹ���"),
	_T("�ܿ����� - ��վ���˹���"),
	_T("�ܿ����� - �����ھӿعܹ���"),
	_T("�ܿ����� - ICMP �عܹ���"),
	_T("�ܿ����� - ľ����"),
	_T("�ܿ����� - ʱ����������"),
	_T("�ܿ����� - ������������")
};
#define ACL_BUTTON_COUNT	sizeof(ACL_CAPTION)/sizeof(TCHAR*)


static TCHAR *ACL_TORJAN_STATUS[] = {
	_T("��ֹͣ"),
	_T("������")
};

static TCHAR *ACL_TORJAN_BUTTON[] = {
	_T("����"),
	_T("ֹͣ")
};

static TCHAR *ACL_FILE_BUTTON[] = {
	_T("����"),
	_T("ֹͣ")
};

static TCHAR *ACL_QUERY_TEXT[] = {
	_T("����"),
	_T("�ܾ�"),
	_T("ѯ��")
};
#define ACL_QUERY_TEXT_COUNT		sizeof(ACL_QUERY_TEXT)/sizeof(TCHAR*)

#define QUERY_LIST_LABEL_WEB		_T("վ�㣺")
#define QUERY_LIST_LABEL_APP		_T("Ӧ�ó���")
#define QUERY_LIST_LABEL_PROTOCOL	_T("Э�飺")
#define QUERY_LIST_LABEL_DIRECTION	_T("���߷���")
#define QUERY_LIST_LABEL_LOCAL		_T("����IP/�˿ڣ�")
#define QUERY_LIST_LABEL_REMOTE		_T("Ŀ��IP/�˿ڣ�")
#define QUERY_LIST_LABEL_TIME		_T("ʱ�䣺")
#define QUERY_LIST_LABEL_HOST		_T("����/Զ��������")

#define CHAR_WIDTH		10
#define MEMO_CONST		_T("��")
static TCHAR *ACL_APP_LIST_HEADER[] = {
	_T("���"),
	_T("Ӧ�ó���"),
	_T("����"),
	_T("Ŀ������"),
	_T("Ŀ�Ķ˿�"),
	_T("���ض˿�(����)"),
	_T("����"),
	_T("Э��"),
	_T("ʱ������"),
	_T("��ע"),
	_T("Ӧ�ó���·��"),
};
static int ACL_APP_LIST_LENTH[] = {
	_tcslen(ACL_APP_LIST_HEADER[0]) * CHAR_WIDTH,
	_tcslen(ACL_APP_LIST_HEADER[1]) * CHAR_WIDTH - 20,
	_tcslen(ACL_APP_LIST_HEADER[2]) * CHAR_WIDTH,
	_tcslen(ACL_APP_LIST_HEADER[3]) * CHAR_WIDTH - 10,
	_tcslen(ACL_APP_LIST_HEADER[4]) * CHAR_WIDTH - 35,
	_tcslen(ACL_APP_LIST_HEADER[5]) * CHAR_WIDTH - 100,
	_tcslen(ACL_APP_LIST_HEADER[6]) * CHAR_WIDTH,
	_tcslen(ACL_APP_LIST_HEADER[7]) * CHAR_WIDTH + 10,
	_tcslen(ACL_APP_LIST_HEADER[8]) * CHAR_WIDTH - 10,
	_tcslen(ACL_APP_LIST_HEADER[9]) * CHAR_WIDTH + 200,
	_tcslen(ACL_APP_LIST_HEADER[10]) * CHAR_WIDTH + 200,
};
static DWORD ACL_APP_LIST_LENTH_EX[] = {0xA9C9E7B1,0xD1D4FACC,0x212A4429,0xFCB7D2B8,0xACC9B3B1,0xEAD2B6CB,0x0000D3CB};
#define ACL_APP_LIST_COUNT	sizeof(ACL_APP_LIST_HEADER)/sizeof(TCHAR*)

static TCHAR *ACL_WEB_LIST_HEADER[] = {
	_T("���"),
	_T("վ��"),
	_T("����"),
	_T("��ע")
};
static int ACL_WEB_LIST_LENTH[] = {
	_tcslen(ACL_WEB_LIST_HEADER[0]) * CHAR_WIDTH,
	_tcslen(ACL_WEB_LIST_HEADER[1]) * CHAR_WIDTH + 160,
	_tcslen(ACL_WEB_LIST_HEADER[2]) * CHAR_WIDTH,
	_tcslen(ACL_WEB_LIST_HEADER[3]) * CHAR_WIDTH + 100
};
#define ACL_WEB_LIST_COUNT	sizeof(ACL_WEB_LIST_HEADER)/sizeof(TCHAR*)

#define ACL_NNB_COMSTOM			_T("�Զ���")
static TCHAR *ACL_NNB_LIST_HEADER[] = {
	_T("���"),
	_T("�����ھ�"),
	_T("���߷���"),
	_T("ʱ������"),
	_T("����"),
	_T("��ע")
};
static int ACL_NNB_LIST_LENTH[] = {
	_tcslen(ACL_NNB_LIST_HEADER[0]) * CHAR_WIDTH,
	_tcslen(ACL_NNB_LIST_HEADER[1]) * CHAR_WIDTH + 30,
	_tcslen(ACL_NNB_LIST_HEADER[2]) * CHAR_WIDTH - 10,
	_tcslen(ACL_NNB_LIST_HEADER[3]) * CHAR_WIDTH,
	_tcslen(ACL_NNB_LIST_HEADER[4]) * CHAR_WIDTH,
	_tcslen(ACL_NNB_LIST_HEADER[5]) * CHAR_WIDTH + 40
};
#define ACL_NNB_LIST_COUNT	sizeof(ACL_NNB_LIST_HEADER)/sizeof(TCHAR*)

static TCHAR *ACL_ICMP_LIST_HEADER[] = {
	_T("���"),
	_T("Զ������"),
	_T("���߷���"),
	_T("ʱ������"),
	_T("����"),
	_T("��ע")
};
static int ACL_ICMP_LIST_LENTH[] = {
	_tcslen(ACL_ICMP_LIST_HEADER[0]) * CHAR_WIDTH,
	_tcslen(ACL_ICMP_LIST_HEADER[1]) * CHAR_WIDTH + 30,
	_tcslen(ACL_ICMP_LIST_HEADER[2]) * CHAR_WIDTH - 10,
	_tcslen(ACL_ICMP_LIST_HEADER[3]) * CHAR_WIDTH,
	_tcslen(ACL_ICMP_LIST_HEADER[4]) * CHAR_WIDTH,
	_tcslen(ACL_ICMP_LIST_HEADER[5]) * CHAR_WIDTH + 40
};
#define ACL_ICMP_LIST_COUNT	sizeof(ACL_ICMP_LIST_HEADER)/sizeof(TCHAR*)

static TCHAR *ACL_TIME_TYPE[] = {
	_T("�κ�ʱ��"),
	_T("����ʱ��"),
	_T("������ҵ��ʱ��"),
	_T("��ĩ"),
	_T("Լ��ʱ��"),
	_T("����ʱ��"),
	_T("�Զ���ʱ��")
};
#define ACL_TIME_TYPE_COUNT	sizeof(ACL_TIME_TYPE)/sizeof(TCHAR*)

static TCHAR *ACL_NET_TYPE[] = {
	_T("��������"),
	_T("������"),
	_T("��Լ��������"),
	_T("���ε�����"),
	_T("�Զ��������")
};
#define ACL_NET_TYPE_COUNT	sizeof(ACL_NET_TYPE)/sizeof(TCHAR*)

static TCHAR *ACL_IP_LIST_HEADER[] = {
	_T("���"),
	_T("��ʼIP"),
	_T("����IP")
};
static int ACL_IP_LIST_LENTH[] = {
	_tcslen(ACL_IP_LIST_HEADER[0]) * CHAR_WIDTH,
	_tcslen(ACL_IP_LIST_HEADER[1]) * CHAR_WIDTH + 60,
	_tcslen(ACL_IP_LIST_HEADER[2]) * CHAR_WIDTH + 60
};

#define MON_BUTTON_APP		0
#define MON_BUTTON_NNB		1
#define MON_BUTTON_ICMP		2
#define MON_BUTTON_PORT		3
#define MON_BUTTON_LINE		4

#define MON_BUTTON_CLEAR	0
#define MON_BUTTON_SCROLL	1
#define MON_BUTTON_MONITOR	2

static TCHAR *MONITOR_CAPTION[] = {
	_T("�������� - Ӧ�ó���������"),
	_T("�������� - �����ھӷ������"),
	_T("�������� - ICMP �������"),
	_T("�������� - �����˿�"),
	_T("�������� - ��ǰ����")
};

static TCHAR *LISTEN_HEADER[] = {
	_T("���"),
	_T("Ӧ�ó���"),
	_T("Э��"),
	_T("�����˿�"),
	_T("��ʼʱ��/����ʱ��"),
};
static int LISTEN_HEADER_LENTH[] = {
	_tcslen(LISTEN_HEADER[0]) * CHAR_WIDTH + 20,
	_tcslen(LISTEN_HEADER[1]) * CHAR_WIDTH + 20,
	_tcslen(LISTEN_HEADER[2]) * CHAR_WIDTH + 20,
	_tcslen(LISTEN_HEADER[3]) * CHAR_WIDTH - 20,
	_tcslen(LISTEN_HEADER[4]) * CHAR_WIDTH - 50,
};
#define LISTEN_HEADER_COUNT sizeof(LISTEN_HEADER)/sizeof(TCHAR*)

static TCHAR *ONLINE_HEADER[] = {
	_T("���"),
	_T("Ӧ�ó���"),
	_T("Ŀ��IP/�˿�"),
	_T("����/����"),
	_T("Э��/����"),
	_T("��ʼʱ��/����ʱ��"),
	_T("����IP/�˿�"),
};
static int ONLINE_HEADER_LENTH[] = {
	_tcslen(ONLINE_HEADER[0]) * CHAR_WIDTH + 20,
	_tcslen(ONLINE_HEADER[1]) * CHAR_WIDTH - 20,
	_tcslen(ONLINE_HEADER[2]) * CHAR_WIDTH,
	_tcslen(ONLINE_HEADER[3]) * CHAR_WIDTH - 20,
	_tcslen(ONLINE_HEADER[4]) * CHAR_WIDTH - 20,
	_tcslen(ONLINE_HEADER[5]) * CHAR_WIDTH - 50,
	_tcslen(ONLINE_HEADER[6]) * CHAR_WIDTH + 20,
};
#define ONLINE_HEADER_COUNT sizeof(ONLINE_HEADER)/sizeof(TCHAR*)

static TCHAR* SEND_OR_RECV[] = {
	_T("RECV"),
	_T("SEND"),
	_T("RDSD")
};

static TCHAR *MONITOR_APP_HEADER[] = {
	_T("����"),
	_T("Ӧ�ó���"),
	_T("ʱ��"),
	_T("Ŀ��IP/�˿�"),
	_T("����/����"),
	_T("Э��/����"),
	_T("����IP/�˿�"),
	_T("��ע"),
};
static int MONITOR_APP_HEADER_LENTH[] = {
	_tcslen(MONITOR_APP_HEADER[0]) * CHAR_WIDTH,
	_tcslen(MONITOR_APP_HEADER[1]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_APP_HEADER[2]) * CHAR_WIDTH + 20,
	_tcslen(MONITOR_APP_HEADER[3]) * CHAR_WIDTH + 20,
	_tcslen(MONITOR_APP_HEADER[4]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_APP_HEADER[5]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_APP_HEADER[6]) * CHAR_WIDTH + 20,
	_tcslen(MONITOR_APP_HEADER[7]) * CHAR_WIDTH + 200,
};
#define MONITOR_APP_HEADER_COUNT sizeof(MONITOR_APP_HEADER)/sizeof(TCHAR*)

static TCHAR *MONITOR_NNB_HEADER[] = {
	_T("����"),
	_T("Ӧ�ó���"),
	_T("��������/�˿�"),
	_T("Զ������/�˿�"),
	_T("״̬/����"),
	_T("Э��/����"),
	_T("ʱ��"),
	_T("��ע"),
};
static int MONITOR_NNB_HEADER_LENTH[] = {
	_tcslen(MONITOR_NNB_HEADER[0]) * CHAR_WIDTH,
	_tcslen(MONITOR_NNB_HEADER[1]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_NNB_HEADER[2]) * CHAR_WIDTH - 40,
	_tcslen(MONITOR_NNB_HEADER[3]) * CHAR_WIDTH - 40,
	_tcslen(MONITOR_NNB_HEADER[4]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_NNB_HEADER[5]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_NNB_HEADER[6]) * CHAR_WIDTH + 20,
	_tcslen(MONITOR_NNB_HEADER[7]) * CHAR_WIDTH + 200,
};
#define MONITOR_NNB_HEADER_COUNT sizeof(MONITOR_NNB_HEADER)/sizeof(TCHAR*)

static TCHAR *MONITOR_ICMP_HEADER[] = {
	_T("����"),
	_T("Ӧ�ó���"),
	_T("ʱ��"),
	_T("״̬/����"),
	_T("����"),
	_T("��ע"),
};
static int MONITOR_ICMP_HEADER_LENTH[] = {
	_tcslen(MONITOR_ICMP_HEADER[0]) * CHAR_WIDTH,
	_tcslen(MONITOR_ICMP_HEADER[1]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_ICMP_HEADER[2]) * CHAR_WIDTH + 20,
	_tcslen(MONITOR_ICMP_HEADER[3]) * CHAR_WIDTH - 20,
	_tcslen(MONITOR_ICMP_HEADER[4]) * CHAR_WIDTH,
	_tcslen(MONITOR_ICMP_HEADER[5]) * CHAR_WIDTH + 500,
};
#define MONITOR_ICMP_HEADER_COUNT sizeof(MONITOR_ICMP_HEADER)/sizeof(TCHAR*)

#define GUI_ACL_ACTION_0					_T("����")
#define GUI_ACL_ACTION_1					_T("�ܾ�")
#define GUI_ACL_ACTION_2					_T("ѯ��")
#define GUI_ACL_DERECTION_0					_T("����")
#define GUI_ACL_DERECTION_1					_T("����")
#define GUI_ACL_DERECTION_2					_T("˫��")
#define GUI_ACL_DERECTION_3					_T("�㲥")
#define GUI_ACL_DERECTION_4					_T("����")
#define GUI_ACL_SERVICE_TYPE_0				_T("�κ�")
#define GUI_ACL_SERVICE_TYPE_1				_T("TCP")
#define GUI_ACL_SERVICE_TYPE_2				_T("UDP")
#define GUI_ACL_SERVICE_TYPE_3				_T("FTP")
#define GUI_ACL_SERVICE_TYPE_4				_T("TELNET")
#define GUI_ACL_SERVICE_TYPE_5				_T("HTTP")
#define GUI_ACL_SERVICE_TYPE_6				_T("NNTP")
#define GUI_ACL_SERVICE_TYPE_7				_T("POP3")
#define GUI_ACL_SERVICE_TYPE_8				_T("SMTP")
#define GUI_ACL_SERVICE_TYPE_9				_T("ICMP")

static TCHAR *GUI_ACTION[]		= {
	GUI_ACL_ACTION_0,
	GUI_ACL_ACTION_1,
	GUI_ACL_ACTION_2
};
#define GUI_ACTION_COUNT	sizeof(GUI_ACTION)/sizeof(TCHAR*)

static TCHAR *GUI_DIRECTION[]	= {
	GUI_ACL_DERECTION_0,
	GUI_ACL_DERECTION_1,
	GUI_ACL_DERECTION_2,
	GUI_ACL_DERECTION_3,
	GUI_ACL_DERECTION_4,
};
#define GUI_DIRECTION_COUNT	3//sizeof(GUI_DIRECTION)/sizeof(TCHAR*)

static TCHAR *GUI_SERVICE_TYPE[]= {
	 GUI_ACL_SERVICE_TYPE_0,	
	 GUI_ACL_SERVICE_TYPE_1,						
	 GUI_ACL_SERVICE_TYPE_2,						
	 GUI_ACL_SERVICE_TYPE_3,						
	 GUI_ACL_SERVICE_TYPE_4,						
	 GUI_ACL_SERVICE_TYPE_5,						
	 GUI_ACL_SERVICE_TYPE_6,						
	 GUI_ACL_SERVICE_TYPE_7,						
	 GUI_ACL_SERVICE_TYPE_8,
	 GUI_ACL_SERVICE_TYPE_9
};
static TCHAR* GUI_SERVICE_PORT[] = {
	_T("0"),
	_T("0"),
	_T("0"),
	_T("21"),
	_T("23"),
	_T("80"),
	_T("119"),
	_T("110"),
	_T("25")
};
static UINT GUI_SERVICE_PORT_NUM[] = {
	0,
	0,
	0,
	21,
	23,
	80,
	119,
	110,
	25
};
static BOOL GUI_SERVICE_ENABLE[] = {
	TRUE,
	TRUE,
	TRUE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE
};
#define GUI_SERVICE_TYPE_COUNT	sizeof(GUI_SERVICE_TYPE)/sizeof(TCHAR*) - 1

#define GUI_ACL_EXPLAIN_CONST			_T("'*'��������Ӧ�ó���0�������ж˿ڡ�")
#define GUI_ACL_EXPLAIN_PORT_ALL		_T("����")
#define GUI_ACL_EXPLAIN_OUT_FORMAT		_T("%s%sͨ��%sЭ���%s�˿ڷ���%s��%s�˿ڽ���%s��%s")
#define GUI_ACL_EXPLAIN_IN_FORMAT		_T("%s%sͨ��%sЭ���%s�˿��򱾻�%s%s�˿ڷ������������󽫱�%s��%s")
#define GUI_QUERY_EXPLAIN_APP_OUT		_T("%s %s ͨ�� %s Э��� %u �˿ڷ��� %s �� %u �˿ڣ��Ƿ���У�")
#define GUI_QUERY_EXPLAIN_APP_IN		_T("%s %s ͨ�� %s Э��� %u �˿��򱾻� %s %u �˿ڷ������������Ƿ���У�")
#define GUI_QUERY_EXPLAIN_WEB			_T("Ӧ�ó��� %s Ҫ����վ�� %s���Ƿ���У�")

static TCHAR* GUI_QUERY_EXPLAIN_NNB[] = {
	_T("%s Ҫ���ʱ����Ĺ�����Դ���Ƿ���У�"),
	_T("����Ҫ���� %s �Ĺ�����Դ���Ƿ���У�"),
};

static TCHAR* GUI_QUERY_EXPLAIN_ICMP[] = {
	_T("Զ������ %s �򱾻����� ICMP(Ping) �����Ƿ���У�"),
	_T("������ %s ���� ICMP(Ping) �����Ƿ���У�"),
};



#define ERROR_STRING_NOCODE					_T("δ֪�Ĵ���")
#define ERROR_STRING_SUCCESS				_T("�����ɹ�")
#define ERROR_STRING_FILE_NOT_FOUND			_T("û���ҵ�ָ�����ļ�")
#define ERROR_STRING_FILE_ALREDAY_EXISTS	_T("�ļ��Ѿ�����")
#define ERROR_STRING_FILE_LOCKED			_T("�ļ�������")
#define ERROR_STRING_FILE_CREATE_FAILURE	_T("�����ļ�ʧ��")
#define ERROR_STRING_FILE_CAN_NOT_OPEN		_T("���ܹ����ļ�")
#define ERROR_STRING_FILE_INVALID_SIGNATURE	_T("��Ч���ļ���ʶ")
#define ERROR_STRING_FILE_READ_ERROR		_T("�ļ���ȡ����")
#define ERROR_STRING_FILE_SAVE_ERROR		_T("�ļ��������")
#define ERROR_STRING_FILE_ADD_ERROR			_T("���Ӽ�¼����")
#define ERROR_STRING_FILE_GET_STATUS_ERROR	_T("�õ��ļ�״̬����ʧ��")
#define ERROR_STRING_FILE_READ_ONLY			_T("�ļ�Ϊֻ������")
#define ERROR_STRING_FILE_WRITER_HEADER_ERROR	_T("�����ļ�ͷ����")
#define ERROR_STRING_FILE_RECORD_CAN_NOT_FIND	_T("û�з���ָ���ļ�¼")
#define ERROR_STRING_FILE_INVALID_PARAMETER		_T("��Ч�Ĳ���")
#define ERROR_STRING_FILE_INVALID_VERSION		_T("�ļ��汾��Ч")

#define ERROR_STRING_CAN_NOT_READ_PACKET		_T("���ܻ�ȡ�������������װ��û������������������������\n\n\n")
#define ERROR_STRING_HAVE_NOT_INSTALL			_T("û�а�װ�Ѷ����˷���ǽ����Ҫ���°�װ��\n\n\n")
#define ERROR_STRING_DRIVER_NOT_FOUNT			_T("�Ѷ����˷���ǽ���ܼ���������������������°�װ��\n\n\n")

//---------------------------------------------------------------------------------------------
//app's resource

#define GUI_APP_CLASS_NAME			_T("Xfilter")
#define	GUI_APP_CAPTION				_T("�Ѷ����˷���ǽ")
#define	GUI_APP_LOGO				_T("XSTUDIO")

//---------------------------------------------------------------------------------------------
//menu's resource

#define GUI_MENU_FILE				_T("�ļ�(&F)")
#define GUI_MENU_ABOUT				_T("����(&A)")
#define GUI_MENU_CONTROL_FRAME		_T("������(&C)")
#define GUI_MENU_PACKET_MONITOR		_T("�������(&M)")
#define GUI_MENU_LOG_QUERY			_T("��־��ѯ(&Q)")
#define GUI_MENU_ACL				_T("�عܹ���(&L)")
#define GUI_MENU_SYSTEM_SET			_T("ϵͳ����(&S)")
#define GUI_MENU_STOP_FILTER		_T("ֹͣ����(&T)")
#define GUI_MENU_START_FILTER		_T("��ʼ����(&F)")
#define GUI_MENU_EXIT				_T("�˳�(&X)")

//---------------------------------------------------------------------------------------------
//button's resource

#define GUI_BUTTON_OK				_T("ȷ��")
#define GUI_BUTTON_CANCEL			_T("ȡ��")
#define GUI_BUTTON_CANCEL_EX		_T("ȡ��(&C)")
#define GUI_BUTTON_APPLY			_T("Ӧ��(&A)")
#define GUI_BUTTON_HELP				_T("����")

//---------------------------------------------------------------------------------------------
//tab's resource

#define GUI_TAB_PACKET_MONITOR		_T("�������")
#define GUI_TAB_LOG_QUERY			_T("��־��ѯ")
#define GUI_TAB_ACL					_T("�عܹ���")
#define GUI_TAB_SYSTEM_SETTING		_T("ϵͳ����")
#define GUI_TAB_ABOUT				_T("����")

//---------------------------------------------------------------------------------------------
//packet monitor resource

#define	GUI_PACKET_MONITOR_TITLE						_T("�������")
#define GUI_PACKET_MONITOR_LABLE						_T("��������б�")
#define GUI_PACKET_MONITOR_TOOLBAR_CLEAR				_T("���")
#define GUI_PACKET_MONITOR_TOOLBAR_STOP_SCROLL			_T("ֹͣ����")
#define GUI_PACKET_MONITOR_TOOLBAR_START_SCROLL			_T("�Զ�����")
#define GUI_PACKET_MONITOR_TOOLBAR_STOP_MONITOR			_T("ֹͣ����")
#define GUI_PACKET_MONITOR_TOOLBAR_START_MONITOR		_T("��ʼ����")
#define GUI_PACKET_MONITOR_LIST_ACTION					_T("����")
#define GUI_PACKET_MONITOR_LIST_ACTION_SIZE				\
		sizeof(GUI_PACKET_MONITOR_LIST_ACTION)			* 8
#define GUI_PACKET_MONITOR_LIST_STARTTIME_ENDTIME		_T("��ʼʱ�� - ����ʱ��")
#define GUI_PACKET_MONITOR_LIST_STARTTIME_ENDTIME_SIZE	\
		sizeof(GUI_PACKET_MONITOR_LIST_STARTTIME_ENDTIME) * 8
#define GUI_PACKET_MONITOR_LIST_PROTOCOL				_T("Э��")
#define GUI_PACKET_MONITOR_LIST_PROTOCOL_SIZE		\
		sizeof(GUI_PACKET_MONITOR_LIST_PROTOCOL)		* 8
#define GUI_PACKET_MONITOR_LIST_IN_DATA					_T("������")
#define GUI_PACKET_MONITOR_LIST_IN_DATA_SIZE			\
		sizeof(GUI_PACKET_MONITOR_LIST_IN_DATA)			* 8
#define GUI_PACKET_MONITOR_LIST_OUT_DATA				_T("������")
#define GUI_PACKET_MONITOR_LIST_OUT_DATA_SIZE			\
		sizeof(GUI_PACKET_MONITOR_LIST_OUT_DATA)		* 8
#define GUI_PACKET_MONITOR_LIST_IP_PORT					_T("���� IP : �˿� - Զ�� IP : �˿�")
#define GUI_PACKET_MONITOR_LIST_IP_PORT_SIZE			\
		sizeof(GUI_PACKET_MONITOR_LIST_IP_PORT)			* 8
#define GUI_PACKET_MONITOR_LIST_APPLICATION				_T("Ӧ�ó��� : ��ע")
#define GUI_PACKET_MONITOR_LIST_APPLICATION_SIZE		\
		sizeof(GUI_PACKET_MONITOR_LIST_APPLICATION)		* 8

//---------------------------------------------------------------------------------------------
//log query resource

#define	GUI_LOG_QUERY_SET_LABLE				_T("��ѯ��������")
#define GUI_LOG_QUERY_SET_START_TIME_LABLE	_T("��ʼ����/ʱ�䣺")
#define GUI_LOG_QUERY_SET_END_TIME_LABLE	_T("��������/ʱ�䣺")
#define GUI_LOG_QUERY_RESULT_LABLE			_T("��ѯ���")
#define GUI_LOG_QUERY_RESULT_LIST_LABLE		_T("��%uҳ,��%uҳ,��ǰ%u��,��%u��")
#define GUI_LOG_QUERY_BUTTON_QUERY			_T("��ʼ��ѯ(&Q)")
#define GUI_LOG_QUERY_BUTTON_BACK			_T("��һҳ(&B)")
#define GUI_LOG_QUERY_BUTTON_NEXT			_T("��һҳ(&N)")
#define GUI_LOG_QUERY_BUTTON_DELETE			_T("���(&D)")

//---------------------------------------------------------------------------------------------
//ACL RESOURCE

#define GUI_ACL_APPLICATION_SET_LABLE		_T("Ӧ�ó�������")
#define GUI_ACL_RADIO_PASS					_T("��������")
#define GUI_ACL_RADIO_QUERY					_T("ѯ��")
#define GUI_ACL_RADIO_DENY					_T("�ܾ�����")
#define GUI_ACL_SET_LABLE					_T("�عܹ�������")
#define GUI_ACL_BUTTON_ADD					_T("���(&A)")
#define GUI_ACL_BUTTON_EDIT					_T("�޸�(&E)")
#define GUI_ACL_BUTTON_DELETE				_T("ɾ��(&D)")

//---------------------------------------------------------------------------------------------
//ACL SET RESOURCE

//---------------------------------------------------------------------------------------------
//net and time set sheet resource

#define GUI_NET_TIME_SHEET_CAPTION			_T("������ʱ������")

//---------------------------------------------------------------------------------------------
//set time resource

#define GUI_SET_TIME_WEEK_TIME_LABLE		_T("������ʱ��")
#define GUI_SET_TIME_CHECK_SUNDAY			_T("������")
#define GUI_SET_TIME_CHECK_MONDAY			_T("����һ")
#define GUI_SET_TIME_CHECK_TUESDAY			_T("���ڶ�")
#define GUI_SET_TIME_CHECK_WEDNESDAY		_T("������")
#define GUI_SET_TIME_CHECK_THURSDAY			_T("������")
#define GUI_SET_TIME_CHECK_FRIDAY			_T("������")
#define GUI_SET_TIME_CHECK_SATURDAY			_T("������")
#define GUI_SET_TIME_TIME_LABLE				_T("ʱ�䣺")
#define GUI_SET_TIME_LABLE_START_TIME		_T("��ʼʱ��")
#define GUI_SET_TIME_LABLE_END_TIME			_T("����ʱ��")
#define GUI_SET_TIME_TREE_0					_T("�κ�ʱ��")
#define GUI_SET_TIME_TREE_1					_T("����ʱ��")
#define GUI_SET_TIME_TREE_2					_T("������ҵ��ʱ��")
#define GUI_SET_TIME_TREE_3					_T("��ĩ")
#define GUI_SET_TIME_TREE_4					_T("Լ��ʱ��")
#define GUI_SET_TIME_TREE_5					_T("����ʱ��")
#define GUI_SET_TIME_TREE_6					_T("�Զ���ʱ��")

//---------------------------------------------------------------------------------------------
//net ip aria resource

#define GUI_NET_IP_ARIA_CAPTION				_T("IP ��ַ��")
#define GUI_NET_IP_ARIA_LABLE				_T("Զ������ IP ��ַ��")
#define GUI_NET_IP_ARIA_LABLE_START_IP		_T("��ʼ IP��")
#define GUI_NET_IP_ARIA_LABLE_END_IP		_T("���� IP��")
#define GUI_NET_IP_ARIA_ALL					_T("*")

//---------------------------------------------------------------------------------------------
//set remote net resource

#define GUI_SET_NET_LABLE					_T("Զ��������Ϣ")
#define GUI_SET_NET_NET_LABLE				_T("������Ϣ")
#define GUI_SET_NET_BUTTON_ADD				_T("���(&A)")
#define GUI_SET_NET_BUTTON_EDIT				_T("�޸�(&E)")
#define GUI_SET_NET_BUTTON_DELETE			_T("ɾ��(&D)")
#define GUI_SET_NET_LIST_START_IP			_T("��ʼIP")
#define GUI_SET_NET_LIST_END_IP				_T("����IP")
#define GUI_SET_NET_TREE_0					_T("��������")
#define GUI_SET_NET_TREE_1					_T("������")
#define GUI_SET_NET_TREE_2					_T("��Լ��������")
#define GUI_SET_NET_TREE_3					_T("���ε�����")
#define GUI_SET_NET_TREE_4					_T("�Զ��������")

//---------------------------------------------------------------------------------------------
//system set resource

#define GUI_SYSTEM_SET_COMMON_SET_LABLE		_T("��������")
#define GUI_SYSTEM_SET_CHECK_LOG			_T("��¼��־����־�ļ���СΪ")
#define GUI_SYSTEM_SET_UNIT_LABLE			_T("M��")
#define GUI_SYSTEM_SET_CHECK_AUTOSTART		_T("Windows ����ʱ�Զ����� Xfilter��")
#define GUI_SYSTEM_SET_CHECK_SPLASH			_T("Xfilter ����ʱ��ʾ��ӭ���档")
#define GUI_SYSTEM_SET_ALERT_SET_LABLE		_T("��������")
#define GUI_SYSTEM_SET_CHECK_ALERT_PCSPEAKER	_T("����ʱ PC ���ȷ�������������")
#define GUI_SYSTEM_SET_CHECK_ALERT_DIALOG	_T("����ʱ��˸�������ϵ�ͼ�ꡣ")

//---------------------------------------------------------------------------------------------
//about resource

#define GUI_ABOUT_LABLE_TITLE			_T("�Ѷ����˷���ǽ")
#define GUI_ABOUT_LABLE_COPYRIGHT1		_T("�汾��%u.%u.%u (%s)")
#define GUI_ABOUT_LABLE_COPYRIGHT2		_T("Copyright (C) 2002-2003 Passeck Technology")
#define GUI_ABOUT_LABLE_COPYRIGHT3		_T("All rights reserved.")
#define GUI_ABOUT_LABLE_ACCREDIT_TO		_T("�������Ȩ�� %s ʹ��")
#define GUI_ABOUT_LABLE_WEB_ADDRESS_LABLE	_T("��վ��ַ��")
#define GUI_ABOUT_LABLE_EMAIL_LABLE			_T("�����ʼ���")
#define GUI_ABOUT_LABLE_WEB_ADDRESS		_T("http://www.xfilt.com/")
#define GUI_ABOUT_LABLE_EMAIL			_T("mailto:xstudio@xfilt.com")
#define GUI_ABOUT_LABLE_WARNING			_T("���棺�����������Ȩ���ı������������������ɴ��������������ƻ�����������Ի���Ϊӯ��Ŀ�ģ���δ����Ȩ������Ӫ���������һ����ҵ��Ϊ�����⵽�����⳥�����µĴ�������������������̷�����׷�ߡ�")
#define GUI_ABOUT_LABLE_INFO			_T("���������Դ���룬��ϸ��Ϣ������������ַ����վ���г��˲�Ʒ�۸�͸��ʽ����ֱ�ӷ��� Email ������ѯ��")
#define GUI_ABOUT_LABLE_AUTHOR			_T("���ߣ������")

//---------------------------------------------------------------------------------------------
// Register Dialog Resource

#define GUI_REG_CAPTION				_T("Xfilter �û�ע��")
#define GUI_REG_LABEL_INFO_1		_T("���������Դ���룬��ϸ��Ϣ������������ַ�������������ȫ��ѵģ�ֻҪ���� Email ��ַ�����ͳ�Ϊ������ĺϷ��û���")
#define GUI_REG_LABEL_INFO_3		_T("��ȷ�� Email ����ʵ�ԣ��⽫��Ϊ����֧�ֵ���Ҫ;����")
#define GUI_REG_LABEL_INFO_2		_T("ע��������Ϣ���������ڵ�һʱ��õ���������������򣬲��п��ܳ�Ϊ�����û���ѻ��Դ���롣 ")
#define GUI_REG_LABEL_EMAIL			_T("Email��*")
#define GUI_REG_LABEL_NAME			_T("������")
#define GUI_REG_LABEL_GENDER		_T("�Ա�")
#define GUI_REG_LABEL_ID_TYPE		_T("֤�����ͣ�")
#define GUI_REG_LABEL_ID			_T("֤�����룺")
#define GUI_REG_LABEL_BIRTHDAY		_T("���գ�")
#define GUI_REG_LABEL_DEGREE		_T("ѧ����")
#define GUI_REG_LABEL_METIER		_T("ְҵ��")
#define GUI_REG_LABEL_DUTY			_T("ְ��")
#define GUI_REG_LABEL_ZIP			_T("�ʱࣺ")
#define GUI_REG_LABEL_SALARY		_T("��н��")
#define GUI_REG_LABEL_ADDRESS		_T("��ַ��")
#define GUI_REG_LABEL_QQ			_T("QQ ���룺")
#define GUI_REG_LABEL_INC			_T("��λ��")

//---------------------------------------------------------------------------------------------
// Popup Message

#define GUI_REMOVE_CAPTION				_T("ж�� �Ѷ����˷���ǽ")
#define GUI_REMOVE_LABEL_INFO			_T("ж�س��򽫴�����������Ƴ� �Ѷ����˷���ǽ�������Զ���Ҫж�صĸ����������ȷ����ť��ʼж�أ���ȡ����ťȡ��ж�ء�")
#define GUI_REMOVE_CHECK_MAIN			_T("ж�� �Ѷ����˷���ǽ")
#define GUI_REMOVE_CHECK_LOG			_T("ж��ʱɾ����־�ļ�")
#define GUI_REMOVE_CHECK_ACL			_T("ж��ʱɾ���عܹ����ļ�")
#define GUI_REMOVE_LABEL_LOGO			_T("XSTUDIO")

//---------------------------------------------------------------------------------------------
// Popup Message

#define GUI_ACL_MESSAGE_INAVALID_PORT					_T("�˿�ֵ��Ч����Ч��ΧΪ 0 - 65535�����������롣")
#define GUI_ACL_MESSAGE_APP_PATH_ERROR					_T("��������Ӧ�ó���·�������ƣ����������롣")
#define GUI_ACL_MESSAGE_WEB_ERROR						_T("����������վվ�㣬���������롣")
#define GUI_ACL_MESSAGE_NNB_ERROR						_T("�������������ھӣ������²�����")
#define GUI_ACL_MESSAGE_APP_NOT_EXSITS					_T("Ӧ�ó��򲻴��ڣ�����·�������ơ�")
#define GUI_ACL_MESSAGE_ONLY_PATH						_T("��������Ӧ�ó���������·�����ļ�����")
#define GUI_ACL_MESSAGE_ADD_ACL_IP_ERROR				_T("��� ACL IP��Χ����")
#define GUI_ACL_MESSAGE_SAVE_ACL_IP_TIME_ERROR			_T("���� ACL IP��Χ��ʱ�䷶Χ����")
#define GUI_ACL_MESSAGE_ADD_ACL_ERROR					_T("��� ACL �������")
#define GUI_ACL_MESSAGE_SAVE_ACL_ERROR					_T("���� ACL �������")
#define GUI_ACL_MESSAGE_ACL_FILE_SET_ERROR				_T("ACL �ļ�ϵͳ���ô���")
#define GUI_ACL_MESSAGE_ACL_READ_ERROR					_T("��ȡ ACL �ļ�����")
#define GUI_ACL_MESSAGE_ACL_ASK_SAVE					_T("Ҫ�����������޸���")
#define GUI_ACL_MESSAGE_PLEASE_CLOSE_SUB_WINDOW			_T("�����ȹر����е��Ӵ��ڡ�")
#define GUI_ACL_MESSAGE_MAX_ACL							_T("���� ACL ����������Ŀ�����ܹ�����ӡ�")
#define GUI_ACL_MESSAGE_MAX_IP_ARIA						_T("���� IP ��Χ�������Ŀ�����ܹ�����ӡ�")
#define GUI_ACL_MESSAGE_DLL_NOT_FOUND					_T("�����ҵ� Xfilter.dll ����ʱ��ܾ���")
#define GUI_ACL_MESSAGE_FUNCTION_NOT_FOUNT				_T("Xfilter.dll ��û���ҵ���Ա������")
#define GUI_ACL_MESSAGE_INIT_DLL_DATA_ERROR				_T("��ʼ�� Xfilter.dll ����������")
#define GUI_ACL_MESSAGE_INIT_CALLBACK_FUNC_ERROR		_T("��ʼ���ص���������")
#define GUI_NET_IP_ARIA_MESSAGE_IP_NULL					_T("IP��ַ����Ϊ�գ����������롣")
#define GUI_NET_IP_ARIA_MESSAGE_INVALID_IP_ARIA			_T("��ʼ IP ����С�ڵ��ڽ��� IP�����������롣")
#define GUI_ACL_MESSAGE_START_TIME_MIN_END_TIME			_T("��ʼʱ�����С�ڽ���ʱ�䡣")
#define GUI_ACL_MESSAGE_CAN_NOT_FIND_RECORD				_T("û���ҵ����������ļ�¼��")
#define GUI_ACL_MESSAGE_SET_WORK_MODE_ERROR				_T("����ģʽ���浽�ļ�����")
#define GUI_ACL_MESSAGE_SET_SECURITY_ERROR				_T("��ȫ�ȼ����浽�ļ�����")
#define GUI_REG_MESSAGE_MUST_INPUT_EMAIL				_T("��������ȷ�� Email ��ַ��")
#define GUI_REG_MESSAGE_MUST_INPUT_EMAIL2				_T("�Ƽ��� Email ��ַ���Ϸ������������롣")
#define GUI_REG_MESSAGE_MUST_INPUT_CITY					_T("���������ڵĳ��С�")
#define GUI_REG_MESSAGE_MUST_INPUT_OS					_T("��ѡ���ʹ�õĲ���ϵͳ��")
#define GUI_REG_MESSAGE_INVALID_PASSWORD				_T("��Ч�����룬���������롣")
#define GUI_REG_MESSAGE_MUST_INPUT_NAME					_T("��������������")
#define GUI_REG_MESSAGE_MUST_INPUT_ID					_T("��������֤�����롣")
#define GUI_ACL_MESSAGE_NO_WINSOCK2						_T("ϵͳû�� Winsock 2 ֧�֡�")
#define GUI_ACL_MESSAGE_INSTALL_FAILED					_T("��װʧ�ܡ�")
#define GUI_ACL_MESSAGE_PROCESS_IS_RUNNING				_T("Xfilter �������У�ж��ǰ�����ȹر� Xfilter��")
#define GUI_MESSAGE_REMOVE_PROVIDER_FAILED				_T("ж�� Xfilter ����������Xfilterû�а�װ���дע������")
#define GUI_MESSAGE_REMOVE_SUCCESS						_T("ж�سɹ���")
#define GUI_MESSAGE_REMOVE_HAVE_FILE_USING				_T("ж�سɹ����������ļ�����ʹ�ã��������������ֹ�ɾ����")
#define GUI_MESSAGE_OPEN_HELP_FAILED					_T("�򿪰����ļ�����")
#define GUI_MESSAGE_HYPER_LINK_ERROR					_T("�򿪳������ӳ���")

//---------------------------------------------------------------------------------------------
// some resource arrays

static TCHAR *GUI_DIRECTION_EX[]= {
	_T("<-"),
	_T("->"),
	_T("<->")
};

static TCHAR *GUI_TIME[]		= {
	GUI_SET_TIME_TREE_0,
	GUI_SET_TIME_TREE_1,
	GUI_SET_TIME_TREE_2,
	GUI_SET_TIME_TREE_3,
	GUI_SET_TIME_TREE_4,
	GUI_SET_TIME_TREE_5,
	GUI_SET_TIME_TREE_6
};

static TCHAR *GUI_NET[]			= {
	GUI_SET_NET_TREE_0,
	GUI_SET_NET_TREE_1,
	GUI_SET_NET_TREE_2,
	GUI_SET_NET_TREE_3,
	GUI_SET_NET_TREE_4
};

#endif	// LANGUAGE_CHINESE

#endif // GUIRES_H

