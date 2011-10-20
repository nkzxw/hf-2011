//���ļ�����Ӵʷ����������е��ڲ���ʾ

#pragma once
using namespace std;

//������ŵ����ͣ�ע�⣬�������ݺ�name������һ�µġ���
typedef enum {Vn,Key,Border,Arithmetic,Relation,Intc,Charc,Id,$,Action,Default,Eof} symbol_type;
//������ŵĽṹ�������ͺ��ڱ��е�ƫ��
typedef pair < symbol_type, int > symbol; // a symbol in the grammer
//�����ַ������ڲ����ŵ�ӳ�䣬����������
typedef map < string, symbol, less<string> > symbols;
//�����ռ����ͷ��ռ���
typedef symbol nonterminal;
typedef symbol terminal;
//�������ʽ�Ľṹ��һ�����ռ�����Ϊ�󲿣� һ������������Ϊ�Ҳ�
struct production
{
	nonterminal lhs;
	vector<symbol> rhs;
};

//����TOKEN���ڲ�����
class TOKEN
{
public:
	TOKEN(symbol v, int l):value(v),line(l) {};
	~TOKEN() {};
	symbol value; //TOKEN���ڲ���ʾ
	int line; //��TOKEN��Ӧ���ı���������
};


//initalizing semantic analyser

//*****************************************************
// **********���������Ҫ�õ������ͼ���������************
 //*****************************************************


//��ʶ�������ͣ����ͣ�����������ʽ�ı�ʶ��
typedef  enum    {typeKind,varKind,procKind} IdKind;

//���������dir��ֱ�ӱ���(ֵ��)��indir��ʾ��ӱ���(���)						
typedef  enum    {dir,indir} AccessKind;

struct typeIR;
//��ʶ�������Խṹ����
typedef struct 
{
	struct typeIR  * idtype;		//ָ���ʶ���������ڲ���ʾ
	IdKind    kind;					//��ʶ��������
	union   
	{
		struct
		{
			AccessKind   access;   //�ж��Ǳ�λ���ֵ��
			int          level;    
			int          off;
			bool         isParam;  //�ж��ǲ���������ͨ����

		}VarAttr;//������ʶ��������	
		struct
		{
			int         level;     //�ù��̵Ĳ���
			               
		}ProcAttr;//��������ʶ��������
	
	}More;//��ʶ���Ĳ�ͬ�����в�ͬ������

}AttributeIR;




//*****************************************************
// *****************   �����ڲ���ʾ    ******************
 //*****************************************************

//���͵�ö�ٶ���
typedef  enum {intTy,charTy,arrayTy,recordTy,boolTy} TypeKind;


struct typeIR;
struct field;
//�����͵�Ԫ�ṹ����
typedef struct field
{
	char * id;              //������/
	int    off;                 //���ڼ�¼�е�ƫ��
	struct typeIR  *  UnitType; //���г�Ա������
	struct field * next;  //ָ����һ�����Ա
}Field;


//���͵��ڲ��ṹ����
typedef   struct  typeIR
{	
	int				size;   //������ռ�ռ��С
	TypeKind		kind;
	union
	{   struct
		{	
			struct typeIR  * elemTy; //��¼����Ԫ�ص�����
			int    low;     //��¼�������͵��½�
			int    top;      //��¼�������͵��Ͻ�
		}ArrayAttr;
		 Field  * body;  //��¼�����е���
	} More;
}TypeIR;

//�����¼�����ͣ����ͳ�������ʶ�������ͣ������飬����
enum semantic_record_kind { INTC, ID, TYPE, FIELD, ARRAY, ERROR };

//���������¼
typedef struct sem_rec
{
	enum semantic_record_kind record_kind;
	union
	{
		int intc; //INTC
		char *id_record; //ID
		TypeIR * type_record; //TYPE
		struct field * field_record; //FIELD
		
		int err_record; //ERROR
	} sem_info;
} semantic_record;






