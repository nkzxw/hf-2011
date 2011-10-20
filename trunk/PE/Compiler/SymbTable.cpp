#include "stdafx.h"
#include "SymbTable.h"


//�����㷨��Ҫ�ǰ����ͺͱ�ʶ���ϲ���һ�����ű������Ϣ������.
void SymbTable::insert(char * _idName, AttributeIR _attrIR)
{
	switch(_attrIR.kind)
	{
	case varKind:
		{
			if ( level_find(_idName) ) 
			{
				cerr<<_idName<<" has been decirbe!"<<" level: "<<level<<", offset: "<<off<<endl;
				exit(1);
			}
			else
				cout<<_idName<<" is inserted,"<<" level: "<<level<<", offset: "<<off<<endl;
			
			_attrIR.More.VarAttr.level = level;
			_attrIR.More.VarAttr.off = off;
			SymTab temp;
			temp.attrIR = _attrIR;
			temp.idName = _idName;
			temp.Level = level;
			temp.Off = off;
			Table.push_back(temp);
			off++;
			break;
		}
//�����ͱ�������������û����
	case typeKind:
		break;
	case procKind:
		break;
	}
}

//��ȫ�ַ��ű��в��ұȵ�ǰ����С�Ĳ����Ƿ��Ѿ��б�������.
bool SymbTable::find(char * _idName)
{
	if (Table.empty()) return false;
	vector < SymTab >::iterator S_p = Table.begin();
	for(; S_p != Table.end(); S_p++ ) 
	{ 
		if ( !strcmp(S_p->idName,_idName) ) break;
		cout<<strcmp(S_p->idName,_idName)<<' '<<S_p->idName<<' '<<_idName<<endl;
	}
	if ( S_p == Table.end() ) return false;
	else 
		if ( S_p->Level > level ) return false;
		else return true;
}

//�ڱ�����ű��в����Ƿ����б�������
bool SymbTable::level_find(char * _idName)
{
	if (Table.empty()) return false;
	vector < SymTab >::iterator S_p = Table.begin();
	for(; S_p != Table.end(); S_p++ ) 
		if ( !strcmp(S_p->idName,_idName) && S_p->Level == level ) break;
	if ( S_p == Table.end() ) return false;
	else return true;
}

//��ӷ��ű�,ֻ�Ǽ򵥵Ĳ�����һ,Ȼ����ϲ�ƫ�Ʊ�����SOCPEջ��,ƫ����0
void SymbTable::create() 
{ 
	level++; 
	scope.push(off);
	off = 0; 
}

//�������ű�,ֻ�Ǽ򵥵Ĳ�����һ,Ȼ����ϲ�ƫ�ƴ�SOCPEջ�е������ָ�.
void SymbTable::destroy()
{
	level--;
	off = scope.top();
	scope.pop();
}

//��ʾ�����е���Ϣ
void Show_TypeIR (typeIR * type)
{
	cout<<"Size: "<<type->size<<" Kind: ";
	switch(type->kind)
	{
	case intTy:
		{
			cout<<"integer"<<endl;
			break;
		}
	case charTy:
		{
			cout<<"char"<<endl;
			break;
		}
	case arrayTy:
		{
			cout<<"array"<<endl;
			cout<<"low: "<<type->More.ArrayAttr.low<<" top: "<<type->More.ArrayAttr.top<<endl;
			cout<<"The infomation of element: "<<endl;
			Show_TypeIR(type->More.ArrayAttr.elemTy);
			break;
		}
	case recordTy:
		{
			cout<<"record"<<endl;
			cout<<"field is consisted of"<<endl;
			struct field *head = type->More.body;
			while ( head != NULL)
			{
				Show_TypeIR(head->UnitType);
				cout<<"Name: "<<head->id<<" Offset: "<<head->off<<endl;
				head = head->next;
			}
			break;
		}
	case boolTy:
		{
			cout<<"bool"<<endl;
			break;
		}
	}
}

//��ӡ������.
void SymbTable::print_table()
{
	
	vector< SymTab >::iterator S_it = Table.begin();
	for (; S_it != Table.end(); S_it++)
	{
		cout<<endl<<"+++++++++++++++++++++++++++++++++++++"<<endl;
		cout<<"Level: "<<S_it->Level<<", Offset: "<<S_it->Off<<' ';
		cout<<"Name: "<<S_it->idName<<endl<<"Type: ";
		switch (S_it->attrIR.kind)
		{
		case typeKind:
			{
				break;
			}
		case varKind:
			{
				cout<<"var"<<endl;
				cout<<"The infomation of this var is "<<endl;
				Show_TypeIR(S_it->attrIR.idtype);
				if(S_it->attrIR.More.VarAttr.isParam)
				{			
					if(S_it->attrIR.More.VarAttr.access == dir)
						cout<<"This var is direct parameter."<<endl;
					else cout<<"This var is direct parameter."<<endl;
				}
				else cout<<"This var is not a partameter."<<endl;
			}
		case procKind:
			{
				break;
			}
		}
		
	}
}
