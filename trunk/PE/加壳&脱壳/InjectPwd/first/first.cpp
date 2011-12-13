// *** Id: first.cpp - not functional

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>

#pragma warning(disable:4996)

void func_A( char *_str );
void func_B(void);
void func_C( char *_str );

long  g_var1 = 43981;
char *g_var2 = "this is a sample string!\n\n";

int main( int _argc, char *_argv[] )
{
   char  l_var0[64];
   int   l_var1 = 0;
   char *l_var2 = "another sample string (%ld)!\n";

   (void) memset( l_var0, 0x00, sizeof(l_var0) );

   (void) strcpy( l_var0, "test string\n" );
   func_A( l_var0 );

   sprintf( l_var0, l_var2, g_var1 );
   func_A( l_var0 );

   (void) strcpy( l_var0, g_var2 );
   func_A( l_var0 );

   func_B();

// func_C(l_var0);

	return 0;
}

void func_A( char *_str )
{
   char *ori = _str;

   while ( *_str )
      *_str++ = toupper(*_str);

   printf( ori );
}

//#pragma optimize( "", off ) 

#pragma code_seg(".seg0")  

void func_B(void)
{
   ::MessageBox( NULL, "MessageBox Called!", "First", MB_ICONINFORMATION );
}

void func_C(char *_str)
{
   char *ori = _str;

   while ( *_str )
      *_str++ = tolower(*_str);

   printf( ori );
}

#pragma code_seg()

//#pragma optimize( "", on ) 
