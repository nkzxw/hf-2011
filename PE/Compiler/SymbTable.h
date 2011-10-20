#pragma once
#include "TypeDef.h"
using namespace std;

//本文件定义了符号表


//符号表的一项
typedef struct  symbtable
{
	char  * idName; //变量标识符
	AttributeIR  attrIR; //变量类型
	int Level; //变量所在层数
	int Off; //变量所在偏移
} SymTab;

class SymbTable
{
public:
	SymbTable():off(0),level(0) {};
	~SymbTable() {};
	//在符号表中插入一个类型为_attrIR,标识符为_idName的变量
	void insert(char * _idName, AttributeIR _attrIR); 
	void create(); //添加符号表
	void destroy(); //跳出符号表
	void print_table(); //打印符号表


protected:
	//使用scope栈的局部符号表方法中所用到的scope栈
	stack < int > scope;
	vector < SymTab > Table; //符号表
	bool find(char * _idName); //在全局符号表中查找符合条件的变量声明
	bool level_find(char * _idName); //在本层符号表中查找符合条件的变量声明
	
	int off; //在同层的变量偏移
	int level; //当前层数
};


