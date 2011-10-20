#include "stdafx.h"
#include "Rec_Parse.h"

//����˼���LL1������һ��,��ѹջ�ĳɵ���,�ѵ�ջ�ĳɷ���.
//�Ѳ���LL1������ĳɲ���PREDICT��(��ʵ�Ĳ��Ķ�һ��.)
void Rec_Parse::Parse(symbol X)
{
	terminal a;
	int pnum;
	vector<symbol>::reverse_iterator v_it;


	a=pos->value;

	if( g->is_nonterminal(X) )
	{
		if(a.first == Id || a.first == Intc) a=make_pair(a.first,-1);
		for (pnum=0; pnum < Predict.size(); pnum++) //����PREDICT��
		{
			if( (g->productions[pnum].lhs == X) && ( Predict[pnum].find(a) != Predict[pnum].end() ) )
			{
				cout<<"Production "<<pnum+1<<endl;
				for (int n=0; n < g->productions[pnum].rhs.size(); n++)
					Parse(g->productions[pnum].rhs[n]); //�ݹ���õ�.
				return;
			}
			else
			{
				if ( pnum == Predict.size() - 1 ) error(pos,X);
				else continue;
			}
		}
	}
	else if (g->is_terminal(X) && (g->match(X,a) || X.first == $) )
	{
		token_pos = pos;
		if (X.first != $)
		{
			cout<<"Match: "<<g->GetStr(X)<<" , "<<g->GetStr(a)<<endl;
			pos++;
		}
		
		return;
	}
	else if ( g->is_action_symbol(X) )
	{
		cout<<"Call: "<<g->GetStr(X)<<endl;
		//Call Semantic Routine corresponding to X;
		act.call_action(X,token_pos);
		return;
	}
	else error(pos,X);
}

//�������LL1��������һ����.
void Rec_Parse::error(list<TOKEN>::iterator pos,nonterminal X)
{
	if(pos == tlist->end())
		cerr<<"Maybe lose "<<g->GetStr(X)<<endl;
	else 
		cerr<<"There's an error here: ("<<pos->line<<") "<<g->GetStr(pos->value)<<' '<<g->GetStr(X)<<endl;
	exit(1);
}


