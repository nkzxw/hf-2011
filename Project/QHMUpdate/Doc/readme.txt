1. 完成端口的例子在细节上有很多方法:
例如accept, AcceptEx之类，对于accept的处理尤其多。
我觉得只要把握住几个要点就行了：异步操作，结果队列，数据的传送（提交操作时传进去，查询时取出来），工作者线程

2. 同步I/O & 异步I/O
   1). 同步I/O: 对于同一个I/O对象句柄在同一时刻只允许一个I/O操作.
   2). 异步I/O: 支持对同一个I/O对象的并行处理.
   3). 异步I/O种类(即排队处理I/O对象):
	   重叠I/O: 同一个线程可以对多个I/O对象进行I/O操作，不同的线程也可以对同一个I/O对象进行操作.
	   异步过程调用（APC），扩展I/O: 每当一个IO操作结束时会产生一个完成信息，如果该IO操作有完成例程的话就添加到完成例程队列。一旦调用线程进入可变等待状态，就会依次执行队列中的完成例程。
	   使用完成端口（IOCP）
	   
   重叠io的使用：
   为了解决I/O阻塞阻塞这个问题，windows引进了重叠io的概念.
   如果在createfile的时候设置了file_flag_overlapped ，那么在调用readfile和writefile的时候就可以给他们最后一个参数传递一个overlapped结构。
   这样readfile或者writefile的调用马上就会返回，这时候你可以去做你要做的事，系统会自动替你完成readfile或者writefile,在你调用了readfile或者writefile后，
   你继续做你的事，系统同时也帮你完成readfile或writefile的操作，这就是所谓的重叠。
   使用重叠io还有一个好处，就是你可以同时发出几个readfile或者writefile的调用，
   然后用waitforsingleobject或者waitformultipleobjects来等待操作系统的操作完成通知，在得到通知信号后，就可以用getoverlappedresult来查询io调用的结果。  
   
   
3. Overlapped, APCs, IOCP比较:
   1). Overlapped
		多个ReadFile,WriteFile操作配合一个Overlapped,并在hFile上等待操作完成。无法区分是哪个操作完成
		Overlapped.Offset = 1;
		ReadFile(hFile, &Overlapped);
		Overlapped.Offset = 2;
		ReadFile(hFile, &Overlapped);
		for (2) {
		   WaitForSingleObject(hFile);
		   GetOverlappedResult; //缺点是无法区分是哪个操作
		}

		Overlapped & Events（解决无法区分是哪个操作）
		一个ReadFile一个Overlapped.hEvent,并在多个hEvent上等待
		Overlapped[0].Offset = 1;
		Overlapped[0].hEvent = CreateEvent; gEvents[0] = Overlapped[0].hEvent;
		ReadFile(hFile, &Overlapped);
		Overlapped[0].Offset = 2;
		Overlapped[0].hEvent = CreateEvent; gEvents[1] = Overlapped[1].hEvent;
		ReadFile(hFile, &Overlapped);
		WaitForMultipleObjects(gEvents); // 返回值决定了是哪个操作，但无法Wait超过64个Event
		GetOverlappedResult; 

	2). APCs解决（无法Wait超过64个Event）
		一个ReadFileEx关联到Callback(完成例程),参数Overlapped.hEvent可以保存一个身份标记
		Callback被呼叫时可以获知身份标记。
		注意一个Read操作被异步完成后只有在程序处于alertable状态(呼叫WaitxxxEx函数时)才会呼叫Callback,并且Wait返回WAIT_IO_COMPLETION

		需要Overlapped吗？
		少于32KB的数据读取OS几乎总是直接处理。

		APCs的问题？
		1. APCs不支持几个API：listen(), WaitCommEvent
		2. 只有发出Overlapped请求的线程才能提供callback

	3). IOCP解决以上所有问题：
		1. 不限制Handle数
		2. 一个线程发起请求，一个线程在请求完成时服务
		3. 支持Scaleable架构（select则不行）
	   
4. IOCP完成端口设计中应该考虑的问题。
   1).  IOCP 中如何安全释放资源.   
