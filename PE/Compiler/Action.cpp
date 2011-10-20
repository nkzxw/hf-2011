#include "stdafx.h"
#include "Action.h"


void _Action::call_action(const symbol &act_sym, list<TOKEN>::iterator token_pos)
{
	switch (act_sym.second)
	{
	case 0:
		_ProgHead();
		return;
	case 1:
		_Id(token_pos);
		return;
	case 2:
		_BaseType(token_pos);
		return;
	case 3:
		_Intc(token_pos);
		return;
	case 4:
		_ArrayType();
		return;
	case 5:
		_RecordType();
		return;
	case 6:
		_TypeDec();
		return;
	case 7:
		_FieldDec();
		return;
	case 8:
		_Param();
		return;
	case 9:
		_VarDec();
		return;
	case 10:
		_ParamVar();
		return;
	case 11:
		_AddLevel();
		return;
	case 12:
		_SubLevel();
		return;
	};
}


void _Action::_ProgHead()
{} 

//对标识符直接把内容压入语义栈
void _Action::_Id(list<TOKEN>::iterator token_pos)
{
	semantic_record record;
	record.record_kind = ID;
	record.sem_info.id_record=(char *) (g->GetStr(token_pos->value)).c_str();
	sem_stack.push(record);
	cout<<"Push "<<sem_stack.top().sem_info.id_record<<" in the semantic stack."<<endl;
}

//处理基本类型,把相应信息压入语义栈
void _Action::_BaseType(list<TOKEN>::iterator token_pos)
{
	semantic_record record;
	record.sem_info.type_record = new TypeIR;
	record.record_kind = TYPE;
	switch (token_pos->value.second)
	{
	case 19: //处理整型
		{
			record.sem_info.type_record->size = 1;
			record.sem_info.type_record->kind = intTy;
			cout<<"Push type of integer in the semantic stack."<<endl;
			break;
		}
	case 20: //处理字符
		{
			record.sem_info.type_record->size = 1;
			record.sem_info.type_record->kind = charTy;
			cout<<"Push type of char in the semantic stack."<<endl;
			break;
		}
	default:
		{
			cerr<<"This type is error!"<<endl;
			exit(1);
		}
	}
	sem_stack.push(record);
}


//处理整型常量,直接压入语义栈
void _Action::_Intc(list<TOKEN>::iterator token_pos)
{
	semantic_record record;
	record.record_kind = INTC;
	sscanf ((g->GetStr(token_pos->value)).c_str(), "%d", &record.sem_info.intc);
	sem_stack.push(record);

	cout<<"Push "<<sem_stack.top().sem_info.intc<<" in the semantic stack."<<endl;
}

//处理数组类型,从语义栈提取low,top,和元素类型信息,填入数组语义纪录中并压栈
void _Action::_ArrayType()
{
	semantic_record record;
	record.sem_info.type_record = new TypeIR;
	record.record_kind = TYPE;

	semantic_record b = sem_stack.top();
	sem_stack.pop();
	semantic_record t = sem_stack.top();
	sem_stack.pop();
	semantic_record l = sem_stack.top();
	sem_stack.pop();

	assert(l.record_kind == INTC);
	assert(t.record_kind == INTC);
	assert(b.record_kind == TYPE);
	assert(b.sem_info.type_record->size == 1);

	record.sem_info.type_record->kind = arrayTy;
	record.sem_info.type_record->More.ArrayAttr.low = l.sem_info.intc;
	record.sem_info.type_record->More.ArrayAttr.top = t.sem_info.intc;
	if (b.sem_info.type_record->kind == intTy)
		record.sem_info.type_record->More.ArrayAttr.elemTy = &_intc;
	else if (b.sem_info.type_record->kind == charTy)
		record.sem_info.type_record->More.ArrayAttr.elemTy = &_charc;
	record.sem_info.type_record->size = t.sem_info.intc - l.sem_info.intc + 1;
	sem_stack.push(record);

	cout<<"Push array type in the semantic stack."<<endl;
	cout<<"Top of the array is "<< record.sem_info.type_record->More.ArrayAttr.top<<endl;
	cout<<"Low of the array is "<< record.sem_info.type_record->More.ArrayAttr.low<<endl;
	cout<<"Size of the array is "<<record.sem_info.type_record->size <<endl;
	if (record.sem_info.type_record->More.ArrayAttr.elemTy->kind == intTy)
	cout<<"The type of element is integer."<<endl;
	if (record.sem_info.type_record->More.ArrayAttr.elemTy->kind == charTy)
	cout<<"The type of element is char."<<endl; 

}

//纪录类型,读出域,计算域中各变量的偏移和大小,计算总大小,填写语义纪录并压栈
void _Action::_RecordType()
{
	semantic_record record;
	record.record_kind = TYPE;
	record.sem_info.type_record = new TypeIR;
	record.sem_info.type_record->kind = recordTy;
	record.sem_info.type_record->size = 0;
	
	assert( sem_stack.top().record_kind == FIELD);
	record.sem_info.type_record->More.body = sem_stack.top().sem_info.field_record;
	struct field * pos = record.sem_info.type_record->More.body;
	
	for (int n=0;pos != NULL; pos = pos->next,n++) 
	{
		record.sem_info.type_record->size += pos->UnitType->size;//累加计算总大小
		pos->off=n;//累加填入偏移
	}
	sem_stack.pop();
	sem_stack.push(record);
		
	cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
	cout<<"Push field type in the semantic stack."<<endl;
	cout<<"This field is consisted by:"<<endl;
	for (pos = record.sem_info.type_record->More.body; pos!=NULL; pos = pos->next)
	{
		
		if (pos->UnitType->kind == intTy)
			cout<<"integer "<<pos->id<<";"<<" Off: "<<pos->off<<endl;
		else if (pos->UnitType->kind == charTy)
			cout<<"char "<<pos->id<<";"<<" Off: "<<pos->off<<endl;
		else if (pos->UnitType->kind == arrayTy)
		{
			cout<<"array [ "<<pos->UnitType->More.ArrayAttr.low;
			cout<<".."<<pos->UnitType->More.ArrayAttr.top<<" ] of ";
			if (pos->UnitType->More.ArrayAttr.elemTy->kind == intTy)
				cout<<"integer "<<pos->id<<";"<<" Off: "<<pos->off<<endl;
			else if (pos->UnitType->More.ArrayAttr.elemTy->kind == charTy)
				cout<<"char "<<pos->id<<";"<<" Off: "<<pos->off<<endl;
		}
	}
}



