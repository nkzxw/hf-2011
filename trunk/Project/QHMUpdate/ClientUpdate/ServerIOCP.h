#ifndef _SERVER_IOCP_H_
#define 

/*
* ��ʼ��
*/
bool Initialize();

/*
* ж��
*/
void DeInitialize();

/*
* ���տͻ��������߳�
* @param lParam: 
* Exception: 
* Return: 
*/
DWORD WINAPI AcceptThread(LPVOID lParam);

/*
* ����Socket �� IOCP �Ĺ���
* @param ListenSocket: ����Socket
* Exception: 
* Return: 
*/
void AcceptConnection(SOCKET ListenSocket);

/*
* ������ɶ˿ڵĹ����߳�
* @param lpParam: 
* Exception: 
* Return: 
*/
DWORD WINAPI WorkerThread(LPVOID lpParam);

/*
* ����ͻ���������Ϣ
* @param pClientContext: �ͻ���������Ϣ����
* Exception: 
* Return:
*/
void AddToClientList(CClientContext   *pClientContext);

/*
* �Ƴ��ͻ���������Ϣ
* @param pClientContext: �ͻ���������Ϣ����
* Exception: 
* Return: 
*/
void RemoveFromClientList(CClientContext   *pClientContext);

/*
* ������еĿͻ�������Ϣ
*/
void CleanClientList();


#endif //_SERVER_IOCP_H_