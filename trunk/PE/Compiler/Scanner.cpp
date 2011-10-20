#include "stdafx.h"
#include "Scanner.h"




char Scanner::alphaprocess(char buffer)  //关键字和标识符处理子函数
{
	symbol atype;
	int i=-1;
	char alphatp[20];
	while ( (isalpha(buffer)) || (isdigit(buffer)) ) //如果是连续的字母或数字就连续吃进
	{
		alphatp[++i]=buffer;
		buffer=fgetc(fp);
	};
	alphatp[i+1]='\0';
	atype=g->Search(alphatp,Key); //在关键字表中查找
	if (atype.first != Default)
		tokenlist.push_back(TOKEN(atype,linenum));
	else   //找不到就是标识符,查找并给出偏移
	{
		atype=g->Search(alphatp,Id);
		tokenlist.push_back(TOKEN(atype,linenum));
	};
	return buffer;
}

char Scanner::digitprocess(char buffer)   //数字处理函数
{
	int i=0;
	char digittp[20];
	symbol dtype;
	while ((isdigit(buffer)))  //连续的数字就连续吃进
	{
		digittp[i]=buffer;
		i++;
		buffer=fgetc(fp);
	}
	digittp[i]='\0';
	dtype=g->Search(digittp,Intc); //在表中找是否已经存在,并给出偏移.
	tokenlist.push_back(TOKEN(dtype,linenum));
	return buffer;
}

char Scanner::otherprocess(char buffer)    //其他字符处理函数
{
	int i=0;
	char othertp[20];
	symbol otype,ttype=make_pair(Default,0);
	char cbuffer=buffer;
	if ( buffer == '\n' || buffer == ' ' || buffer == '\t' ) //如果是回车,空格或制表位就跳过
	{
	   buffer=fgetc(fp);
	   return buffer;
	}
	//如果不是字母或数字,也不是空格或回车,一定是特殊符号
	while ( (!isdigit(buffer)) && (!isalpha(buffer)) && (buffer != ' ') && (buffer != '\n') )
	{
		othertp[i]=buffer;
		i++;
		othertp[i]='\0';
		//先吃进一个看是不是运算符或特殊符号或关系符号,找到就再吃进一个查找(双目)直至没找到为止.
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
	else //如果以上条件都不符合,显示出错.
	{
		cout<<othertp<<'('<<linenum<<") error,not a word\n";
		exit(1);
	}
	
}



Scanner::Scanner(Grammar *grammar)
{
	g=grammar;
	linenum=1;
	if ((fp=fopen(".\\demo.txt","r"))==NULL) //打开E:\DEMO.TXT(源程序文件)
	{
		cerr<<"error,can't open the file!\n"<<endl;
		exit(1);
	}
	else
		scan(); //扫描该文件.
}

void Scanner::scan()
{
	cout<<"Scanning..."<<endl<<endl;
	cbuffer = fgetc(fp);
	while (cbuffer!=EOF)
	{
		if (cbuffer == '\n') linenum++;
		if (isalpha(cbuffer)) //是字母就先看是不是关键字或标识符
			cbuffer=alphaprocess(cbuffer);
		else if (isdigit(cbuffer))  //是数字就先看是不是数字
			cbuffer=digitprocess(cbuffer);
		else cbuffer=otherprocess(cbuffer); //其他情况考虑特殊符号
	};
	printoken();
	cout<<endl;
};

void Scanner::printoken() //打印整个TOKEN序列
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
