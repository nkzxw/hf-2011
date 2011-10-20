#pragma once
#include "Grammar.h"
using namespace std;
//这里定义了词法分析

class Scanner
{
public:
	Scanner(Grammar *grammer);
	~Scanner() {};
	void scan();
	void printoken();
	list<TOKEN> tokenlist;
protected:
	FILE *fp;  //文件指针，用于指向要分析的源程序
	char cbuffer; //现在读入的字符
	int linenum; //分析到的行数
	int search(char searchchar[],int wordtype); //查找
	char alphaprocess(char buffer);  //关键字和标识符处理子函数
	char digitprocess(char buffer);   //数字处理函数
	char otherprocess(char buffer);    //其他字符处理函数

	Grammar * g; //文法提供终极符.
};
