// queue.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

//�򵥶��в���������֧��AC�㷨
#include <assert.h>

template <class Type> class Queue;

template <class Type>
class QueueNode
{
    friend class Queue<Type>;
public:
    QueueNode(const Type& d,QueueNode *a=0):data(d),link(a){}
private:
    Type data;
    QueueNode *link;
};

template <class Type>
class Queue
{
public:
    Queue()
    {
        front=rear=0;
    }
    ~Queue();
    void Add(const Type& y);
    Type* Delete(Type&);
    bool IsEmpty()
    {
        return !front;
    }
private:
    QueueNode<Type> *front,*rear;
};

template <class Type>
Queue<Type>::~Queue()
{
    QueueNode<Type> *p=front;
    while(p)
    {
        p=p->link;
        delete front;
        front=p;
    }
}
template <class Type> 
void Queue<Type>::Add(const Type& y) 
{
    QueueNode<Type> *p=new QueueNode<Type>(y, 0);
    assert(p);
    if (front == 0) //�ն���
        front = rear =  p;

    else {
        rear->link = p;
        rear = p;	
        // ���½��ӵ���ˣ��޸�rear
    }
}


template <class Type> 
Type* Queue<Type>::Delete(Type& retvalue) 
{
    if (front == 0) 
    { 
        return 0;
    } 
    // �蹹�캯����front��ʼ��Ϊ0����ʾ�ն���
    QueueNode<Type> *x = front; 
    retvalue = front->data; 
    front = x->link; 	// ɾ��ǰ�˽��
    delete x; 
    return &retvalue; 
}
