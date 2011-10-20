//本文件定义从词法到语义所有的内部表示

#pragma once
using namespace std;

//定义符号的类型（注意，其中内容和name数组是一致的。）
typedef enum {Vn,Key,Border,Arithmetic,Relation,Intc,Charc,Id,$,Action,Default,Eof} symbol_type;
//定义符号的结构，即类型和在表中的偏移
typedef pair < symbol_type, int > symbol; // a symbol in the grammer
//定义字符串到内部符号的映射，即关联数组
typedef map < string, symbol, less<string> > symbols;
//定义终极符和非终极符
typedef symbol nonterminal;
typedef symbol terminal;
//定义产生式的结构，一个非终极符作为左部， 一个符号数组作为右部
struct production
{
	nonterminal lhs;
	vector<symbol> rhs;
};

//定义TOKEN的内部类型
class TOKEN
{
public:
	TOKEN(symbol v, int l):value(v),line(l) {};
	~TOKEN() {};
	symbol value; //TOKEN的内部表示
	int line; //该TOKEN对应的文本所在行数
};


//initalizing semantic analyser

//*****************************************************
// **********语义分析需要用到的类型及变量定义************
 //*****************************************************


//标识符的类型：类型，变量，产生式的标识符
typedef  enum    {typeKind,varKind,procKind} IdKind;

//变量的类别。dir表直接变量(值参)，indir表示间接变量(变参)						
typedef  enum    {dir,indir} AccessKind;

struct typeIR;
//标识符的属性结构定义
typedef struct 
{
	struct typeIR  * idtype;		//指向标识符的类型内部表示
	IdKind    kind;					//标识符的类型
	union   
	{
		struct
		{
			AccessKind   access;   //判断是变参还是值参
			int          level;    
			int          off;
			bool         isParam;  //判断是参数还是普通变量

		}VarAttr;//变量标识符的属性	
		struct
		{
			int         level;     //该过程的层数
			               
		}ProcAttr;//过程名标识符的属性
	
	}More;//标识符的不同类型有不同的属性

}AttributeIR;




//*****************************************************
// *****************   类型内部表示    ******************
 //*****************************************************

//类型的枚举定义
typedef  enum {intTy,charTy,arrayTy,recordTy,boolTy} TypeKind;


struct typeIR;
struct field;
//域类型单元结构定义
typedef struct field
{
	char * id;              //变量名/
	int    off;                 //所在记录中的偏移
	struct typeIR  *  UnitType; //域中成员的类型
	struct field * next;  //指向下一个域成员
}Field;


//类型的内部结构定义
typedef   struct  typeIR
{	
	int				size;   //类型所占空间大小
	TypeKind		kind;
	union
	{   struct
		{	
			struct typeIR  * elemTy; //记录数组元素的类型
			int    low;     //记录数组类型的下界
			int    top;      //记录数组类型的上界
		}ArrayAttr;
		 Field  * body;  //记录类型中的域
	} More;
}TypeIR;

//语义记录的类型：整型常量，标识符，类型，域，数组，错误
enum semantic_record_kind { INTC, ID, TYPE, FIELD, ARRAY, ERROR };

//定义语义记录
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