//没处理类型变量
void _Action::_TypeDec()
{

}


//处理域中各变量,先把变量标识符压栈保存,
//然后读出语义栈顶的类型信息,然后组装成域信息.
void _Action::_FieldDec()
{
	semantic_record record;
	record.record_kind = FIELD;
	
	stack <char*> ids;
	
	assert(sem_stack.top().record_kind == ID);
	while (sem_stack.top().record_kind == ID)
	{
		ids.push(sem_stack.top().sem_info.id_record);
		sem_stack.pop();
	}
	cout<<"********************************************************************"<<endl;

	assert(sem_stack.top().record_kind == TYPE);

	struct field * temp = new struct field;
	record.sem_info.field_record=temp;
	for (int i=0; i<ids.size()-1; i++) //域其实是个链表
	{
		temp->id = ids.top();
		ids.pop();
		temp->UnitType = sem_stack.top().sem_info.type_record;
		temp->next = new struct field;
		temp = temp->next;
	}
	if(ids.size() > 0)
	{
		temp->id = ids.top();
		ids.pop();
		temp->UnitType = sem_stack.top().sem_info.type_record;
		temp->next = NULL;
	}
	sem_stack.pop();
	
	if ( sem_stack.top().record_kind == FIELD )
	{
		struct field * pos = sem_stack.top().sem_info.field_record;
		while (pos->next != NULL) pos = pos->next;
		pos->next = record.sem_info.field_record;
	}
	else sem_stack.push(record);

	cout<<"Add part of field decribitions in the semantic stack."<<endl;
	cout<<"This part is consisted by:"<<endl;
	for (struct field *pos = record.sem_info.field_record; pos!=NULL; pos = pos->next)
	{
		if (pos->UnitType->kind == intTy)
			cout<<"integer "<<pos->id<<";"<<endl;
		else if (pos->UnitType->kind == charTy)
			cout<<"char "<<pos->id<<";"<<endl;
		else if (pos->UnitType->kind == arrayTy)
		{
			cout<<"array [ "<<pos->UnitType->More.ArrayAttr.low;
			cout<<".."<<pos->UnitType->More.ArrayAttr.top<<" ] of ";
			if (pos->UnitType->More.ArrayAttr.elemTy->kind == intTy)
				cout<<"integer "<<pos->id<<";"<<endl;
			else if (pos->UnitType->More.ArrayAttr.elemTy->kind == charTy)
				cout<<"char "<<pos->id<<";"<<endl;
		}
	}
}

//处理变量声明,思想和域处理一样,只是不须要组装,直接把变量信息压入语义栈里即可.
void _Action::_VarDec()
{
	stack <char*> ids;
	
	assert(sem_stack.top().record_kind == ID);
	while (sem_stack.top().record_kind == ID)
	{
		ids.push(sem_stack.top().sem_info.id_record);
		sem_stack.pop();
	}

	assert(sem_stack.top().record_kind == TYPE);
	AttributeIR attribute;
	attribute.idtype = sem_stack.top().sem_info.type_record;
	attribute.kind = varKind;
	attribute.More.VarAttr.isParam = false;
	
	while(!ids.empty())
	{
		symbtable.insert(ids.top(),attribute);
	    ids.pop();
	}

}


//处理值参声明,和处理变量声明一样,只是需要标明是值参
void _Action::_Param()
{
	stack <char*> ids;
	
	assert(sem_stack.top().record_kind == ID);
	while (sem_stack.top().record_kind == ID)
	{
		ids.push(sem_stack.top().sem_info.id_record);
		sem_stack.pop();
	}

	assert(sem_stack.top().record_kind == TYPE);
	AttributeIR attribute;
	attribute.idtype = sem_stack.top().sem_info.type_record;
	attribute.kind = varKind;
	attribute.More.VarAttr.isParam = true; //标明是参数
	attribute.More.VarAttr.access = dir; //是值参
		
	while(!ids.empty())
	{
		symbtable.insert(ids.top(),attribute);
		ids.pop();
	}
}

//处理值参声明,和处理变量声明一样,只是需要标明是变参
void _Action::_ParamVar()
{
	stack <char*> ids;
	
	assert(sem_stack.top().record_kind == ID);
	while (sem_stack.top().record_kind == ID)
	{
		ids.push(sem_stack.top().sem_info.id_record);
		sem_stack.pop();
	}

	assert(sem_stack.top().record_kind == TYPE);
	AttributeIR attribute;
	attribute.idtype = sem_stack.top().sem_info.type_record;
	attribute.kind = varKind;
	attribute.More.VarAttr.isParam = true;
	attribute.More.VarAttr.access = indir;
		
	while(!ids.empty())
	{
		symbtable.insert(ids.top(),attribute);
		ids.pop();
	}

}

//符号表层数加一
void _Action::_AddLevel()
{
	symbtable.create();
}

//符号表层数减一
void _Action::_SubLevel()
{
	symbtable.destroy();
}

