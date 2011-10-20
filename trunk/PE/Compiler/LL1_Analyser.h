#pragma once
#include "Grammar.h"
using namespace std;


class LL1_Analyser
{
public:
	LL1_Analyser(const Grammar &grammar);
	~LL1_Analyser() {} ;
	void show_first_set(); //显示FIRST集
	void show_follow_set(); //显示FOLLOW集
	void show_predict_set();  //显示PREDICT集
	void show_LL_Table();  //显示LL1分析表
	void show_derives_lambda(); //显示能推出空的非终极符集合
	map< pair<nonterminal,terminal>, int > LL_Table; //LL1分析表
	vector< set<symbol> > predict; //PREDICT集
protected:

	Grammar g; //待分析的文法
	map< symbol, set<symbol> > first; //FRIST集
	map< symbol, set<symbol> > follow; //FOLLOW集
	map< symbol, bool > derives_lambda; //能推出空的非终极符集合

	void mark_lambda(); //找出能推出空的非终极符
	set<symbol> compute_first (const vector<symbol> &right); //计算一个产生式的FIRST集
	void fill_first_set (); //计算所有非终极符的FIRST集,并填入FIRST的MAP
	//内部函数,找出以非终极符vt为左部,以vt为右部第一个字符的产生式是否存在
	bool _find_production(const nonterminal &vn, const terminal &vt);
	void fill_follow_set ();//计算所有非终极符的FOLLOW集,并填入FOLLOW的MAP
	void fill_predict_set ();//计算所有产生式的PREDICT集,并填入PREDCIT的MAP
	void fill_LL_Table();//根据PREDICT集,计算LL1分析表
};



