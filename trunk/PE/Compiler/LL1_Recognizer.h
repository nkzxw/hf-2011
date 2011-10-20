#pragma once
#include "Grammar.h"
#include "Action.h"
#include "SymbTable.h"
using namespace std;

//��ΪLL1�﷨����������,����ͨ���ķ�,TOKEN���к�LL1������,�ж��﷨�Ƿ���ȷ,ͬʱ��������.

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
	void print_table() {act.symbtable.print_table();} //������ű�
	
protected:
	list<TOKEN> *tlist; //TOKEN����
	Grammar *g; //�ķ�
	map< pair<nonterminal,terminal>, int > LL_Table; //LL1������
	void lldriver();  //LL1�﷨������
	void error(list<TOKEN>::iterator pos,nonterminal X); //�������
	stack<symbol> sym_stack;  //�﷨����ջ
	//for semantic analyse
	list<TOKEN>::iterator token_pos; //ָ��ǰ������TOKEN�ĵ�����
	_Action act; //�����ķ�
};
