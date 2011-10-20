#include "stdafx.h"
#include "Scanner.h"




char Scanner::alphaprocess(char buffer)  //�ؼ��ֺͱ�ʶ�������Ӻ���
{
	symbol atype;
	int i=-1;
	char alphatp[20];
	while ( (isalpha(buffer)) || (isdigit(buffer)) ) //�������������ĸ�����־������Խ�
	{
		alphatp[++i]=buffer;
		buffer=fgetc(fp);
	};
	alphatp[i+1]='\0';
	atype=g->Search(alphatp,Key); //�ڹؼ��ֱ��в���
	if (atype.first != Default)
		tokenlist.push_back(TOKEN(atype,linenum));
	else   //�Ҳ������Ǳ�ʶ��,���Ҳ�����ƫ��
	{
		atype=g->Search(alphatp,Id);
		tokenlist.push_back(TOKEN(atype,linenum));
	};
	return buffer;
}

char Scanner::digitprocess(char buffer)   //���ִ�����
{
	int i=0;
	char digittp[20];
	symbol dtype;
	while ((isdigit(buffer)))  //���������־������Խ�
	{
		digittp[i]=buffer;
		i++;
		buffer=fgetc(fp);
	}
	digittp[i]='\0';
	dtype=g->Search(digittp,Intc); //�ڱ������Ƿ��Ѿ�����,������ƫ��.
	tokenlist.push_back(TOKEN(dtype,linenum));
	return buffer;
}

char Scanner::otherprocess(char buffer)    //�����ַ�������
{
	int i=0;
	char othertp[20];
	symbol otype,ttype=make_pair(Default,0);
	char cbuffer=buffer;
	if ( buffer == '\n' || buffer == ' ' || buffer == '\t' ) //����ǻس�,�ո���Ʊ�λ������
	{
	   buffer=fgetc(fp);
	   return buffer;
	}
	//���������ĸ������,Ҳ���ǿո��س�,һ�����������
	while ( (!isdigit(buffer)) && (!isalpha(buffer)) && (buffer != ' ') && (buffer != '\n') )
	{
		othertp[i]=buffer;
		i++;
		othertp[i]='\0';
		//�ȳԽ�һ�����ǲ����������������Ż��ϵ����,�ҵ����ٳԽ�һ������(˫Ŀ)ֱ��û�ҵ�Ϊֹ.
		otype=g->Search(othertp,Arithmetic);
		if (otype.first == Default) break;
		else { ttype=otype; buffer=fgetc(fp); continue; }
		otype=g->Search(othertp,Relation);
		if (otype.first == Default) break;
		else { ttype=otype; continue; }
		otype=g->Search(othertp,Border);
		if (otype.first == Default) break;
		else { ttype=otype; buffer=fgetc(fp); continue; }
	}
	if (ttype.first != Default)
	{
		tokenlist.push_back(TOKEN(ttype,linenum));
		return buffer;
	}
	else //�������������������,��ʾ����.
	{
		cout<<othertp<<'('<<linenum<<") error,not a word\n";
		exit(1);
	}
	
}



Scanner::Scanner(Grammar *grammar)
{
	g=grammar;
	linenum=1;
	if ((fp=fopen(".\\demo.txt","r"))==NULL) //��E:\DEMO.TXT(Դ�����ļ�)
	{
		cerr<<"error,can't open the file!\n"<<endl;
		exit(1);
	}
	else
		scan(); //ɨ����ļ�.
}

void Scanner::scan()
{
	cout<<"Scanning..."<<endl<<endl;
	cbuffer = fgetc(fp);
	while (cbuffer!=EOF)
	{
		if (cbuffer == '\n') linenum++;
		if (isalpha(cbuffer)) //����ĸ���ȿ��ǲ��ǹؼ��ֻ��ʶ��
			cbuffer=alphaprocess(cbuffer);
		else if (isdigit(cbuffer))  //�����־��ȿ��ǲ�������
			cbuffer=digitprocess(cbuffer);
		else cbuffer=otherprocess(cbuffer); //������������������
	};
	printoken();
	cout<<endl;
};

void Scanner::printoken() //��ӡ����TOKEN����
{
	cout<<"Token list:"<<endl;
	list<TOKEN>::iterator itr=tokenlist.begin();
	while(itr != tokenlist.end()) 
	{
		cout.width(8);
		cout<<g->GetStr(itr->value);
		cout.width(5);
		cout<<itr->value.first<<setw(3)<<itr->value.second<<setw(3)<<itr->line<<endl; 
		itr++; 
	}
}
