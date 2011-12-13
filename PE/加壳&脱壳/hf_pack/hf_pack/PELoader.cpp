#include "stdafx.h"
#include <windows.h>

bool PELoader(char *lpStaticPEBuff, long lStaticPELen)
{
	long lPESignOffset = *(long *)(lpStaticPEBuff + 0x3c);
	IMAGE_NT_HEADERS *pINH = (IMAGE_NT_HEADERS *)(lpStaticPEBuff + lPESignOffset);

	//取加载到内存中大小
	long lImageSize = pINH->OptionalHeader.SizeOfImage;
	char *lpDynPEBuff = new char[lImageSize];
	if(lpDynPEBuff == NULL){
		return false;
	}

	//取PE文件的节数量
	long lSectionNum = pINH->FileHeader.NumberOfSections;

	//计算PE头信息及节表信息占用内存大小
	long lPEHeadSize = lPESignOffset + sizeof(IMAGE_NT_HEADERS) + lSectionNum * sizeof(IMAGE_SECTION_HEADER);

	//加载PE头部信息及其各个节表
	memset(lpDynPEBuff, 0, lImageSize);
	memcpy(lpDynPEBuff, lpStaticPEBuff, lPEHeadSize);

	//加载各个节
	long lFileAlignMask = pINH->OptionalHeader.FileAlignment - 1;        //各节在磁盘中的对齐掩码
	long lSectionAlignMask = pINH->OptionalHeader.SectionAlignment - 1;  //各节在load后内存中的对齐掩码
	IMAGE_SECTION_HEADER *pISH = (IMAGE_SECTION_HEADER *)((char *)pINH + sizeof(IMAGE_NT_HEADERS));
	for(int nIndex = 0; nIndex < lSectionNum; nIndex++, pISH++)
	{
		//判定各节的对齐属性，合法不
		if((pISH->VirtualAddress & lSectionAlignMask) || 
			(pISH->SizeOfRawData & lFileAlignMask)){
			//出现非法节
			delete lpDynPEBuff;
			return false;
		}

		//加载改节
		memcpy(lpDynPEBuff + pISH->VirtualAddress, lpStaticPEBuff + pISH->PointerToRawData, pISH->SizeOfRawData);
	}

	//修改导入表，导入程序执行过程中要用到的API函数地址
	if(pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size > 0) //大于0说明有导入表
	{
		IMAGE_IMPORT_DESCRIPTOR *pIID = (IMAGE_IMPORT_DESCRIPTOR *)(lpDynPEBuff + 
										pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		//循环扫描每个将有函数导入的dll
		for(; pIID->Name != NULL; pIID++)
		{
			/*曾看过OllyDump源代码，那里在重建导入表的时候，并没有初始化OriginalFirstThunk这个字段,
			所以这里也不对OriginalFirstThunk这个字段进行处理了*/
			IMAGE_THUNK_DATA *pITD = (IMAGE_THUNK_DATA *)(lpDynPEBuff + pIID->FirstThunk);

			HINSTANCE hInstance = LoadLibrary(lpDynPEBuff + pIID->Name);
			if(hInstance == NULL){
				//导入这个dll失败
				delete lpDynPEBuff;
				return false;
			}

			//循环扫描dll内每个被导入函数
			for(; pITD->u1.Ordinal != 0; pITD++)
			{
				FARPROC fpFun;
				if(pITD->u1.Ordinal & IMAGE_ORDINAL_FLAG32){
					//函数是以序号的方式导入的
					fpFun = GetProcAddress(hInstance, (LPCSTR)(pITD->u1.Ordinal & 0x0000ffff));
				}
				else{
					//函数是以名称方式导入的
					IMAGE_IMPORT_BY_NAME * pIIBN = (IMAGE_IMPORT_BY_NAME *)(lpDynPEBuff + pITD->u1.Ordinal);
					fpFun = GetProcAddress(hInstance, (LPCSTR)pIIBN->Name);
				}

				if(fpFun == NULL)
				{
					//导出这个函数失败
					delete lpDynPEBuff;
					return false;
				}

				pITD->u1.Ordinal = (long)fpFun;
			}

			 FreeLibrary(hInstance);
		}
	}


	//重定位处理
	if(pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0)
	{
		//取第一个重定位块
		IMAGE_BASE_RELOCATION *pIBR = (IMAGE_BASE_RELOCATION *)(lpDynPEBuff + pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

		long lDifference = (long)lpDynPEBuff - pINH->OptionalHeader.ImageBase;

		//循环每个重定位块
		for(; pIBR->VirtualAddress != 0; )
		{
			char *lpMemPage = lpDynPEBuff + pIBR->VirtualAddress;
			long lCount = (pIBR->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) >> 1;

			//对这个页面中的每个需重定位的项进行处理
			short int *pRelocationItem = (short int *)((char *)pIBR + sizeof(IMAGE_BASE_RELOCATION));
			for(int nIndex = 0; nIndex < lCount; nIndex++)
			{
				int nOffset = pRelocationItem[nIndex] &0x0fff;
				int nType = pRelocationItem[nIndex] >> 12;

				//虽然windows定义了很多重定位类型，但是在PE文件中只能见到0和3两种
				if(nType == 3){
					*(long *)(lpDynPEBuff + nOffset) += lDifference;
				}
				else if(nType == 0){
					//TODO
				}
			}

			//pIBR指向下一个重定位块
			pIBR = (IMAGE_BASE_RELOCATION *)(pRelocationItem + lCount);
		}

	}

	delete lpDynPEBuff;

	return true;
}