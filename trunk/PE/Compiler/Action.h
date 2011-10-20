#pragma once
#include "Grammar.h"
#include "SymbTable.h"
using namespace std;
//该_Action类定义了动作符号对应的动作.



class _Action
{
public:
	_Action(Grammar *grammar):g(grammar)
	{
		_intc.size = 1;
		_intc.kind = intTy;
		_charc.size = 1;
		_charc.kind = charTy;
	}
	~_Action() {};
	//输入当前TOKEN和动作符号,调用相应动作.
	void call_action(const symbol &act_sym, list<TOKEN>::iterator token_pos); 
	SymbTable symbtable; //符号表

protected:
	stack<semantic_record> sem_stack; //语义栈
	Grammar *g;  //文法类提供动作文法.
	//以下为各种语义
	void _ProgHead();
	void _Id(list<TOKEN>::iterator token_pos);
	void _BaseType(list<TOKEN>::iterator token_pos);
	void _Intc(list<TOKEN>::iterator token_pos);
	void _ArrayType();
	void _RecordType();
	void _TypeDec();
	void _FieldDec();
	void _Param();
	void _VarDec();
	void _ParamVar();
	void _AddLevel();
	void _SubLevel();

	TypeIR _intc;
	TypeIR _charc;

};
