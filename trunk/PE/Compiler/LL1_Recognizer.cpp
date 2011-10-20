#include "stdafx.h"
#include "LL1_Recognizer.h"



void LL1_Recognizer::lldriver()
{
	sym_stack.push(g->start_symbol);
	cout<<"Push: "<<g->GetStr(g->start_symbol)<<endl;
	list<TOKEN>::iterator pos=tlist->begin(); //װ�뿪ʼ��
	terminal a; //TOKEN�е��ռ�����Ϣ
	nonterminal X; //����ʽ�е���Ӧ����
	pair<nonterminal,terminal> p_temp;
	int j;
	vector<symbol>::reverse_iterator v_it;
	if(pos == tlist->end()) return;
	while( !sym_stack.empty() ) //����ʱ����
	{
		a=pos->value;
		X=sym_stack.top();
		//��X�Ƿ��ռ���,ͨ��LL1���ҵ�(X,a)��Ӧ�Ĳ���ʽ������X,ѹ�����ʽ�Ҳ�.
		if( g->is_nonterminal(X) )  
		{
			if(a.first == Id || a.first == Intc) a=make_pair(a.first,-1);
			p_temp=make_pair(X,a);
			if( LL_Table.find(p_temp) == LL_Table.end() ) error(pos,X);
			else 
			{
				cout<<"Pop: "<<g->GetStr(X)<<endl;
				sym_stack.pop();
				j=LL_Table[p_temp];
				
				for(v_it=g->productions[j].rhs.rbegin();v_it != g->productions[j].rhs.rend(); v_it++)
				{ sym_stack.push(*v_it); cout<<"Push: "<<g->GetStr(*v_it)<<endl; }
			}
		}
		//��X���ռ���,����ǿ�����ƥ�䲢����,����ǿ���ֱ�ӵ���
		else if (g->is_terminal(X) && (g->match(X,a) || X.first == $) )
		{
			if (X.first == $) cout<<"Pop: $"<<endl;
			else cout<<"Match: "<<g->GetStr(X)<<" , "<<g->GetStr(a)<<endl;
			sym_stack.pop();
			token_pos = pos;
			if( X.first != $ ) pos++;
		}
		//���X�Ƕ�������,��������Ӧ�Ķ�������.
		else if ( g->is_action_symbol(X) )
		{
			cout<<"Call: "<<g->GetStr(X)<<endl;
			sym_stack.pop();
			//Call Semantic Routine corresponding to X;
			act.call_action(X,token_pos);
		}
		else error(pos,X);
	}
	//��TOKENָ��ָ���ļ�β,˵���﷨�����ɹ�.
	if(pos == tlist->end()) cout<<"Cheer! Accept!"<<endl<<endl;
	else error(pos,X);
}

void LL1_Recognizer::error(list<TOKEN>::iterator pos,nonterminal X)
{
	//���TOKEN���н���������δ����,����ʾ�ٵĲ���,������ʽ�е�X
	if(pos == tlist->end())
		cerr<<"Maybe lose "<<g->GetStr(X)<<endl; 
	else
		//���TOKEN����δ����,˵����ƥ��,����ƥ���(X,a)��ʾ.
		cerr<<"There's an error here: ("<<pos->line<<") "<<g->GetStr(pos->value)<<' '<<g->GetStr(X)<<endl;
	exit(1);
}


