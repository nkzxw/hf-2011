// Compiler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Grammar.h"
#include "Scanner.h"
#include "LL1_Analyser.h"
#include "LL1_Recognizer.h"
#include "Rec_Parse.h"
#include "Action.h"

void pause()
{
	cout<<endl<<"Press Enter to continue";
	getchar();
	system("cls");
}

int main(int argc, char* argv[])
{
	Grammar g;
	pause();
	Scanner s(&g);
	pause();
	LL1_Analyser l(g);
	pause();
	LL1_Recognizer r(&g,l.LL_Table,&(s.tokenlist));
	pause();
	Rec_Parse p(&g,l.predict,&(s.tokenlist));
	pause();
	r.print_table();
	return 0;
}

