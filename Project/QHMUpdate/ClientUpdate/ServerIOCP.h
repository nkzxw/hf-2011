#ifndef _SERVER_IOCP_H_
#define 

/*
* 初始化
*/
bool Initialize();

/*
* 卸载
*/
void DeInitialize();

/*
* 接收客户端连接线程
* @param lParam: 
* Exception: 
* Return: 
*/
DWORD WINAPI AcceptThread(LPVOID lParam);

/*
* 建立Socket 与 IOCP 的关联
* @param ListenSocket: 监听Socket
* Exception: 
* Return: 
*/
void AcceptConnection(SOCKET ListenSocket);

/*
* 处理完成端口的工作线程
* @param lpParam: 
* Exception: 
* Return: 
*/
DWORD WINAPI WorkerThread(LPVOID lpParam);

/*
* 保存客户端连接信息
* @param pClientContext: 客户端连接信息内容
* Exception: 
* Return:
*/
void AddToClientList(CClientContext   *pClientContext);

/*
* 移除客户端连接信息
* @param pClientContext: 客户端连接信息内容
* Exception: 
* Return: 
*/
void RemoveFromClientList(CClientContext   *pClientContext);

/*
* 清楚所有的客户连接信息
*/
void CleanClientList();


#endif //_SERVER_IOCP_H_