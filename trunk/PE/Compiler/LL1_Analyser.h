#pragma once
#include "Grammar.h"
using namespace std;


class LL1_Analyser
{
public:
	LL1_Analyser(const Grammar &grammar);
	~LL1_Analyser() {} ;
	void show_first_set(); //��ʾFIRST��
	void show_follow_set(); //��ʾFOLLOW��
	void show_predict_set();  //��ʾPREDICT��
	void show_LL_Table();  //��ʾLL1������
	void show_derives_lambda(); //��ʾ���Ƴ��յķ��ռ�������
	map< pair<nonterminal,terminal>, int > LL_Table; //LL1������
	vector< set<symbol> > predict; //PREDICT��
protected:

	Grammar g; //���������ķ�
	map< symbol, set<symbol> > first; //FRIST��
	map< symbol, set<symbol> > follow; //FOLLOW��
	map< symbol, bool > derives_lambda; //���Ƴ��յķ��ռ�������

	void mark_lambda(); //�ҳ����Ƴ��յķ��ռ���
	set<symbol> compute_first (const vector<symbol> &right); //����һ������ʽ��FIRST��
	void fill_first_set (); //�������з��ռ�����FIRST��,������FIRST��MAP
	//�ڲ�����,�ҳ��Է��ռ���vtΪ��,��vtΪ�Ҳ���һ���ַ��Ĳ���ʽ�Ƿ����
	bool _find_production(const nonterminal &vn, const terminal &vt);
	void fill_follow_set ();//�������з��ռ�����FOLLOW��,������FOLLOW��MAP
	void fill_predict_set ();//�������в���ʽ��PREDICT��,������PREDCIT��MAP
	void fill_LL_Table();//����PREDICT��,����LL1������
};



