
#ifndef XXXXXXXXXXXXXXXXX
#define XXXXXXXXXXXXXXXXX

//返回load后的基地址
ULONG	myloader(char*  Name, wchar_t* Path, PDRIVER_OBJECT	fakeDriverObject, BOOLEAN Boot ,BOOLEAN bExcute);

wchar_t* 
GetModuleName(wchar_t* ModPath);


#endif
