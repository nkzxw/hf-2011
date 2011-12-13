#include "stdafx.h"
#include <windows.h>

bool PELoader(char *lpStaticPEBuff, long lStaticPELen)
{
	long lPESignOffset = *(long *)(lpStaticPEBuff + 0x3c);
	IMAGE_NT_HEADERS *pINH = (IMAGE_NT_HEADERS *)(lpStaticPEBuff + lPESignOffset);

	//ȡ���ص��ڴ��д�С
	long lImageSize = pINH->OptionalHeader.SizeOfImage;
	char *lpDynPEBuff = new char[lImageSize];
	if(lpDynPEBuff == NULL){
		return false;
	}

	//ȡPE�ļ��Ľ�����
	long lSectionNum = pINH->FileHeader.NumberOfSections;

	//����PEͷ��Ϣ���ڱ���Ϣռ���ڴ��С
	long lPEHeadSize = lPESignOffset + sizeof(IMAGE_NT_HEADERS) + lSectionNum * sizeof(IMAGE_SECTION_HEADER);

	//����PEͷ����Ϣ��������ڱ�
	memset(lpDynPEBuff, 0, lImageSize);
	memcpy(lpDynPEBuff, lpStaticPEBuff, lPEHeadSize);

	//���ظ�����
	long lFileAlignMask = pINH->OptionalHeader.FileAlignment - 1;        //�����ڴ����еĶ�������
	long lSectionAlignMask = pINH->OptionalHeader.SectionAlignment - 1;  //������load���ڴ��еĶ�������
	IMAGE_SECTION_HEADER *pISH = (IMAGE_SECTION_HEADER *)((char *)pINH + sizeof(IMAGE_NT_HEADERS));
	for(int nIndex = 0; nIndex < lSectionNum; nIndex++, pISH++)
	{
		//�ж����ڵĶ������ԣ��Ϸ���
		if((pISH->VirtualAddress & lSectionAlignMask) || 
			(pISH->SizeOfRawData & lFileAlignMask)){
			//���ַǷ���
			delete lpDynPEBuff;
			return false;
		}

		//���ظĽ�
		memcpy(lpDynPEBuff + pISH->VirtualAddress, lpStaticPEBuff + pISH->PointerToRawData, pISH->SizeOfRawData);
	}

	//�޸ĵ�����������ִ�й�����Ҫ�õ���API������ַ
	if(pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size > 0) //����0˵���е����
	{
		IMAGE_IMPORT_DESCRIPTOR *pIID = (IMAGE_IMPORT_DESCRIPTOR *)(lpDynPEBuff + 
										pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		//ѭ��ɨ��ÿ�����к��������dll
		for(; pIID->Name != NULL; pIID++)
		{
			/*������OllyDumpԴ���룬�������ؽ�������ʱ�򣬲�û�г�ʼ��OriginalFirstThunk����ֶ�,
			��������Ҳ����OriginalFirstThunk����ֶν��д�����*/
			IMAGE_THUNK_DATA *pITD = (IMAGE_THUNK_DATA *)(lpDynPEBuff + pIID->FirstThunk);

			HINSTANCE hInstance = LoadLibrary(lpDynPEBuff + pIID->Name);
			if(hInstance == NULL){
				//�������dllʧ��
				delete lpDynPEBuff;
				return false;
			}

			//ѭ��ɨ��dll��ÿ�������뺯��
			for(; pITD->u1.Ordinal != 0; pITD++)
			{
				FARPROC fpFun;
				if(pITD->u1.Ordinal & IMAGE_ORDINAL_FLAG32){
					//����������ŵķ�ʽ�����
					fpFun = GetProcAddress(hInstance, (LPCSTR)(pITD->u1.Ordinal & 0x0000ffff));
				}
				else{
					//�����������Ʒ�ʽ�����
					IMAGE_IMPORT_BY_NAME * pIIBN = (IMAGE_IMPORT_BY_NAME *)(lpDynPEBuff + pITD->u1.Ordinal);
					fpFun = GetProcAddress(hInstance, (LPCSTR)pIIBN->Name);
				}

				if(fpFun == NULL)
				{
					//�����������ʧ��
					delete lpDynPEBuff;
					return false;
				}

				pITD->u1.Ordinal = (long)fpFun;
			}

			 FreeLibrary(hInstance);
		}
	}


	//�ض�λ����
	if(pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0)
	{
		//ȡ��һ���ض�λ��
		IMAGE_BASE_RELOCATION *pIBR = (IMAGE_BASE_RELOCATION *)(lpDynPEBuff + pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

		long lDifference = (long)lpDynPEBuff - pINH->OptionalHeader.ImageBase;

		//ѭ��ÿ���ض�λ��
		for(; pIBR->VirtualAddress != 0; )
		{
			char *lpMemPage = lpDynPEBuff + pIBR->VirtualAddress;
			long lCount = (pIBR->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) >> 1;

			//�����ҳ���е�ÿ�����ض�λ������д���
			short int *pRelocationItem = (short int *)((char *)pIBR + sizeof(IMAGE_BASE_RELOCATION));
			for(int nIndex = 0; nIndex < lCount; nIndex++)
			{
				int nOffset = pRelocationItem[nIndex] &0x0fff;
				int nType = pRelocationItem[nIndex] >> 12;

				//��Ȼwindows�����˺ܶ��ض�λ���ͣ�������PE�ļ���ֻ�ܼ���0��3����
				if(nType == 3){
					*(long *)(lpDynPEBuff + nOffset) += lDifference;
				}
				else if(nType == 0){
					//TODO
				}
			}

			//pIBRָ����һ���ض�λ��
			pIBR = (IMAGE_BASE_RELOCATION *)(pRelocationItem + lCount);
		}

	}

	delete lpDynPEBuff;

	return true;
}