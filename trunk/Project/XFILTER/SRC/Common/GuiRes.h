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
#define GUI_VERSION					_T("专业版")
#else
#define GUI_VERSION					_T("标准版")
#endif // ZIGUANG
#define USER_REGISTER_QUESTION		_T("费尔个人防火墙软件目前还没有注册，现在要免费注册吗？\n\n\n")
#define USER_REGISTER_ALREDAY		_T("已经注册过，要修改以前的注册信息并重新注册吗？\n\n\n")
#define UPDATE_VERSION_ALREDAY_NEW	_T("此软件已经是最新版本，无需更新。")
#define UPDATE_VERSION_NEW			_T("当前版本：%s，最新版本：%s，欢迎下载最新版本。")
#define UPDATE_VERSION_CANNOT		_T("不能连接到网络。")

static TCHAR* MON_BUTTON_SCROOL_CAPTION[] = {
	_T("自动滚动"),
	_T("停止滚动"),
};
static TCHAR* MON_BUTTON_MONITOR_CAPTION[] = {
	_T("开始监视"),
	_T("停止监视"),
};

#define MON_STATUS_SPACE		_T("                    ")
#define MON_STATUS_FORMAT		_T("状态：%s，%s")
static TCHAR* MON_STATUS_MONITOR[] = {
	_T("不监视"),
	_T("监视"),
};
static TCHAR* MON_STATUS_SCROLL[] = {
	_T("不滚动"),
	_T("滚动"),
};


#define XFILTER_SERVICE_DLL_NAME	_T("XFILTER.DLL")
#define XFILTER_HELP_FILE_NAME		_T("XFILTER.CHM")
#define XFILTER_MEMORY_ACL_FILE		_T("XFILTER_MEMORY_ACL_FILE")

#ifdef LANGUAGE_CHINESE
#ifdef ZIGUANG
#define WINDOW_CAPTION				_T("费尔个人防火墙 - 专业版")
#else
#define WINDOW_CAPTION				_T("费尔个人防火墙 - 标准版")
#endif // ZIGUANG
#else
#define WINDOW_CAPTION				_T("XFILTER PERSONAL FIREWALL 2")
#endif

#define BUTTON_CAPTION_HIDE			_T("隐藏")
#define BUTTON_CAPTION_CLOSE		_T("关闭")
#define BUTTON_CAPTION_MIN			_T("最小化")
#define BUTTON_CAPTION_TOPMOST		_T("置于顶层")

#define BUTTON_CAPTION_GREEN		_T("全部放行")
#define BUTTON_CAPTION_YELLOW		_T("过滤")
#define BUTTON_CAPTION_RED			_T("全部拒绝")
static TCHAR* WORK_MODE_STATUS[] = {
	BUTTON_CAPTION_GREEN,
	BUTTON_CAPTION_YELLOW,
	BUTTON_CAPTION_RED,
};

#define ACL_CAPTION_ADD				_T("增加控管规则")
#define ACL_CAPTION_EDIT			_T("修改控管规则")
#define ACL_CAPTION_APP_ADD			_T("增加应用程序控管规则")
#define ACL_CAPTION_APP_EDIT		_T("修改应用程序控管规则")
#define ACL_CAPTION_WEB_ADD			_T("增加网站过滤规则")
#define ACL_CAPTION_WEB_EDIT		_T("修改网站过滤规则")
#define ACL_CAPTION_NNB_ADD			_T("增加网上邻居控管规则")
#define ACL_CAPTION_NNB_EDIT		_T("修改网上邻居控管规则")
#define ACL_CAPTION_ICMP_ADD		_T("增加ICMP控管规则")
#define ACL_CAPTION_ICMP_EDIT		_T("修改ICMP控管规则")
#define ACL_CAPTION_NET_ADD			_T("增加IP地址段记录")
#define ACL_CAPTION_NET_EDIT		_T("修改IP地址段记录")

#define ACL_QUERY_CAPTION_APP		_T("应用程序过滤")
#define ACL_QUERY_CAPTION_APP_DRV	_T("应用程序过滤")
#define ACL_QUERY_CAPTION_WEB		_T("站点过滤")
#define ACL_QUERY_CAPTION_NNB		_T("网上邻居过滤")
#define ACL_QUERY_CAPTION_ICMP		_T("ICMP 过滤")

