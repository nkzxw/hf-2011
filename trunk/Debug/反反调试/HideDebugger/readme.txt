����Ŀ�����ķ����Ե�С����
��������Ի��ѿǵ�ʱ�򾭳����������ԣ����ǲ�֪��Ŀ������������ַ����Լ�������ʲô�ط����˷����ԡ�ʹ������������߿��Կ�������ķ������ֶ�����Ϣ��
ע�⣺���Ŀ���������peb��DebugFlagλ����NtGlobalFlagλ�������߲���������Ϣ��

ԭ��ܼ򵥣�����һ��������
1.�����������У�hook ZwQueryInformationProcess�����ProcessDebugPort��hook ZwQueryObject�� ���DebugObject�� hook SetInformationThread�� ���ThreadHideFromDebugger��hook GetContextThread�����Ե�����Ӳ���ϵ�ļ�⣬hook SetContextThread�� ��������������Ӳ���ϵ�

2.ȥ��PEB�е�DebugFlagλ��NtGlobalFlagλ��heap flagλ �ȡ�

3.��KiUserExceptionDispatcher��ZwContinue����Ӳ���ϵ㣬����쳣�����еķ�����

4.�ڷ���CREATE_THREAD_DEBUG_EVENT�¼�ʱ�����½��߳�����KiUserExceptionDispatcher��ZwContinueӲ���ϵ㣬��������߳��еķ�����

5.������������쳣������쳣֮����¼���Ҳ����쳣��Ϣ�͵�ַ

6.���Ŀ����������쳣�������ص���Ӳ���ϵ�KiUserExceptionDispatcher������ô���context�еĵ��ԼĴ���������ִ����һ��ZwContinue��ʱ��ԭ��