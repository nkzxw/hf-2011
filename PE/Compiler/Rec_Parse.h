#pragma once
#include "Grammar.h"
#include "Action.h"
using namespace std;


class Rec_Parse
{
public:
	Rec_Parse(Grammar *grammar,vector< set<symbol> > p,
		list<TOKEN> *tokenlist):g(grammar),Predict(p),tlist(tokenlist),pos(tlist->begin()),act(grammar)
	{
		cout<<"Recursive Parsing..."<<endl;
		Parse(g->start_symbol);
		if(pos == tlist->end()) cout<<"Cheer! Accept!"<<endl<<endl;
	}
	~Rec_Parse() {};
//	void print_table() {symbol_table.print_table();}

protected:
	list<TOKEN> *tlist;
	list<TOKEN>::iterator pos;
	Grammar *g;
	vector< set<symbol> > Predict;
	void Parse(symbol X);
	void error(list<TOKEN>::iterator pos,nonterminal X);

	//for semantic analyse
	list<TOKEN>::iterator token_pos;
	_Action act;
};
