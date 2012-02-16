// queue.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

//简单队列操作，用于支持AC算法
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
    if (front == 0) //空队列
        front = rear =  p;

    else {
        rear->link = p;
        rear = p;	
        // 将新结点加到后端，修改rear
    }
}


template <class Type> 
Type* Queue<Type>::Delete(Type& retvalue) 
{
    if (front == 0) 
    { 
        return 0;
    } 
    // 设构造函数将front初始化为0，表示空队列
    QueueNode<Type> *x = front; 
    retvalue = front->data; 
    front = x->link; 	// 删除前端结点
    delete x; 
    return &retvalue; 
}
