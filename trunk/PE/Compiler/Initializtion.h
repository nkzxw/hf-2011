//本文件初始化了文法，便于进一步进行分析，它为构造GRAMMAR类提供的信息

#pragma once
using namespace std;

const int keynum=21;  //关键字个数
const int bordernum=10;  //符号个数
const int arithnum=4;  //算术运算符个数
const int relnum=6;    //关系运算符个数
const int namenum=8;   //各种类型名字个数
const int Vnum=72;     //非终极符个数
const int actionum=13;  //动作符号个数
int Intcnum=0, //整型常量个数
	idnum=0,   //标识符个数
	Charcnum=0;  //字符常量个数
string key[keynum]= //关键字表
{
	"program",	"procedure", "type", "var", "if",
	"then", "else", "fi", "while", "do", "endwh",
	"begin", "end",	"read", "write", "array",
	"of", "record", "return", "integer", "char"
}; 
string border[bordernum]={",",";",":=",".","(",")",":","[","]",".."}; //符号表
string arithmetic[arithnum]={"+","-","*","/"}; //算术运算符表 
string relation[relnum]={"<","<=","=",">",">=","<>"}; //关系运算符表
//非终级符集合
string vn[Vnum]=
{
	"<Program>", "<ProgramHead>", "<ProgramName>",	"<DeclarePart>",
	"<TypeDecpart>","<TypeDef>","<VarDecpart>",
	"<TypeDec>", "<TypeDeclaration>", "<TypeDecList>", "<TypeDecMore>",
	"<TypeId>", "<TypeName>", "<BaseType>", "<StructureType>",
	"<ArrayType>", "<Low>",	"<Top>", "<RecType>",
	"<FieldDecList>", "<FieldDecMore>",	"<IdList>",	"<IdMore>",
	"<VarDec>",	"<VarDeclaration>", "<VarDecList>",	"<VarDecMore>",
	"<VarIdList>", "<VarIdMore>", "<ProcDec>", "<ProcDeclaration>",
	"<ProcDecMore>", "<ProcName>", "<ParamList>", "<ParamDecList>",
	"<ParamMore>", "<Param>", "<FormList>", "<FidMore>",
	"<ProcDecPart>", "<ProcDecpart>", "<ProcBody>", "<ProgramBody>",	"<StmList>",
	"<StmMore>", "<Stm>", "<AssCall>", "<AssignmentRest>",
	"<ConditionalStm>", "<StmL>", "<LoopStm>", "<InputStm>",
	"<InVar>", "<OutputStm>", "<ReturnStm>", "<CallStmRest>",
	"<ActParamList>", "<ActParamMore>", "<RelExp>",	"<OtherRelE>",
	"<Exp>", "<OtherTerm>", "<Term>", "<OtherFactor>",
	"<Factor>", "<Variable>", "<VariMore>", "<FieldVar>",
	"<FieldVarMore>", "<CmpOp>", "<AddOp>", "<MultOp>"
};
//各类类型名字
string name[namenum]={"VN","KEY","BORDER","ARITHMETIC","RELATION","INTC","CHARC","ID"};
//动作符号表
string action[actionum]=
{
	"#ProgHead", "#Id", "#BaseType","#Intc","#ArrayType","#RecordType",
	"#TypeDec","#FieldDec","#Param", "#VarDec", "#ParamVar","#AddLevel",
	"#SubLevel"
};
const int num_prods=104; //产生式个数
const int max_prod_length=13; //最长产生式长度
//建立二维字符串数组存放产生式
string ProdStr[num_prods][max_prod_length]= 
{
	//总程序:
	{ "<Program>", "<ProgramHead>", "#AddLevel", "<DeclarePart>", "<ProgramBody>", ".", "\n" }, //1
	//程序头:
	{ "<ProgramHead>", "program", "<ProgramName>", "#ProgHead", "\n" }, //2
	{ "<ProgramName>", "ID", "#Id","\n" },  //3
	//程序声明:
	{ "<DeclarePart>", "<TypeDecpart>", "<VarDecpart>", "<ProcDecpart>", "\n" }, //4
	//类型声明:
	{ "<TypeDecpart>", "$", "\n" }, //5
	{ "<TypeDecpart>", "<TypeDec>", "\n" }, //6
	{ "<TypeDec>", "type", "<TypeDecList>", "\n" }, //7
	{ "<TypeDecList>", "<TypeId>", "=", "<TypeDef>", ";", "#TypeDec", "<TypeDecMore>", "\n" }, //8
	{ "<TypeDecMore>", "$", "\n" }, //9
	{ "<TypeDecMore>", "<TypeDecList>", "\n" }, //10
	{ "<TypeId>", "ID", "#Id", "\n" }, //11
	//类型:
	{ "<TypeDef>", "<BaseType>", "\n" }, //12
	{ "<TypeDef>", "<StructureType>", "\n" }, //13
	{ "<TypeDef>", "ID", "#Id", "\n" }, //14
	{ "<BaseType>", "integer", "#BaseType", "\n" }, //15
	{ "<BaseType>", "char", "#BaseType", "\n" }, //16
	{ "<StructureType>", "<ArrayType>", "\n" }, //17
	{ "<StructureType>", "<RecType>", "\n" }, //18
	{ "<ArrayType>", "array", "[", "<Low>", "..", "<Top>", "]", "of", "<BaseType>", "#ArrayType", "\n" }, //19
	{ "<Low>", "INTC", "#Intc", "\n" }, //20
	{ "<Top>", "INTC", "#Intc", "\n" }, //21
	{ "<RecType>", "record", "<FieldDecList>", "end", "#RecordType", "\n" }, //22
	{ "<FieldDecList>", "<BaseType>", "<IdList>", ";", "#FieldDec", "<FieldDecMore>", "\n" }, //23
	{ "<FieldDecList>", "<ArrayType>", "<IdList>", ";", "#FieldDec","<FieldDecMore>", "\n" }, //24	
	{ "<FieldDecMore>", "$", "\n" }, //25
	{ "<FieldDecMore>", "<FieldDecList>", "\n" }, //26
	{ "<IdList>", "ID", "#Id", "<IdMore>", "\n" }, //27
	{ "<IdMore>", "$", "\n"}, //28
	{ "<IdMore>", ",", "<IdList>", "\n" }, //29
	//变量声明:
	{ "<VarDecpart>", "$", "\n" }, //30
	{ "<VarDecpart>", "<VarDec>", "\n" }, //31
	{ "<VarDec>", "var", "<VarDecList>",  "\n" },  //32 
	{ "<VarDecList>", "<TypeDef>", "<VarIdList>", ";", "#VarDec", "<VarDecMore>", "\n" }, //33
	{ "<VarDecMore>", "$", "\n" }, //34
	{ "<VarDecMore>", "<VarDecList>", "\n" }, //35
	{ "<VarIdList>", "ID", "#Id", "<VarIdMore>", "\n" }, //36
	{ "<VarIdMore>", "$", "\n" }, //37
	{ "<VarIdMore>", ",", "<VarIdList>", "\n" }, //38
	//过程声明:
	{ "<ProcDecpart>", "$", "\n" }, //39
	{ "<ProcDecpart>", "<ProcDec>", "\n" }, //40
	{ "<ProcDec>", "procedure", "<ProcName>",  "(", "#AddLevel","<ParamList>", ")", ";", 
		       "<ProcDecPart>", "<ProcBody>", "<ProcDecMore>", "\n" }, //41
	{ "<ProcDecMore>", "$", "\n" }, //42
	{ "<ProcDecMore>", "<ProcDec>", "\n" }, //43
	{ "<ProcName>", "ID", "#Id", "\n" }, //44
	//参数声明:
	{ "<ParamList>", "$", "\n" }, //45
	{ "<ParamList>", "<ParamDecList>", "\n" }, //46
	{ "<ParamDecList>", "<Param>", "<ParamMore>", "\n" }, //47
	{ "<ParamMore>", "$", "\n" }, //48
	{ "<ParamMore>", ";", "<ParamDecList>", "\n" }, //49
	{ "<Param>", "<TypeDef>", "<FormList>", "#Param", "\n" }, //50
	{ "<Param>", "var", "<TypeDef>", "<FormList>", "#ParamVar","\n" }, //51
	{ "<FormList>", "ID", "#Id", "<FidMore>", "\n" }, //52
	{ "<FidMore>", "$", "\n" }, //53
	{ "<FidMore>", ",", "<FormList>", "\n" }, //54
	//过程中的声明部分:
	{ "<ProcDecPart>", "<DeclarePart>", "\n" }, //55
	//过程体:
	{ "<ProcBody>", "<ProgramBody>", "\n" }, //56
	//主程序体:
	{ "<ProgramBody>", "begin", "<StmList>", "end", "#SubLevel","\n" }, //57
	//语句序列:
	{ "<StmList>", "<Stm>", "<StmMore>", "\n" }, //58
	{ "<StmMore>", "$", "\n" }, //59
	{ "<StmMore>", ";", "<StmList>", "\n" }, //60
	//语句:
	{ "<Stm>", "<ConditionalStm>", "\n" }, //61
	{ "<Stm>", "<LoopStm>", "\n" }, //62
	{ "<Stm>", "<InputStm>", "\n" }, //63
	{ "<Stm>", "<OutputStm>", "\n" }, //64
	{ "<Stm>", "<ReturnStm>", "\n" }, //65
	{ "<Stm>", "ID", "#Id", "<AssCall>", "\n" }, //66
	//因为赋值语句和函数调用语句的开始部分都是标识符,所以将赋值语句和调用语句写在一起.
	{ "<AssCall>", "<AssignmentRest>", "\n" }, //67
	{ "<AssCall>", "<CallStmRest>", "\n" }, //68
	//赋值语句:
	{ "<AssignmentRest>", "<VariMore>", ":=", "<Exp>", "\n" }, //69
	//条件语句:	
	{ "<ConditionalStm>", "if", "<RelExp>", "then", "<StmList>",
						  "else", "<StmList>", "fi", "\n" }, //70
	//循环语句:
	{ "<LoopStm>", "while", "<RelExp>", "do", "<StmList>", "endwh", "\n" }, //71
	//输入语句:
	{ "<InputStm>", "read", "(", "<InVar>", ")", "\n" }, //72
	{ "<InVar>", "ID", "#Id", "\n" }, //73
	//输出语句:
	{ "<OutputStm>", "write", "(", "<Exp>", ")", "\n" }, //74
	//返回语句:
	{ "<ReturnStm>", "return", "\n" }, //75
	//过程调用语句:
	{ "<CallStmRest>", "(", "<ActParamList>", ")", "\n" }, //76
	{ "<ActParamList>", "$", "\n" }, //77
	{ "<ActParamList>", "<Exp>", "<ActParamMore>", "\n" }, //78
	{ "<ActParamMore>", "$", "\n" }, //79
 	{ "<ActParamMore>", ",", "<ActParamList>", "\n" }, //80
	//条件表达式:
	{ "<RelExp>", "<Exp>", "<OtherRelE>", "\n" }, //81
	{ "<OtherRelE>", "<CmpOp>", "<Exp>", "\n" }, //82
	//算术表达式:
	{ "<Exp>", "<Term>", "<OtherTerm>", "\n" }, //83
	{ "<OtherTerm>", "$", "\n" }, //84
	{ "<OtherTerm>", "<AddOp>", "<Exp>", "\n" }, //85
	//项:	
	{ "<Term>", "<Factor>", "<OtherFactor>", "\n" }, //86
	{ "<OtherFactor>", "$", "\n" }, //87
	{ "<OtherFactor>", "<MultOp>", "<Term>", "\n" }, //88
	//因子:
	{ "<Factor>", "(", "<Exp>", ")", "\n" }, //89
	{ "<Factor>", "INTC", "#Intc", "\n" }, //90
	{ "<Factor>", "<Variable>", "\n" }, //91
	{ "<Variable>", "ID", "#Id", "<VariMore>", "\n" }, //92
	{ "<VariMore>", "$", "\n" }, //93
	{ "<VariMore>", "[", "<Exp>", "]", "\n" }, //94
	{ "<VariMore>", ".", "<FieldVar>", "\n" }, //95
	{ "<FieldVar>", "ID", "#Id", "<FieldVarMore>", "\n" }, //96
	{ "<FieldVarMore>", "$", "\n" }, //97
	{ "<FieldVarMore>", "[", "<Exp>", "]", "\n" }, //98
	{ "<CmpOp>", "<", "\n" }, //99
	{ "<CmpOp>", "=", "\n" }, //100
	{ "<AddOp>", "+", "\n" }, //101
	{ "<AddOp>", "-", "\n" }, //102
	{ "<MultOp>", "*", "\n" }, //103
	{ "<MultOp>", "/", "\n" }, //104
};
