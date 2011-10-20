/***************************************************************
ģ�飺Dbg.h
��Ȩ��Tweek

Module name: Dbg.h
Copyright (c) 2010 Tweek

Notice:	it was implemented by Tweek when it works nice
		if not, I don't know who wrote it.

***************************************************************/

#ifdef MYDLLAPI

#else 
#define MYDLLAPI extern "C" __declspec(dllimport)
#endif

//�����Խ��̵������Ϣ
MYDLLAPI DEBUG_EVENT					stDebugEvent;
MYDLLAPI PROCESS_INFORMATION			stProcessInfo;

//��ͨ�ϵ�
//dwBreakAddress�Ƕϵ��ַ��pi��ָ�򱻵��Խ��̵Ľ�����Ϣ��ָ��
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
//����:����Ӧ�ĵ�ַ����0xCC���
MYDLLAPI int SetNormalBreakPoint(DWORD dwBreakAddress);

//ɾ����ͨ�ϵ�
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int DelNormalBreakPoint(DWORD dwBreakAddress);

//��ȡ������ͨ�ϵ�ĵ�ַ
//���ص��ǵ�i���ϵ�ĵ�ַ��ͨ��ѭ�������г�����int3�ϵ�
//����ֵ���Ƕ�Ӧ�ϵ��ַ
MYDLLAPI DWORD GetNormalBreakPoints(int i);

//�ڴ�ϵ�
//dwBreakAddress�Ƕϵ��ַ
//����:����Ӧ��ַ�޸Ĳ���Ȩ��
/*********
typeȡֵ��
1��д��ϵ�
2�����ʶϵ�  
3��һ���Է��ʶϵ�ϵ� ���������ֶϵ�󣬲���ҪDel��ϵͳ�Զ����쳣�������
4��ִ�жϵ�
********/
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int SetMemoryBreakPoint(DWORD dwBreakAddress, int type);

//ɾ���ڴ�ϵ�
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int DelMemoryBreakPoint(DWORD dwBreakAddress);

//Ӳ���ϵ�
/**********
Register:ȡֵ
0:Dr0
1:Dr1
2:Dr2
3:Dr3
type��ȡֵ
100  ȫ�� 1�ֽ� ִ�жϵ�

101  ȫ�� 1�ֽ� д�����ݶϵ�

103  ȫ�� 1�ֽ� ��д�ϵ�

111  ȫ�� 2�ֽ� д�����ݶϵ�

113  ȫ�� 2�ֽ� ��д�ϵ�

131  ȫ�� 4�ֽ� д�����ݶϵ�

133  ȫ�� 4�ֽ� ��д�ϵ�

000  �ֲ� 1�ֽ� ִ�жϵ�

001  �ֲ� 1�ֽ� д�����ݶϵ�

003  �ֲ� 1�ֽ� ��д�ϵ�

011  �ֲ� 2�ֽ� д�����ݶϵ�

013  �ֲ� 2�ֽ� ��д�ϵ�

031  �ֲ� 4�ֽ� д�����ݶϵ�

033  �ֲ� 4�ֽ� ��д�ϵ�

I/0�˿ڶϵ������ҵ�cpu��֧��
���� 8�ֽڵ�������Ҽ����ҵ�cpu ��֧��
������û������
LE GE��cpu��֧�֣�û������,GD����������Ҳû������
******************************/
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int SetHardwareBreakPoint(DWORD dwBreakAddress,int Register, int type);

//ɾ��Ӳ���ϵ�
/**********
Register:ȡֵ
0:Dr0
1:Dr1
2:Dr2
3:Dr3
*************/
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int DelHardwareBreakPoint(int Register);

//���ر����Խ��� FilePath �ļ�·��
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int LoadDebuggedProcess(LPCWSTR FilePath);

//��ָ�������¼����𱻵��Խ���
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int StopOnDebugEvent(DWORD dwDebugEventCode);

//���쳣ʱ�������
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int StopOnException();

//����������Ľ���
//����ֵΪ0������  ��Ϊ0 ���Ƕ�Ӧ������
MYDLLAPI int ResumeDebuggedThread();