#define ACL_QUERY_CAPTION_INFO_APP			_T("对本次连接请求如何处理？")
#define ACL_QUERY_CAPTION_INFO_APP_DRV		_T("本次连接已拒绝，以后对此类连接请求如何处理？")
#define ACL_QUERY_CAPTION_INFO_WEB			_T("对本次连接请求如何处理？")
#define ACL_QUERY_CAPTION_INFO_NNB			_T("本次连接已拒绝，以后对此类连接请求如何处理？")
#define ACL_QUERY_CAPTION_INFO_ICMP			_T("本次连接已拒绝，以后对此类连接请求如何处理？")


static TCHAR *ACL_CAPTION[] = {
	_T("管控中心 - 应用程序控管规则"),
	_T("管控中心 - 网站过滤规则"),
	_T("管控中心 - 网上邻居控管规则"),
	_T("管控中心 - ICMP 控管规则"),
	_T("管控中心 - 木马检测"),
	_T("管控中心 - 时间类型设置"),
	_T("管控中心 - 网络类型设置")
};
#define ACL_BUTTON_COUNT	sizeof(ACL_CAPTION)/sizeof(TCHAR*)


static TCHAR *ACL_TORJAN_STATUS[] = {
	_T("已停止"),
	_T("已启动")
};

static TCHAR *ACL_TORJAN_BUTTON[] = {
	_T("启动"),
	_T("停止")
};

static TCHAR *ACL_FILE_BUTTON[] = {
	_T("启动"),
	_T("停止")
};

static TCHAR *ACL_QUERY_TEXT[] = {
	_T("放行"),
	_T("拒绝"),
	_T("询问")
};
#define ACL_QUERY_TEXT_COUNT		sizeof(ACL_QUERY_TEXT)/sizeof(TCHAR*)

#define QUERY_LIST_LABEL_WEB		_T("站点：")
#define QUERY_LIST_LABEL_APP		_T("应用程序：")
#define QUERY_LIST_LABEL_PROTOCOL	_T("协议：")
#define QUERY_LIST_LABEL_DIRECTION	_T("连线方向：")
#define QUERY_LIST_LABEL_LOCAL		_T("本地IP/端口：")
#define QUERY_LIST_LABEL_REMOTE		_T("目的IP/端口：")
#define QUERY_LIST_LABEL_TIME		_T("时间：")
#define QUERY_LIST_LABEL_HOST		_T("本地/远端主机：")

#define CHAR_WIDTH		10
#define MEMO_CONST		_T("无")
static TCHAR *ACL_APP_LIST_HEADER[] = {
	_T("编号"),
	_T("应用程序"),
	_T("方向"),
	_T("目的网络"),
	_T("目的端口"),
	_T("本地端口(侦听)"),
	_T("动作"),
	_T("协议"),
	_T("时间类型"),
	_T("备注"),
	_T("应用程序路径"),
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
	_T("编号"),
	_T("站点"),
	_T("动作"),
	_T("备注")
};
static int ACL_WEB_LIST_LENTH[] = {
	_tcslen(ACL_WEB_LIST_HEADER[0]) * CHAR_WIDTH,
	_tcslen(ACL_WEB_LIST_HEADER[1]) * CHAR_WIDTH + 160,
	_tcslen(ACL_WEB_LIST_HEADER[2]) * CHAR_WIDTH,
	_tcslen(ACL_WEB_LIST_HEADER[3]) * CHAR_WIDTH + 100
};
#define ACL_WEB_LIST_COUNT	sizeof(ACL_WEB_LIST_HEADER)/sizeof(TCHAR*)

#define ACL_NNB_COMSTOM			_T("自定义")
static TCHAR *ACL_NNB_LIST_HEADER[] = {
	_T("编号"),
	_T("网上邻居"),
	_T("连线方向"),
	_T("时间类型"),
	_T("动作"),
	_T("备注")
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
	_T("编号"),
	_T("远端网络"),
	_T("连线方向"),
	_T("时间类型"),
	_T("动作"),
	_T("备注")
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
	_T("任何时间"),
	_T("工作时间"),
	_T("工作日业余时间"),
	_T("周末"),
	_T("约束时间"),
	_T("信任时间"),
	_T("自定义时间")
};
#define ACL_TIME_TYPE_COUNT	sizeof(ACL_TIME_TYPE)/sizeof(TCHAR*)

