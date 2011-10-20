//本文件定义了文法GRAMMAR类，它通过initializtion.h的信息建立文法的内部表示。

#pragma once
#include "TypeDef.h"
using namespace std;



class Grammar
{
public:
	Grammar();
	~Grammar() {};
	//查找wordtype类型的search字符串的内部表示
	symbol Search(const string& search,symbol_type wordtype); 
	//输入内部表示，返回相应的文本信息
	string GetStr(const symbol &sym);
	//固定查找
	symbol Static_Search(const string& search);
	//把产生式输出
	void Show_Productions();
	//返回产生式的右部，在语法分析中使用
	const vector<symbol>& GetRight(int n) { return productions[n].rhs; }
	//返回产生式的左部
	symbol GetLeft(int n) { return productions[n].lhs; }
	//返回非终极符的个数
	int Get_Num_Nonterminal() { return nonterminals.size(); }
	//返回第i个非终极符
	symbol Get_Nonterminal(int i) 
	{
		if ( i >= nonterminals.size() ) return make_pair(Default,0);
		symbols::iterator s_it=nonterminals.begin();
		advance (s_it,i);
		return s_it->second;
	}
	//存放产生式数组
	vector<production> productions;
	//存放终极符
	symbols terminals;
	//存放非终极符
	symbols nonterminals;
	//存放动作符号
	symbols actions;
	//开始符
	nonterminal start_symbol;
	//判定是否是非终极符
	bool is_nonterminal(const symbol &X);
	//判定是否是终极符
	bool is_terminal(const symbol &X);
	//判定是否是动作符号
	bool is_action_symbol(const symbol &X);
	//实现语法分析中的“匹配”功能
	bool match(const terminal &X, const terminal &Y);
protected:
	vector<string> Intliterals;  //常数表
	vector<string> id;	 //标识符表
	vector<string> Chars; //字符常量表
};
