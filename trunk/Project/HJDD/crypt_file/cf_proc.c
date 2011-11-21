///
/// @file         cf_proc.c
/// @author    crazy_chu
/// @date       2009-1-30
/// @brief      ��õ�ǰ���̵����֡� 
/// 
/// ��������
/// ������Ϊʾ�����롣δ���꾡���ԣ�����֤�ɿ��ԡ����߶�
/// �κ���ʹ�ô˴��뵼�µ�ֱ�Ӻͼ����ʧ�������Ρ�
/// 
/// ��ȨЭ��
/// ����������ڹ���crypt_file.�ǳ�������wowocockΪ��������
/// ������Windows�ں˱������Ϣ��ȫ������д���ļ�͸������
/// ʾ���������̽���֧��WindowsXP�£�FastFat�ļ�ϵͳ�¼���
/// ���ļ��ܡ�δ������ɱ��������������ļ��������������
/// �����������ȫ��Ȩ��Ϊ���߱�������������ѧϰ���Ķ�ʹ
/// �á�δ����λ����������Ȩ������ֱ�Ӹ��ơ����߻��ڴ˴�
/// ������޸ġ����ô˴����ṩ��ȫ�����߲��ּ���������ҵ
/// ��������������������Ļ�����Ϊ������Υ�������߱�����
/// �ߺͻ�ȡ�⳥֮Ȩ�����Ķ��˴��룬���Զ���Ϊ����������
/// ȨЭ�顣�粻���ܴ�Э���ߣ��벻Ҫ�Ķ��˴��롣
///

#include <ntifs.h>

// �������������DriverEntry�е��ã�����cfCurProcName���������á�
static size_t s_cf_proc_name_offset = 0;
void cfCurProcNameInit()
{
	ULONG i;
	PEPROCESS  curproc;
	curproc = PsGetCurrentProcess();
	// ����EPROCESS�ṹ���������ҵ��ַ���
	for(i=0;i<3*4*1024;i++)
	{
		if(!strncmp("System",(PCHAR)curproc+i,strlen("System"))) 
		{
			s_cf_proc_name_offset = i;
			break;
		}
	}
}

// ���º������Ի�ý����������ػ�õĳ��ȡ�
ULONG cfCurProcName(PUNICODE_STRING name)
{
	PEPROCESS  curproc;
	ULONG	i,need_len;
    ANSI_STRING ansi_name;
	if(s_cf_proc_name_offset == 0)
		return 0;

    // ��õ�ǰ����PEB,Ȼ���ƶ�һ��ƫ�Ƶõ�����������λ�á�
	curproc = PsGetCurrentProcess();

    // ���������ansi�ַ���������ת��Ϊunicode�ַ�����
    RtlInitAnsiString(&ansi_name,((PCHAR)curproc + s_cf_proc_name_offset));
    need_len = RtlAnsiStringToUnicodeSize(&ansi_name);
    if(need_len > name->MaximumLength)
    {
        return RtlAnsiStringToUnicodeSize(&ansi_name);
    }
    RtlAnsiStringToUnicodeString(name,&ansi_name,FALSE);
	return need_len;
}

// �жϵ�ǰ�����ǲ���notepad.exe
BOOLEAN cfIsCurProcSec(void)
{
    WCHAR name_buf[32] = { 0 };
    UNICODE_STRING proc_name = { 0 };
    UNICODE_STRING note_pad = { 0 };
    ULONG length;
    RtlInitEmptyUnicodeString(&proc_name,name_buf,32*sizeof(WCHAR));
    length = cfCurProcName(&proc_name);
    RtlInitUnicodeString(&note_pad,L"notepad.exe");
    if(RtlCompareUnicodeString(&note_pad,&proc_name,TRUE) == 0)
        return TRUE;
    return FALSE;
}