static TCHAR *ACL_NET_TYPE[] = {
	_T("所有网络"),
	_T("局域网"),
	_T("受约束的网络"),
	_T("信任的网络"),
	_T("自定义的网络")
};
#define ACL_NET_TYPE_COUNT	sizeof(ACL_NET_TYPE)/sizeof(TCHAR*)

static TCHAR *ACL_IP_LIST_HEADER[] = {
	_T("编号"),
	_T("开始IP"),
	_T("结束IP")
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
	_T("网络监控室 - 应用程序封包监视"),
	_T("网络监控室 - 网上邻居封包监视"),
	_T("网络监控室 - ICMP 封包监视"),
	_T("网络监控室 - 侦听端口"),
	_T("网络监控室 - 当前连线")
};

static TCHAR *LISTEN_HEADER[] = {
	_T("编号"),
	_T("应用程序"),
	_T("协议"),
	_T("侦听端口"),
	_T("开始时间/消逝时间"),
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
	_T("编号"),
	_T("应用程序"),
	_T("目的IP/端口"),
	_T("发送/接收"),
	_T("协议/连线"),
	_T("开始时间/消逝时间"),
	_T("本地IP/端口"),
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
	_T("动作"),
	_T("应用程序"),
	_T("时间"),
	_T("目的IP/端口"),
	_T("发送/接收"),
	_T("协议/方向"),
	_T("本地IP/端口"),
	_T("备注"),
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
	_T("动作"),
	_T("应用程序"),
	_T("本地主机/端口"),
	_T("远端主机/端口"),
	_T("状态/流量"),
	_T("协议/方向"),
	_T("时间"),
	_T("备注"),
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
	_T("动作"),
	_T("应用程序"),
	_T("时间"),
	_T("状态/流量"),
	_T("方向"),
	_T("备注"),
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

#define GUI_ACL_ACTION_0					_T("放行")
#define GUI_ACL_ACTION_1					_T("拒绝")
#define GUI_ACL_ACTION_2					_T("询问")
#define GUI_ACL_DERECTION_0					_T("连入")
#define GUI_ACL_DERECTION_1					_T("连出")
#define GUI_ACL_DERECTION_2					_T("双向")
#define GUI_ACL_DERECTION_3					_T("广播")
#define GUI_ACL_DERECTION_4					_T("侦听")
#define GUI_ACL_SERVICE_TYPE_0				_T("任何")
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

#define GUI_ACL_EXPLAIN_CONST			_T("'*'代表所有应用程序，0代表所有端口。")
#define GUI_ACL_EXPLAIN_PORT_ALL		_T("所有")
#define GUI_ACL_EXPLAIN_OUT_FORMAT		_T("%s%s通过%s协议的%s端口访问%s的%s端口将被%s。%s")
#define GUI_ACL_EXPLAIN_IN_FORMAT		_T("%s%s通过%s协议的%s端口向本机%s%s端口发出的连接请求将被%s。%s")
#define GUI_QUERY_EXPLAIN_APP_OUT		_T("%s %s 通过 %s 协议的 %u 端口访问 %s 的 %u 端口，是否放行？")
#define GUI_QUERY_EXPLAIN_APP_IN		_T("%s %s 通过 %s 协议的 %u 端口向本机 %s %u 端口发出连接请求，是否放行？")
#define GUI_QUERY_EXPLAIN_WEB			_T("应用程序 %s 要访问站点 %s，是否放行？")

static TCHAR* GUI_QUERY_EXPLAIN_NNB[] = {
	_T("%s 要访问本机的共享资源，是否放行？"),
	_T("本机要访问 %s 的共享资源，是否放行？"),
};

static TCHAR* GUI_QUERY_EXPLAIN_ICMP[] = {
	_T("远端主机 %s 向本机发出 ICMP(Ping) 请求，是否放行？"),
	_T("本机向 %s 发出 ICMP(Ping) 请求，是否放行？"),
};



#define ERROR_STRING_NOCODE					_T("未知的错误")
#define ERROR_STRING_SUCCESS				_T("操作成功")
#define ERROR_STRING_FILE_NOT_FOUND			_T("没有找到指定的文件")
#define ERROR_STRING_FILE_ALREDAY_EXISTS	_T("文件已经存在")
#define ERROR_STRING_FILE_LOCKED			_T("文件被锁定")
#define ERROR_STRING_FILE_CREATE_FAILURE	_T("创建文件失败")
#define ERROR_STRING_FILE_CAN_NOT_OPEN		_T("不能够打开文件")
#define ERROR_STRING_FILE_INVALID_SIGNATURE	_T("无效的文件标识")
#define ERROR_STRING_FILE_READ_ERROR		_T("文件读取错误")
#define ERROR_STRING_FILE_SAVE_ERROR		_T("文件保存错误")
#define ERROR_STRING_FILE_ADD_ERROR			_T("增加记录错误")
#define ERROR_STRING_FILE_GET_STATUS_ERROR	_T("得到文件状态操作失败")
#define ERROR_STRING_FILE_READ_ONLY			_T("文件为只读属性")
#define ERROR_STRING_FILE_WRITER_HEADER_ERROR	_T("保存文件头错误")
#define ERROR_STRING_FILE_RECORD_CAN_NOT_FIND	_T("没有发现指定的记录")
#define ERROR_STRING_FILE_INVALID_PARAMETER		_T("无效的参数")
#define ERROR_STRING_FILE_INVALID_VERSION		_T("文件版本无效")

#define ERROR_STRING_CAN_NOT_READ_PACKET		_T("不能获取网络封包，如果安装后还没有重新启动，请重新启动。\n\n\n")
#define ERROR_STRING_HAVE_NOT_INSTALL			_T("没有安装费尔个人防火墙，需要重新安装。\n\n\n")
#define ERROR_STRING_DRIVER_NOT_FOUNT			_T("费尔个人防火墙不能加载驱动程序组件，请重新安装。\n\n\n")

//---------------------------------------------------------------------------------------------
//app's resource

#define GUI_APP_CLASS_NAME			_T("Xfilter")
#define	GUI_APP_CAPTION				_T("费尔个人防火墙")
#define	GUI_APP_LOGO				_T("XSTUDIO")

//---------------------------------------------------------------------------------------------
//menu's resource

#define GUI_MENU_FILE				_T("文件(&F)")
#define GUI_MENU_ABOUT				_T("关于(&A)")
#define GUI_MENU_CONTROL_FRAME		_T("主窗口(&C)")
#define GUI_MENU_PACKET_MONITOR		_T("封包监视(&M)")
#define GUI_MENU_LOG_QUERY			_T("日志查询(&Q)")
#define GUI_MENU_ACL				_T("控管规则(&L)")
#define GUI_MENU_SYSTEM_SET			_T("系统设置(&S)")
#define GUI_MENU_STOP_FILTER		_T("停止过滤(&T)")
#define GUI_MENU_START_FILTER		_T("开始过滤(&F)")
#define GUI_MENU_EXIT				_T("退出(&X)")

//---------------------------------------------------------------------------------------------
//button's resource

#define GUI_BUTTON_OK				_T("确定")
#define GUI_BUTTON_CANCEL			_T("取消")
#define GUI_BUTTON_CANCEL_EX		_T("取消(&C)")
#define GUI_BUTTON_APPLY			_T("应用(&A)")
#define GUI_BUTTON_HELP				_T("帮助")

//---------------------------------------------------------------------------------------------
//tab's resource

#define GUI_TAB_PACKET_MONITOR		_T("封包监视")
#define GUI_TAB_LOG_QUERY			_T("日志查询")
#define GUI_TAB_ACL					_T("控管规则")
#define GUI_TAB_SYSTEM_SETTING		_T("系统设置")
#define GUI_TAB_ABOUT				_T("关于")

//---------------------------------------------------------------------------------------------
//packet monitor resource

#define	GUI_PACKET_MONITOR_TITLE						_T("封包监视")
#define GUI_PACKET_MONITOR_LABLE						_T("封包监视列表：")
#define GUI_PACKET_MONITOR_TOOLBAR_CLEAR				_T("清除")
#define GUI_PACKET_MONITOR_TOOLBAR_STOP_SCROLL			_T("停止滚动")
#define GUI_PACKET_MONITOR_TOOLBAR_START_SCROLL			_T("自动滚动")
#define GUI_PACKET_MONITOR_TOOLBAR_STOP_MONITOR			_T("停止监视")
#define GUI_PACKET_MONITOR_TOOLBAR_START_MONITOR		_T("开始监视")
#define GUI_PACKET_MONITOR_LIST_ACTION					_T("动作")
#define GUI_PACKET_MONITOR_LIST_ACTION_SIZE				\
		sizeof(GUI_PACKET_MONITOR_LIST_ACTION)			* 8
#define GUI_PACKET_MONITOR_LIST_STARTTIME_ENDTIME		_T("开始时间 - 结束时间")
#define GUI_PACKET_MONITOR_LIST_STARTTIME_ENDTIME_SIZE	\
		sizeof(GUI_PACKET_MONITOR_LIST_STARTTIME_ENDTIME) * 8
#define GUI_PACKET_MONITOR_LIST_PROTOCOL				_T("协议")
#define GUI_PACKET_MONITOR_LIST_PROTOCOL_SIZE		\
		sizeof(GUI_PACKET_MONITOR_LIST_PROTOCOL)		* 8
#define GUI_PACKET_MONITOR_LIST_IN_DATA					_T("进流量")
#define GUI_PACKET_MONITOR_LIST_IN_DATA_SIZE			\
		sizeof(GUI_PACKET_MONITOR_LIST_IN_DATA)			* 8
#define GUI_PACKET_MONITOR_LIST_OUT_DATA				_T("出流量")
#define GUI_PACKET_MONITOR_LIST_OUT_DATA_SIZE			\
		sizeof(GUI_PACKET_MONITOR_LIST_OUT_DATA)		* 8
#define GUI_PACKET_MONITOR_LIST_IP_PORT					_T("本地 IP : 端口 - 远端 IP : 端口")
#define GUI_PACKET_MONITOR_LIST_IP_PORT_SIZE			\
		sizeof(GUI_PACKET_MONITOR_LIST_IP_PORT)			* 8
#define GUI_PACKET_MONITOR_LIST_APPLICATION				_T("应用程序 : 备注")
#define GUI_PACKET_MONITOR_LIST_APPLICATION_SIZE		\
		sizeof(GUI_PACKET_MONITOR_LIST_APPLICATION)		* 8

//---------------------------------------------------------------------------------------------
//log query resource

#define	GUI_LOG_QUERY_SET_LABLE				_T("查询条件设置")
#define GUI_LOG_QUERY_SET_START_TIME_LABLE	_T("开始日期/时间：")
#define GUI_LOG_QUERY_SET_END_TIME_LABLE	_T("结束日期/时间：")
#define GUI_LOG_QUERY_RESULT_LABLE			_T("查询结果")
#define GUI_LOG_QUERY_RESULT_LIST_LABLE		_T("第%u页,共%u页,当前%u笔,共%u笔")
#define GUI_LOG_QUERY_BUTTON_QUERY			_T("开始查询(&Q)")
#define GUI_LOG_QUERY_BUTTON_BACK			_T("上一页(&B)")
#define GUI_LOG_QUERY_BUTTON_NEXT			_T("下一页(&N)")
#define GUI_LOG_QUERY_BUTTON_DELETE			_T("清除(&D)")

//---------------------------------------------------------------------------------------------
//ACL RESOURCE

#define GUI_ACL_APPLICATION_SET_LABLE		_T("应用程序设置")
#define GUI_ACL_RADIO_PASS					_T("放行所有")
#define GUI_ACL_RADIO_QUERY					_T("询问")
#define GUI_ACL_RADIO_DENY					_T("拒绝所有")
#define GUI_ACL_SET_LABLE					_T("控管规则设置")
#define GUI_ACL_BUTTON_ADD					_T("添加(&A)")
#define GUI_ACL_BUTTON_EDIT					_T("修改(&E)")
#define GUI_ACL_BUTTON_DELETE				_T("删除(&D)")

//---------------------------------------------------------------------------------------------
//ACL SET RESOURCE

//---------------------------------------------------------------------------------------------
//net and time set sheet resource

#define GUI_NET_TIME_SHEET_CAPTION			_T("网络与时间设置")

//---------------------------------------------------------------------------------------------
//set time resource

#define GUI_SET_TIME_WEEK_TIME_LABLE		_T("星期与时间")
#define GUI_SET_TIME_CHECK_SUNDAY			_T("星期日")
#define GUI_SET_TIME_CHECK_MONDAY			_T("星期一")
#define GUI_SET_TIME_CHECK_TUESDAY			_T("星期二")
#define GUI_SET_TIME_CHECK_WEDNESDAY		_T("星期三")
#define GUI_SET_TIME_CHECK_THURSDAY			_T("星期四")
#define GUI_SET_TIME_CHECK_FRIDAY			_T("星期五")
#define GUI_SET_TIME_CHECK_SATURDAY			_T("星期六")
#define GUI_SET_TIME_TIME_LABLE				_T("时间：")
#define GUI_SET_TIME_LABLE_START_TIME		_T("开始时间")
#define GUI_SET_TIME_LABLE_END_TIME			_T("结束时间")
#define GUI_SET_TIME_TREE_0					_T("任何时间")
#define GUI_SET_TIME_TREE_1					_T("工作时间")
#define GUI_SET_TIME_TREE_2					_T("工作日业余时间")
#define GUI_SET_TIME_TREE_3					_T("周末")
#define GUI_SET_TIME_TREE_4					_T("约束时间")
#define GUI_SET_TIME_TREE_5					_T("信任时间")
#define GUI_SET_TIME_TREE_6					_T("自定义时间")

//---------------------------------------------------------------------------------------------
//net ip aria resource

#define GUI_NET_IP_ARIA_CAPTION				_T("IP 地址段")
#define GUI_NET_IP_ARIA_LABLE				_T("远端网络 IP 地址段")
#define GUI_NET_IP_ARIA_LABLE_START_IP		_T("开始 IP：")
#define GUI_NET_IP_ARIA_LABLE_END_IP		_T("结束 IP：")
#define GUI_NET_IP_ARIA_ALL					_T("*")

//---------------------------------------------------------------------------------------------
//set remote net resource

#define GUI_SET_NET_LABLE					_T("远端网络信息")
#define GUI_SET_NET_NET_LABLE				_T("网络信息")
#define GUI_SET_NET_BUTTON_ADD				_T("添加(&A)")
#define GUI_SET_NET_BUTTON_EDIT				_T("修改(&E)")
#define GUI_SET_NET_BUTTON_DELETE			_T("删除(&D)")
#define GUI_SET_NET_LIST_START_IP			_T("开始IP")
#define GUI_SET_NET_LIST_END_IP				_T("结束IP")
#define GUI_SET_NET_TREE_0					_T("所有网络")
#define GUI_SET_NET_TREE_1					_T("局域网")
#define GUI_SET_NET_TREE_2					_T("受约束的网络")
#define GUI_SET_NET_TREE_3					_T("信任的网络")
#define GUI_SET_NET_TREE_4					_T("自定义的网络")

//---------------------------------------------------------------------------------------------
//system set resource

#define GUI_SYSTEM_SET_COMMON_SET_LABLE		_T("公共设置")
#define GUI_SYSTEM_SET_CHECK_LOG			_T("记录日志，日志文件大小为")
#define GUI_SYSTEM_SET_UNIT_LABLE			_T("M。")
#define GUI_SYSTEM_SET_CHECK_AUTOSTART		_T("Windows 启动时自动启动 Xfilter。")
#define GUI_SYSTEM_SET_CHECK_SPLASH			_T("Xfilter 启动时显示欢迎界面。")
#define GUI_SYSTEM_SET_ALERT_SET_LABLE		_T("报警设置")
#define GUI_SYSTEM_SET_CHECK_ALERT_PCSPEAKER	_T("拦截时 PC 喇叭发出报警声音。")
#define GUI_SYSTEM_SET_CHECK_ALERT_DIALOG	_T("拦截时闪烁任务栏上的图标。")

//---------------------------------------------------------------------------------------------
//about resource

#define GUI_ABOUT_LABLE_TITLE			_T("费尔个人防火墙")
#define GUI_ABOUT_LABLE_COPYRIGHT1		_T("版本：%u.%u.%u (%s)")
#define GUI_ABOUT_LABLE_COPYRIGHT2		_T("Copyright (C) 2002-2003 Passeck Technology")
#define GUI_ABOUT_LABLE_COPYRIGHT3		_T("All rights reserved.")
#define GUI_ABOUT_LABLE_ACCREDIT_TO		_T("本软件授权给 %s 使用")
#define GUI_ABOUT_LABLE_WEB_ADDRESS_LABLE	_T("网站地址：")
#define GUI_ABOUT_LABLE_EMAIL_LABLE			_T("电子邮件：")
#define GUI_ABOUT_LABLE_WEB_ADDRESS		_T("http://www.xfilt.com/")
#define GUI_ABOUT_LABLE_EMAIL			_T("mailto:xstudio@xfilt.com")
#define GUI_ABOUT_LABLE_WARNING			_T("警告：本软件受著作权法的保护。允许完整的自由传播，但不允许破坏软件的完整性或作为盈利目的，凡未经授权而进行营利活动和其它一切商业行为，将遭到民事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。")
#define GUI_ABOUT_LABLE_INFO			_T("此软件出售源代码，详细信息请浏览上面的网址。网站上列出了产品价格和付款方式，或直接发送 Email 进行咨询。")
#define GUI_ABOUT_LABLE_AUTHOR			_T("作者：朱雁辉")

//---------------------------------------------------------------------------------------------
// Register Dialog Resource

#define GUI_REG_CAPTION				_T("Xfilter 用户注册")
#define GUI_REG_LABEL_INFO_1		_T("本软件出售源代码，详细信息请浏览下面的网址。软件本身是完全免费的，只要输入 Email 地址，您就成为本软件的合法用户。")
#define GUI_REG_LABEL_INFO_3		_T("请确保 Email 的真实性，这将作为技术支持的主要途径。")
#define GUI_REG_LABEL_INFO_2		_T("注册以下信息您将可以在第一时间得到本软件的升级程序，并有可能成为幸运用户免费获得源代码。 ")
#define GUI_REG_LABEL_EMAIL			_T("Email：*")
#define GUI_REG_LABEL_NAME			_T("姓名：")
#define GUI_REG_LABEL_GENDER		_T("性别：")
#define GUI_REG_LABEL_ID_TYPE		_T("证件类型：")
#define GUI_REG_LABEL_ID			_T("证件号码：")
#define GUI_REG_LABEL_BIRTHDAY		_T("生日：")
#define GUI_REG_LABEL_DEGREE		_T("学历：")
#define GUI_REG_LABEL_METIER		_T("职业：")
#define GUI_REG_LABEL_DUTY			_T("职务：")
#define GUI_REG_LABEL_ZIP			_T("邮编：")
#define GUI_REG_LABEL_SALARY		_T("月薪：")
#define GUI_REG_LABEL_ADDRESS		_T("地址：")
#define GUI_REG_LABEL_QQ			_T("QQ 号码：")
#define GUI_REG_LABEL_INC			_T("单位：")

//---------------------------------------------------------------------------------------------
// Popup Message

#define GUI_REMOVE_CAPTION				_T("卸载 费尔个人防火墙")
#define GUI_REMOVE_LABEL_INFO			_T("卸载程序将从您计算机上移除 费尔个人防火墙，您可以定制要卸载的附加组件，按确定按钮开始卸载，按取消按钮取消卸载。")
#define GUI_REMOVE_CHECK_MAIN			_T("卸载 费尔个人防火墙")
#define GUI_REMOVE_CHECK_LOG			_T("卸载时删除日志文件")
#define GUI_REMOVE_CHECK_ACL			_T("卸载时删除控管规则文件")
#define GUI_REMOVE_LABEL_LOGO			_T("XSTUDIO")

//---------------------------------------------------------------------------------------------
// Popup Message

#define GUI_ACL_MESSAGE_INAVALID_PORT					_T("端口值无效，有效范围为 0 - 65535，请重新输入。")
#define GUI_ACL_MESSAGE_APP_PATH_ERROR					_T("必须输入应用程序路径和名称，请重新输入。")
#define GUI_ACL_MESSAGE_WEB_ERROR						_T("必须输入网站站点，请重新输入。")
#define GUI_ACL_MESSAGE_NNB_ERROR						_T("必须设置网上邻居，请重新操作。")
#define GUI_ACL_MESSAGE_APP_NOT_EXSITS					_T("应用程序不存在，请检查路径和名称。")
#define GUI_ACL_MESSAGE_ONLY_PATH						_T("必须输入应用程序完整的路径和文件名。")
#define GUI_ACL_MESSAGE_ADD_ACL_IP_ERROR				_T("添加 ACL IP范围错误。")
#define GUI_ACL_MESSAGE_SAVE_ACL_IP_TIME_ERROR			_T("保存 ACL IP范围或时间范围错误。")
#define GUI_ACL_MESSAGE_ADD_ACL_ERROR					_T("添加 ACL 规则错误。")
#define GUI_ACL_MESSAGE_SAVE_ACL_ERROR					_T("保存 ACL 规则错误。")
#define GUI_ACL_MESSAGE_ACL_FILE_SET_ERROR				_T("ACL 文件系统设置错误。")
#define GUI_ACL_MESSAGE_ACL_READ_ERROR					_T("读取 ACL 文件错误。")
#define GUI_ACL_MESSAGE_ACL_ASK_SAVE					_T("要保存所做的修改吗？")
#define GUI_ACL_MESSAGE_PLEASE_CLOSE_SUB_WINDOW			_T("请首先关闭所有的子窗口。")
#define GUI_ACL_MESSAGE_MAX_ACL							_T("超过 ACL 规则的最大数目，不能够再添加。")
#define GUI_ACL_MESSAGE_MAX_IP_ARIA						_T("超过 IP 范围的最大数目，不能够再添加。")
#define GUI_ACL_MESSAGE_DLL_NOT_FOUND					_T("不能找到 Xfilter.dll 或访问被拒绝。")
#define GUI_ACL_MESSAGE_FUNCTION_NOT_FOUNT				_T("Xfilter.dll 中没有找到成员函数。")
#define GUI_ACL_MESSAGE_INIT_DLL_DATA_ERROR				_T("初始化 Xfilter.dll 数据区错误。")
#define GUI_ACL_MESSAGE_INIT_CALLBACK_FUNC_ERROR		_T("初始化回调函数错误。")
#define GUI_NET_IP_ARIA_MESSAGE_IP_NULL					_T("IP地址不能为空，请重新输入。")
#define GUI_NET_IP_ARIA_MESSAGE_INVALID_IP_ARIA			_T("开始 IP 必须小于等于结束 IP，请重新输入。")
#define GUI_ACL_MESSAGE_START_TIME_MIN_END_TIME			_T("开始时间必须小于结束时间。")
#define GUI_ACL_MESSAGE_CAN_NOT_FIND_RECORD				_T("没有找到符合条件的记录。")
#define GUI_ACL_MESSAGE_SET_WORK_MODE_ERROR				_T("工作模式保存到文件错误。")
#define GUI_ACL_MESSAGE_SET_SECURITY_ERROR				_T("安全等级保存到文件错误。")
#define GUI_REG_MESSAGE_MUST_INPUT_EMAIL				_T("请输入正确的 Email 地址。")
#define GUI_REG_MESSAGE_MUST_INPUT_EMAIL2				_T("推荐人 Email 地址不合法，请重新输入。")
#define GUI_REG_MESSAGE_MUST_INPUT_CITY					_T("请输入所在的城市。")
#define GUI_REG_MESSAGE_MUST_INPUT_OS					_T("请选择最常使用的操作系统。")
#define GUI_REG_MESSAGE_INVALID_PASSWORD				_T("无效的密码，请重新输入。")
#define GUI_REG_MESSAGE_MUST_INPUT_NAME					_T("必须输入姓名。")
#define GUI_REG_MESSAGE_MUST_INPUT_ID					_T("必须输入证件号码。")
#define GUI_ACL_MESSAGE_NO_WINSOCK2						_T("系统没有 Winsock 2 支持。")
#define GUI_ACL_MESSAGE_INSTALL_FAILED					_T("安装失败。")
#define GUI_ACL_MESSAGE_PROCESS_IS_RUNNING				_T("Xfilter 正在运行，卸载前请首先关闭 Xfilter。")
#define GUI_MESSAGE_REMOVE_PROVIDER_FAILED				_T("卸载 Xfilter 服务程序错误，Xfilter没有安装或读写注册表错误。")
#define GUI_MESSAGE_REMOVE_SUCCESS						_T("卸载成功。")
#define GUI_MESSAGE_REMOVE_HAVE_FILE_USING				_T("卸载成功，但是有文件正在使用，请重新启动后手工删除。")
#define GUI_MESSAGE_OPEN_HELP_FAILED					_T("打开帮助文件错误。")
#define GUI_MESSAGE_HYPER_LINK_ERROR					_T("打开超级链接出错。")

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

