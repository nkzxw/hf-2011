源码名称: 编译器源代码

开发环境: VC6 && STL

简介:

这个编译器的源代码是我原先为了完成编译原理实验课作业而写的,所以只具有教学价值,现在发出来和大家共享 ;-)

和网上流传的版本不同,它从文法开始,一直做到了符号表的实现. 想实现自己的编译器的话,只需在把Initializtion.h中的文法修改为自己的即可.

工程结构:
Initializtion.h 初始化文法，便于进一步进行分析，它为构造GRAMMAR类提供了信息.其中默认非终极符用<>括上,修改时需要注意.
Grammar.cpp Grammar.h 定义了文法GRAMMAR类，它通过initializtion.h的信息建立文法的内部表示。
LL1_Analyser.cpp LL1_Analyser.h 定义了LL1分析器,即LL1_Analyser类.
LL1_Recognizer.cpp LL1_Recognizer.h  为LL1语法分析驱动器,可以通过文法,TOKEN序列和LL1分析表,判定语法是否正确,同时驱动动作.
Rec_Parse.cpp Rec_Pares.h  实现了递归下降分析器Rec_Parse类, 递归下降的思想和LL1驱动器一样,不过是把压栈改成调用自己,而把弹栈改成返回.
Scanner.cpp Scanner.h 实现了词法分析器,可以将程序变为TOKEN序列. 扫描的源程序文件路径也在这里被定义(默认为.//demo.txt)
Action.cpp Action.h 实现了语义栈的操作,_Action类定义了动作符号所对应的动作.
SymTable.cpp SymTable.h 实现了符号表的建立和输出.

希望大家能通过该程序对STL和编译原理有更深刻的理解,Have Fun and Good Luck!

					-- David.Morre