1. ��ɶ˿ڵ�������ϸ�����кܶ෽��:
����accept, AcceptEx֮�࣬����accept�Ĵ�������ࡣ
�Ҿ���ֻҪ����ס����Ҫ������ˣ��첽������������У����ݵĴ��ͣ��ύ����ʱ����ȥ����ѯʱȡ���������������߳�

2. ͬ��I/O & �첽I/O
   1). ͬ��I/O: ����ͬһ��I/O��������ͬһʱ��ֻ����һ��I/O����.
   2). �첽I/O: ֧�ֶ�ͬһ��I/O����Ĳ��д���.
   3). �첽I/O����(���ŶӴ���I/O����):
	   �ص�I/O: ͬһ���߳̿��ԶԶ��I/O�������I/O��������ͬ���߳�Ҳ���Զ�ͬһ��I/O������в���.
	   �첽���̵��ã�APC������չI/O: ÿ��һ��IO��������ʱ�����һ�������Ϣ�������IO������������̵Ļ�����ӵ�������̶��С�һ�������߳̽���ɱ�ȴ�״̬���ͻ�����ִ�ж����е�������̡�
	   ʹ����ɶ˿ڣ�IOCP��
	   
   �ص�io��ʹ�ã�
   Ϊ�˽��I/O��������������⣬windows�������ص�io�ĸ���.
   �����createfile��ʱ��������file_flag_overlapped ����ô�ڵ���readfile��writefile��ʱ��Ϳ��Ը��������һ����������һ��overlapped�ṹ��
   ����readfile����writefile�ĵ������Ͼͻ᷵�أ���ʱ�������ȥ����Ҫ�����£�ϵͳ���Զ��������readfile����writefile,���������readfile����writefile��
   �����������£�ϵͳͬʱҲ�������readfile��writefile�Ĳ������������ν���ص���
   ʹ���ص�io����һ���ô������������ͬʱ��������readfile����writefile�ĵ��ã�
   Ȼ����waitforsingleobject����waitformultipleobjects���ȴ�����ϵͳ�Ĳ������֪ͨ���ڵõ�֪ͨ�źź󣬾Ϳ�����getoverlappedresult����ѯio���õĽ����  
   
   
3. Overlapped, APCs, IOCP�Ƚ�:
   1). Overlapped
		���ReadFile,WriteFile�������һ��Overlapped,����hFile�ϵȴ�������ɡ��޷��������ĸ��������
		Overlapped.Offset = 1;
		ReadFile(hFile, &Overlapped);
		Overlapped.Offset = 2;
		ReadFile(hFile, &Overlapped);
		for (2) {
		   WaitForSingleObject(hFile);
		   GetOverlappedResult; //ȱ�����޷��������ĸ�����
		}

		Overlapped & Events������޷��������ĸ�������
		һ��ReadFileһ��Overlapped.hEvent,���ڶ��hEvent�ϵȴ�
		Overlapped[0].Offset = 1;
		Overlapped[0].hEvent = CreateEvent; gEvents[0] = Overlapped[0].hEvent;
		ReadFile(hFile, &Overlapped);
		Overlapped[0].Offset = 2;
		Overlapped[0].hEvent = CreateEvent; gEvents[1] = Overlapped[1].hEvent;
		ReadFile(hFile, &Overlapped);
		WaitForMultipleObjects(gEvents); // ����ֵ���������ĸ����������޷�Wait����64��Event
		GetOverlappedResult; 

	2). APCs������޷�Wait����64��Event��
		һ��ReadFileEx������Callback(�������),����Overlapped.hEvent���Ա���һ����ݱ��
		Callback������ʱ���Ի�֪��ݱ�ǡ�
		ע��һ��Read�������첽��ɺ�ֻ���ڳ�����alertable״̬(����WaitxxxEx����ʱ)�Ż����Callback,����Wait����WAIT_IO_COMPLETION

		��ҪOverlapped��
		����32KB�����ݶ�ȡOS��������ֱ�Ӵ���

		APCs�����⣿
		1. APCs��֧�ּ���API��listen(), WaitCommEvent
		2. ֻ�з���Overlapped������̲߳����ṩcallback

	3). IOCP��������������⣺
		1. ������Handle��
		2. һ���̷߳�������һ���߳����������ʱ����
		3. ֧��Scaleable�ܹ���select���У�
	   
4. IOCP��ɶ˿������Ӧ�ÿ��ǵ����⡣
   1).  IOCP ����ΰ�ȫ�ͷ���Դ.   
