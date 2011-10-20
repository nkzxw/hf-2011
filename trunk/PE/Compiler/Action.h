#pragma once
#include "Grammar.h"
#include "SymbTable.h"
using namespace std;
//��_Action�ඨ���˶������Ŷ�Ӧ�Ķ���.



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
	//���뵱ǰTOKEN�Ͷ�������,������Ӧ����.
	void call_action(const symbol &act_sym, list<TOKEN>::iterator token_pos); 
	SymbTable symbtable; //���ű�

protected:
	stack<semantic_record> sem_stack; //����ջ
	Grammar *g;  //�ķ����ṩ�����ķ�.
	//����Ϊ��������
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
