

//显示进程信息
VOID Display()
{
	ProcessInfo  *q;  
	int j=0;
	//获取进程的ID和进程名
	for(p=head;p;p=p->next)
	{
		p->pid=*(int *)(p->addr+UNIQUEPROCESSID);
    	strcpy(p->name,(UCHAR *)(p->addr+IMAGEFILENAME));
	}
	for(p=head;p;p=p->next)
	{
		j++;
		DbgPrint("process 0x%08X,%d,%s\n",p->addr,p->pid,p->name);
	}
	DbgPrint("total number is %d",j);
	//释放链表
	p=head;
	q=p->next;
	while(q!=NULL)
	{
		ExFreePool(p);
		p=q;
		q=p->next;
	}
	ExFreePool(p);
}
