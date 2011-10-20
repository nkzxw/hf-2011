#include "stdafx.h"
#include "LL1_Analyser.h"
//计算并集
template <class T> inline
set<T> Set_Union (const set<T> &lset, const set<T> &rset)
{
	set<T> temp;
	insert_iterator< set<T> > pos(temp, temp.begin());
	set_union (lset.begin(), lset.end(), rset.begin(), rset.end(), pos);
	return temp;
}
//计算差集
template <class T> inline
set<T> Set_Difference (const set<T> &lset, const set<T> &rset)
{
	set<T> temp;
	insert_iterator< set<T> > pos(temp, temp.begin());
	set_difference (lset.begin(), lset.end(), rset.begin(), rset.end(), pos);
	return temp;
}
//看value是否属于集合Set
template <class T> inline
bool Is_Attribute (const T &value, const set<T> &Set)
{
	return (Set.find(value) != Set.end());
}


LL1_Analyser::LL1_Analyser(const Grammar &grammar):g(grammar) 
{
	//首先删除所有文法中的动作符号
	for(int i=0; i < g.productions.size();i++)
		for(int j=0; j<g.productions[i].rhs.size(); j++)
			if (g.productions[i].rhs[j].first == Action) g.productions[i].rhs.erase(&(g.productions[i].rhs[j])); 
	g.actions.clear();
	cout<<"Analysing the grammar..."<<endl<<endl;
	//一步一步从FIRST集求到LL1分析表
	mark_lambda();
	fill_first_set(); 
	fill_follow_set();
	fill_predict_set();
	fill_LL_Table();
	show_LL_Table();
};

void LL1_Analyser::show_first_set()
{
	cout<<"First set:"<<endl;
	map< symbol, set<symbol> >::iterator pos=first.begin();
	for(; pos != first.end(); pos++)
	{
		cout<<g.GetStr(pos->first)<<" :  ";
		for(set<symbol>::iterator s_it=pos->second.begin(); s_it != pos->second.end(); s_it++)
			cout<<g.GetStr(*s_it)<<' ';
		cout<<endl;
	}
}

void LL1_Analyser::show_follow_set()
{
	cout<<"Follow set:"<<endl;
	map< symbol, set<symbol> >::iterator pos=follow.begin();
	for(; pos != follow.end(); pos++)
	{
		cout<<g.GetStr(pos->first)<<" :  ";
		for(set<symbol>::iterator s_it=pos->second.begin(); s_it != pos->second.end(); s_it++)
			cout<<g.GetStr(*s_it)<<' ';
		cout<<endl;
	}
}

void LL1_Analyser::show_predict_set()
{
	cout<<"Predict set:"<<endl;
	vector< set<symbol> >::iterator pos=predict.begin();
	for(; pos != predict.end(); pos++)
	{
		cout<<"Production "<<distance(predict.begin(),pos)+1<<" :  ";
		for(set<symbol>::iterator s_it=pos->begin(); s_it != pos->end(); s_it++)
			cout<<g.GetStr(*s_it)<<' ';
		cout<<endl;
	}
}

void LL1_Analyser::show_LL_Table()
{
	cout<<"LL1 Analyse Table:"<<endl;
	map< pair<nonterminal,terminal>, int >::iterator pos=LL_Table.begin();
	for(; pos != LL_Table.end(); pos++)
		cout<<g.GetStr(pos->first.first)<<' '<<g.GetStr(pos->first.second)<<" -> "<<pos->second+1<<endl;
	cout<<endl;
}

void LL1_Analyser::show_derives_lambda()
{
	cout<<"The following nonterminals can derive lambda."<<endl;
	map< symbol, bool >::iterator pos=derives_lambda.begin();
	for(; pos != derives_lambda.end(); pos++)
		if (pos->second == true) cout<<g.GetStr(pos->first)<<' ';
	cout<<endl;
}
//算法大意:从每个符号在产生式的左部出发,一步步向下推导出是否为空,直到该集合derives_lambda收敛为止
void LL1_Analyser::mark_lambda()
{
	bool changes;
	bool rhs_derives_lambda;
	symbol v;
	int i,j;
	//初始化,规定每个符号都推不出空
	symbols::iterator s_it=g.nonterminals.begin();
	for (;s_it != g.nonterminals.end(); s_it++)
		derives_lambda[s_it->second] = false;
	s_it=g.terminals.begin();
	for (;s_it != g.terminals.end(); s_it++)
		derives_lambda[s_it->second] = false;

	do
	{
		changes = false; //判定是否收敛,如果为false为收敛
		for (i=0; i < g.productions.size(); i++)
		{
			production &p=g.productions[i];
			if ( !derives_lambda[p.lhs] ) 
			{
				//如果产生式右部只有空则一定能推导出空
				if ( p.rhs.size() == 1 && p.rhs[0].first == $ )
				{
					changes = true;
					derives_lambda[p.lhs] = true;
					continue;
				};
				rhs_derives_lambda = derives_lambda[p.rhs[0]];
				//否则继续向下推导(即在原集合中查找右部能否推出空)
				for (j=1; j<p.rhs.size(); j++)
					rhs_derives_lambda = rhs_derives_lambda && derives_lambda[p.rhs[j]];
				if (rhs_derives_lambda) { changes=true; derives_lambda[p.lhs]=true; }
			}
		}
	} while (changes); //不收敛就循环
}
//算法大意:见公式
set<symbol> LL1_Analyser::compute_first (const vector<symbol> &right)
{
	int i,k=right.size();
	symbol n=make_pair($,0);
	set<symbol> temp,temp1;
	set<symbol> nl;
	nl.insert(n);
	set<symbol> result;
	if (k == 0) return result;
	else
	{
		result=Set_Union(result,first[right[0]]);
		for (i=1; i<k && ( Is_Attribute(n,first[right[i-1]]) ); i++)
			result=Set_Union(result,Set_Difference(first[right[i]],nl));
		if ( i==k && Is_Attribute(n,first[right[k-1]]) )
			result.insert(n);
	}
	return result;
}

