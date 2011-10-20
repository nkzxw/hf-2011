#include "stdafx.h"
#include "LL1_Recognizer.h"



void LL1_Recognizer::lldriver()
{
	sym_stack.push(g->start_symbol);
	cout<<"Push: "<<g->GetStr(g->start_symbol)<<endl;
	list<TOKEN>::iterator pos=tlist->begin(); //装入开始符
	terminal a; //TOKEN中的终极符信息
	nonterminal X; //产生式中的相应符号
	pair<nonterminal,terminal> p_temp;
	int j;
	vector<symbol>::reverse_iterator v_it;
	if(pos == tlist->end()) return;
	while( !sym_stack.empty() ) //表不空时工作
	{
		a=pos->value;
		X=sym_stack.top();
		//当X是非终极符,通过LL1表找到(X,a)对应的产生式并弹出X,压入产生式右部.
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
		//当X是终极符,如果非空则检查匹配并弹出,如果是空则直接弹出
		else if (g->is_terminal(X) && (g->match(X,a) || X.first == $) )
		{
			if (X.first == $) cout<<"Pop: $"<<endl;
			else cout<<"Match: "<<g->GetStr(X)<<" , "<<g->GetStr(a)<<endl;
			sym_stack.pop();
			token_pos = pos;
			if( X.first != $ ) pos++;
		}
		//如果X是动作符号,则运行相应的动作函数.
		else if ( g->is_action_symbol(X) )
		{
			cout<<"Call: "<<g->GetStr(X)<<endl;
			sym_stack.pop();
			//Call Semantic Routine corresponding to X;
			act.call_action(X,token_pos);
		}
		else error(pos,X);
	}
	//当TOKEN指针指向文件尾,说明语法分析成功.
	if(pos == tlist->end()) cout<<"Cheer! Accept!"<<endl<<endl;
	else error(pos,X);
}

void LL1_Recognizer::error(list<TOKEN>::iterator pos,nonterminal X)
{
	//如果TOKEN序列结束而分析未结束,则显示少的部分,即产生式中的X
	if(pos == tlist->end())
		cerr<<"Maybe lose "<<g->GetStr(X)<<endl; 
	else
		//如果TOKEN序列未结束,说明不匹配,将不匹配的(X,a)显示.
		cerr<<"There's an error here: ("<<pos->line<<") "<<g->GetStr(pos->value)<<' '<<g->GetStr(X)<<endl;
	exit(1);
}


