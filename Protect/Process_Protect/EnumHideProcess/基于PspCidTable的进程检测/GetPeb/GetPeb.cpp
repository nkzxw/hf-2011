// GetPeb.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>

__inline __declspec(naked) unsigned int GetPEB()
{
__asm
{ 
    xor esi, esi
    mov esi, fs:[esi + 30H] 
    mov eax, esi 
    ret
}
}

int main(int argc, char* argv[])
{
	printf("located at: 0x%0.8X\n",GetPEB());
    getchar();
	return 0;
}

