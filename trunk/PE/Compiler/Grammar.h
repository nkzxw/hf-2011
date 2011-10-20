//���ļ��������ķ�GRAMMAR�࣬��ͨ��initializtion.h����Ϣ�����ķ����ڲ���ʾ��

#pragma once
#include "TypeDef.h"
using namespace std;



class Grammar
{
public:
	Grammar();
	~Grammar() {};
	//����wordtype���͵�search�ַ������ڲ���ʾ
	symbol Search(const string& search,symbol_type wordtype); 
	//�����ڲ���ʾ��������Ӧ���ı���Ϣ
	string GetStr(const symbol &sym);
	//�̶�����
	symbol Static_Search(const string& search);
	//�Ѳ���ʽ���
	void Show_Productions();
	//���ز���ʽ���Ҳ������﷨������ʹ��
	const vector<symbol>& GetRight(int n) { return productions[n].rhs; }
	//���ز���ʽ����
	symbol GetLeft(int n) { return productions[n].lhs; }
	//���ط��ռ����ĸ���
	int Get_Num_Nonterminal() { return nonterminals.size(); }
	//���ص�i�����ռ���
	symbol Get_Nonterminal(int i) 
	{
		if ( i >= nonterminals.size() ) return make_pair(Default,0);
		symbols::iterator s_it=nonterminals.begin();
		advance (s_it,i);
		return s_it->second;
	}
	//��Ų���ʽ����
	vector<production> productions;
	//����ռ���
	symbols terminals;
	//��ŷ��ռ���
	symbols nonterminals;
	//��Ŷ�������
	symbols actions;
	//��ʼ��
	nonterminal start_symbol;
	//�ж��Ƿ��Ƿ��ռ���
	bool is_nonterminal(const symbol &X);
	//�ж��Ƿ����ռ���
	bool is_terminal(const symbol &X);
	//�ж��Ƿ��Ƕ�������
	bool is_action_symbol(const symbol &X);
	//ʵ���﷨�����еġ�ƥ�䡱����
	bool match(const terminal &X, const terminal &Y);
protected:
	vector<string> Intliterals;  //������
	vector<string> id;	 //��ʶ����
	vector<string> Chars; //�ַ�������
};
