#pragma once
#include "Grammar.h"
using namespace std;
//���ﶨ���˴ʷ�����

class Scanner
{
public:
	Scanner(Grammar *grammer);
	~Scanner() {};
	void scan();
	void printoken();
	list<TOKEN> tokenlist;
protected:
	FILE *fp;  //�ļ�ָ�룬����ָ��Ҫ������Դ����
	char cbuffer; //���ڶ�����ַ�
	int linenum; //������������
	int search(char searchchar[],int wordtype); //����
	char alphaprocess(char buffer);  //�ؼ��ֺͱ�ʶ�������Ӻ���
	char digitprocess(char buffer);   //���ִ�����
	char otherprocess(char buffer);    //�����ַ�������

	Grammar * g; //�ķ��ṩ�ռ���.
};
