
#include "stdafx.h"
#include "Grammar.h"

#include "Initializtion.h"



Grammar::Grammar()
{
	cout<<"Initializing the grammer..."<<endl<<endl;
	//initialize terminals
	int i;
	//�ѹؼ��֣����Ŷ����뵽�ռ���ӳ���У����������ǵ��ڲ���ʾsymbol
	for (i=0; i<keynum; i++)
		terminals[key[i]]=make_pair(Key,i); //����ؼ���
	for (i=0; i<bordernum; i++)
		terminals[border[i]]=make_pair(Border,i); //�����������
	for (i=0; i<arithnum; i++)
		terminals[arithmetic[i]]=make_pair(Arithmetic,i); //������ѧ����
	for (i=0; i<relnum; i++)
		terminals[relation[i]]=make_pair(Relation,i); //������ϵ����

	terminals["INTC"]=make_pair(Intc,-1); //ΪINTC�����ڲ���ʾ
	terminals["CHAR"]=make_pair(Charc,-1);//ΪCHAR�����ڲ���ʾ
	terminals["ID"]=make_pair(Id,-1);     //ΪID�����ڲ���ʾ
	terminals["$"]=make_pair($,0); //"$" means null��ΪNULL�����ڲ���ʾ
	terminals["EOF"]=make_pair(Eof,0);//Ϊ�ļ������������ڲ���ʾ
	
	//initialize nonterminals
	start_symbol=make_pair(Vn,0);
	for (i=0; i<Vnum; i++)
		nonterminals[vn[i]]=make_pair(Vn,i);

	//initialize actions
	for (i=0; i<actionum; i++)
		actions[action[i]]=make_pair(Action,i);

	//initialize productions
	symbol sym;
	production temp;
	int t;
	for(i=0; i<num_prods; i++)
	{
		sym=Static_Search(ProdStr[i][0]);
		if (sym.first == Vn) temp.lhs=sym;  //����ʽ���󲿱����Ƿ��ռ���
		else 
		{
			cerr<<"the left of production isn't nontermial!"<<endl;
			cerr<<ProdStr[i][0]<<endl;
			exit(1);
		}
		for (t=1; ProdStr[i][t].compare("\n") ; t++) 
		{
			sym=Static_Search(ProdStr[i][t]);
			if (sym.first == Default)
			{
				cerr<<"this word is error: "<<ProdStr[i][t]<<endl;
				exit(1);
			}
			else temp.rhs.push_back(sym);
		}
		productions.push_back(temp);
		temp.rhs.clear();
	}
	Show_Productions();
}

void Grammar::Show_Productions()
{
	cout<<"The productions:"<<endl;
	vector<production>::iterator pos=productions.begin();
	vector<symbol>::iterator tp;
	while(pos != productions.end())
	{
		cout<<distance(productions.begin(),pos)+1;
		cout.width(20);
		cout<<GetStr(pos->lhs)<<"  ->  ";
		tp=pos->rhs.begin();
		while (tp != pos->rhs.end())
		{
			cout<<GetStr(*tp)<<' ';
			tp++;
		}
		cout<<endl;
		pos++;
	}
	cout<<endl;
}

symbol Grammar::Static_Search(const string& search)
{
	symbols::iterator pos=nonterminals.find(search);
	if ( pos != nonterminals.end() ) return pos->second;
	else 
	{
		pos=terminals.find(search);
		if ( pos != terminals.end() ) return pos->second;
		else 
		{
			pos=actions.find(search);
			if ( pos != terminals.end() ) return pos->second;
			else return make_pair(Default,0);
		}
	}
	return make_pair(Default,0);
}

bool Grammar::is_nonterminal(const symbol &X)
{
	if( X.first == Vn ) return true;
	else return false;
}

bool Grammar::is_terminal(const symbol &X)
{
	switch(X.first)
	{
	case Key:
	case Border:
	case Arithmetic:
	case Relation:
	case Intc:
	case Id:
	case $:
	case Charc:
	case Eof:
		return true;
	case Vn:
	case Action:
	case Default:
	default:
		return false;
	};
}

bool Grammar::is_action_symbol(const symbol &X)
{
	if (X.first == Action) return true;
	else return false;
}


bool Grammar::match(const terminal &X, const terminal &Y)
{
	if(X.first != Y.first) return false; //ƥ�����ͱ�����ͬ
	else if( X.second == Y.second) return true; //����ƫ�Ʊ�����ͬ
	//����ʶ�������ͳ������⣬����ֻҪ���ͷֱ��"ID","INTC"��ͬ����.
	else if( ((X.first == Id) || (X.first == Intc)) ) return true; 
	else return false;
}

	
	

symbol Grammar::Search(const string& search,symbol_type wordtype)
{
	symbols::iterator pos;
	int i=0;
	switch (wordtype) 
	{
	case Vn:
		{
			pos=nonterminals.find(search);
			if ( pos != nonterminals.end() ) return pos->second;
			else return make_pair(Default,0);
		};
	case Key: //���ҹؼ���
	case Border:
	case Arithmetic:
	case Relation:
		{
			pos=terminals.find(search);
			if ( pos != terminals.end() ) return pos->second;
			else return make_pair(Default,0);
		};
	case Intc: //���ҳ�����
		{
			pos=terminals.find(search);
			if ( pos != terminals.end() ) return pos->second;
			else 
			{
				terminals[search]=make_pair(Intc,Intcnum);
				Intliterals.push_back(search);
				return make_pair(Intc,Intcnum++);
			}
		};
	case Charc: //�����ַ���
		{
			pos=terminals.find(search);
			if ( pos != terminals.end() ) return pos->second;
			else 
			{
				terminals[search]=make_pair(Charc,Charcnum);
				Intliterals.push_back(search);
				return make_pair(Charc,Charcnum++);
			}
		};
	case Id:  //���ұ�ʶ����
		{
			pos=terminals.find(search);
			if ( pos != terminals.end() ) return pos->second;
			else 
			{
				terminals[search]=make_pair(Id,idnum);
				id.push_back(search);
				return make_pair(Id,idnum++);
			}
		};
	default:
		   return make_pair(Default,0);
	}
}

string Grammar::GetStr(const symbol &sym)
{
	if (sym.second == -1) return name[(int)sym.first];
	else 
	switch(sym.first)
	{
	case Vn:
		return vn[sym.second];
	case Key:
		return key[sym.second];
	case Border:
		return border[sym.second];
	case Arithmetic:
		return arithmetic[sym.second];
	case Relation:
		return relation[sym.second];
	case Intc:
		return Intliterals[sym.second];
	case Charc:
		return Chars[sym.second];
	case Id:
		return id[sym.second];
	case $:
		return "$";
	case Action:
		return action[sym.second];
	case Eof:
		return "EOF";
	case Default:
		return "Sorry,it's Fail!";
	default:
		return NULL;
	}
}
