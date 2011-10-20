#pragma once
#include "Grammar.h"
#include "Action.h"
#include "SymbTable.h"
using namespace std;

//此为LL1语法分析驱动器,可以通过文法,TOKEN序列和LL1分析表,判定语法是否正确,同时驱动动作.

class LL1_Recognizer
{
public:
	LL1_Recognizer(Grammar *grammar,map< pair<nonterminal,terminal>, int > table,
		list<TOKEN> *tokenlist):g(grammar),LL_Table(table),tlist(tokenlist),act(grammar)//,sym_table_num(0),temp_num(0)
	{
		cout<<"Recognizing..."<<endl;
		lldriver(); 
	}
	~LL1_Recognizer() {};
	void print_table() {act.symbtable.print_table();} //输出符号表
	
protected:
	list<TOKEN> *tlist; //TOKEN序列
	Grammar *g; //文法
	map< pair<nonterminal,terminal>, int > LL_Table; //LL1分析表
	void lldriver();  //LL1语法驱动器
	void error(list<TOKEN>::iterator pos,nonterminal X); //报告出错
	stack<symbol> sym_stack;  //语法分析栈
	//for semantic analyse
	list<TOKEN>::iterator token_pos; //指向当前分析的TOKEN的迭代器
	_Action act; //动作文法
};
