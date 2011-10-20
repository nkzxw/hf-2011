#pragma once
#include "TypeDef.h"
using namespace std;

//���ļ������˷��ű�


//���ű��һ��
typedef struct  symbtable
{
	char  * idName; //������ʶ��
	AttributeIR  attrIR; //��������
	int Level; //�������ڲ���
	int Off; //��������ƫ��
} SymTab;

class SymbTable
{
public:
	SymbTable():off(0),level(0) {};
	~SymbTable() {};
	//�ڷ��ű��в���һ������Ϊ_attrIR,��ʶ��Ϊ_idName�ı���
	void insert(char * _idName, AttributeIR _attrIR); 
	void create(); //��ӷ��ű�
	void destroy(); //�������ű�
	void print_table(); //��ӡ���ű�


protected:
	//ʹ��scopeջ�ľֲ����ű��������õ���scopeջ
	stack < int > scope;
	vector < SymTab > Table; //���ű�
	bool find(char * _idName); //��ȫ�ַ��ű��в��ҷ��������ı�������
	bool level_find(char * _idName); //�ڱ�����ű��в��ҷ��������ı�������
	
	int off; //��ͬ��ı���ƫ��
	int level; //��ǰ����
};