bool LL1_Analyser::_find_production(const nonterminal &vn, const terminal &vt)
{
	vector<production>::iterator pos = g.productions.begin();
	while ( pos != g.productions.end() )
	{
		if (pos->lhs == vn && pos->rhs[0] == vt) return true;
		pos++;
	}
	return false;
}
//算法大意:见公式
void LL1_Analyser::fill_first_set ()
{
	nonterminal A;
	terminal a;
	production p;
	bool changes;
	int temp;
	nonterminal n=make_pair($,0);
	symbols::iterator s_it=g.nonterminals.begin();
	for (; s_it != g.nonterminals.end(); s_it++)
	{
		A = s_it->second;
		if (derives_lambda[A]) first[A].insert(n);
		else first[A].clear();
	}
	for (s_it=g.terminals.begin(); s_it != g.terminals.end(); s_it++)
	{
		a = s_it->second;
		if ( (a.first == Id || a.first == Intc) && (a.second != -1) ) continue;
		first[a].insert(a);
		for (symbols::iterator s_it1=g.nonterminals.begin(); s_it1 != g.nonterminals.end(); s_it1++)
		{
			A = s_it1->second;
			if( _find_production(A,a) ) 
				first[A].insert(a);
		}
	}
	do
	{
		changes = false;
		for (vector<production>::iterator pos=g.productions.begin(); pos != g.productions.end(); pos++)
		{
			p=*pos;
			temp=first[p.lhs].size();
			first[p.lhs]=Set_Union(first[p.lhs],compute_first(p.rhs));
			if ( temp != first[p.lhs].size() ) changes=true;
		}
	} while (changes);

}

//算法大意:见公式
void LL1_Analyser::fill_follow_set()
{
	nonterminal A,B;
	int i;
	bool changes;
	symbol n=make_pair($,0);
	set<symbol> temp_set;
	vector<symbol> follow_string;
	vector<symbol> temp1;
	vector<symbol>::iterator pos1;

	for (symbols::iterator s_it=g.nonterminals.begin(); s_it != g.nonterminals.end(); s_it++)
	{
		A = s_it->second;
		follow[A].clear();
	}
	follow[g.start_symbol].insert(n);
	
	do
	{
		changes = false;
		s_it=g.nonterminals.begin();
		for (; s_it != g.nonterminals.end(); s_it++)
		{
			B = s_it->second;
			i=follow[B].size();
			for (vector<production>::iterator pos=g.productions.begin(); pos != g.productions.end(); pos++)
			{
				pos1=find(pos->rhs.begin(),pos->rhs.end(),B);
				while( pos1 != pos->rhs.end() )
				{
					temp1.clear();
					temp1.insert(temp1.begin(),pos1+1,pos->rhs.end());
					temp_set=compute_first(temp1);
					if( Is_Attribute(n,temp_set) || (temp_set.size() == 0 && find(pos->rhs.begin(),pos->rhs.end(),B) != pos->rhs.end() ) )
					{
						temp_set.erase(n);
						follow[B]=Set_Union(follow[B],temp_set);
						follow[B]=Set_Union(follow[B],follow[pos->lhs]);
					}
					else 
						follow[B]=Set_Union(follow[B],temp_set);
					pos1=find(pos1+1,pos->rhs.end(),B);
				}
			}
			if (i != follow[B].size()) changes = true;
		}
	} while (changes);
}

//算法大意:见公式
void LL1_Analyser::fill_predict_set()
{
	symbol n=make_pair($,0);
	set<symbol> temp_set;
	for (vector<production>::iterator pos=g.productions.begin(); pos != g.productions.end(); pos++)
	{
		temp_set.clear();
		temp_set=compute_first(pos->rhs);
		if ( Is_Attribute(n,temp_set) || temp_set.size() == 0)
		{
			if ( Is_Attribute(n,temp_set) ) temp_set.erase(n);
			temp_set=Set_Union(temp_set,follow[pos->lhs]);
		}
		predict.push_back(temp_set);
	}
}

//算法大意:见公式
void LL1_Analyser::fill_LL_Table()
{
	pair<nonterminal,terminal> p_temp;
	set<symbol>::iterator s_it;
	symbol sym;
	for (int i=0; i<g.productions.size();i++)
	{
		sym=g.productions[i].lhs;
		s_it=predict[i].begin();
		while( s_it != predict[i].end() )
		{
			p_temp=make_pair(sym,*s_it);
			if ( LL_Table.find(p_temp) != LL_Table.end() )
			{
				cerr<<"The grammar isn't LL(1) grammar! because of "<<g.GetStr(p_temp.first)<<" and "<<g.GetStr(p_temp.second)<<endl;
				return;
			}
			LL_Table[p_temp]=i;
			s_it++;
		}
	}
}


