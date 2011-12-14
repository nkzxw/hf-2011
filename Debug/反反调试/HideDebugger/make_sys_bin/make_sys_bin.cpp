
#include <windows.h>
#include <stdio.h>


#define XOR_VALUE 0xAC

void main()
{
	system("del /q _SYS_BIN.h");	
	FILE* fp=fopen("drv.sys", "rb");
	if(fp == NULL)
	{
		printf("open drv error.\n");
		return;
	}

	fseek(fp, 0, SEEK_END);
	int len=ftell(fp);

	rewind(fp);

	BYTE* p = new BYTE[len+1];
	fread(p, len, 1, fp);
	fclose(fp);

	fp = fopen("_SYS_BIN.h", "wt");
	if(fp == NULL)
	{
		printf("create _SYS_BIN.h error.\n");
		return;
	}

	fprintf(fp, "#define SYSDATALEN 0x%X\n\n", len);



	fprintf(fp, "unsigned char gcbSYSData[] = \n");
	for(int i=0; i<len; i++)
	{
		if((i%16) == 0)
		{
			if(i!=0)
				fprintf(fp, "\"");
			fprintf(fp, "\n");
			fprintf(fp, "\"");
		}

		fprintf(fp, "\\x%02X", p[i]);
	}

	fprintf(fp, "\";\n\n");


	fclose(fp);
}