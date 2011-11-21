///
/// @file         ps2intcap.c
/// @author    wowocock,crazy_chu
/// @date       2009-1-27
///

#include <ntddk.h>

// ��һ����ڣ��򱾳������Ϊ�滻INT0x93�����������
// �����ڣ���ΪIOAPIC�ض�λ������
// #define BUILD_FOR_IDT_HOOK

// �����������Ǳ�����ȷһ�����Ƕ���λ����������Ԥ�ȶ��弸����
// ȷ֪������λ���ȵı������Ա��ⲻͬ�����±�����鷳.
typedef unsigned char P2C_U8;
typedef unsigned short P2C_U16;
typedef unsigned long P2C_U32;

#define P2C_MAKELONG(low, high) \
((P2C_U32)(((P2C_U16)((P2C_U32)(low) & 0xffff)) | ((P2C_U32)((P2C_U16)((P2C_U32)(high) & 0xffff))) << 16))

#define P2C_LOW16_OF_32(data) \
((P2C_U16)(((P2C_U32)data) & 0xffff))

#define P2C_HIGH16_OF_32(data) \
((P2C_U16)(((P2C_U32)data) >> 16))

// ��sidtָ����һ�����µĽṹ����������Եõ�IDT�Ŀ�ʼ��ַ
#pragma pack(push,1)
typedef struct P2C_IDTR_ {
	P2C_U16 limit;		// ��Χ
	P2C_U32 base;		// ����ַ�����ǿ�ʼ��ַ��
} P2C_IDTR, *PP2C_IDTR;
#pragma pack(pop)

// �������������sidtָ�����һ��P2C_IDTR�ṹ��������IDT�ĵ�ַ��
void *p2cGetIdt()
{
	P2C_IDTR idtr;
    // һ�����ȡ��IDT��λ�á�
	_asm sidt idtr
	return (void *)idtr.base;
}

#pragma pack(push,1)
typedef struct P2C_IDT_ENTRY_ {
		P2C_U16 offset_low;
		P2C_U16 selector;
		P2C_U8 reserved;
		P2C_U8 type:4;
		P2C_U8 always0:1;
		P2C_U8 dpl:2;
		P2C_U8 present:1;
		P2C_U16 offset_high;
} P2C_IDTENTRY, *PP2C_IDTENTRY;
#pragma pack(pop)

#define OBUFFER_FULL 0x02
#define IBUFFER_FULL 0x01

ULONG p2cWaitForKbRead()
{
	int i = 100;
	P2C_U8 mychar;	
	do
	{
        _asm in al,0x64
        _asm mov mychar,al
	    KeStallExecutionProcessor(50);
	    if(!(mychar & OBUFFER_FULL)) break;
	} while (i--);
	if(i) return TRUE;
	return FALSE;
}

ULONG p2cWaitForKbWrite()
{
	int i = 100;
	P2C_U8 mychar;
	do
	{
        _asm in al,0x64
        _asm mov mychar,al
		KeStallExecutionProcessor(50);
		if(!(mychar & IBUFFER_FULL)) break;
	} while (i--);
	if(i) return TRUE;
	return FALSE;
}

// ���ȶ��˿ڻ�ð���ɨ�����ӡ������Ȼ�����ɨ
// ����д�ض˿ڣ��Ա���Ӧ�ó�������ȷ���յ�������
// ��������ñ�ĳ���ػ񰴼�������д��һ�������
// ���ݡ�
void p2cUserFilter()
{
    static P2C_U8 sch_pre = 0;
	P2C_U8	sch;
	p2cWaitForKbRead();
    _asm in al,0x60
    _asm mov sch,al
    KdPrint(("p2c: scan code = %2x\r\n",sch));
   //  ������д�ض˿ڣ��Ա��ñ�ĳ��������ȷ��ȡ��
	if(sch_pre != sch)
	{
		sch_pre = sch;
        _asm mov al,0xd2
        _asm out 0x64,al
		p2cWaitForKbWrite();
        _asm mov al,sch
        _asm out 0x60,al
	}
}

void *g_p2c_old = NULL;

__declspec(naked) p2cInterruptProc()
{
	__asm
	{
		pushad					// �������е�ͨ�üĴ���
		pushfd					// �����־�Ĵ���
		call p2cUserFilter	// ��һ�������Լ��ĺ����� ���������ʵ��
								    // һЩ�����Լ��Ĺ���
		popfd					// �ָ���־�Ĵ���
		popad					// �ָ�ͨ�üĴ���
		jmp	g_p2c_old		// ����ԭ�����жϷ������
	}
}

// ��������޸�IDT���еĵ�0x93��޸�Ϊp2cInterruptProc��
// ���޸�֮ǰҪ���浽g_p2c_old�С�
void p2cHookInt93(BOOLEAN hook_or_unhook)
{
    PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)p2cGetIdt();
    idt_addr += 0x93;
    KdPrint(("p2c: the current address = %x.\r\n",
        (void *)P2C_MAKELONG(idt_addr->offset_low,idt_addr->offset_high)));
    if(hook_or_unhook)
    {
        KdPrint(("p2c: try to hook interrupt.\r\n"));
        // ���g_p2c_old��NULL����ô����hook
        g_p2c_old = (void *)P2C_MAKELONG(idt_addr->offset_low,idt_addr->offset_high);
        idt_addr->offset_low = P2C_LOW16_OF_32(p2cInterruptProc);
        idt_addr->offset_high = P2C_HIGH16_OF_32(p2cInterruptProc);
    }
    else
    {
        KdPrint(("p2c: try to recovery interrupt.\r\n"));
        // ���g_p2c_old����NULL����ôȡ��hook.
        idt_addr->offset_low = P2C_LOW16_OF_32(g_p2c_old);
        idt_addr->offset_high = P2C_HIGH16_OF_32(g_p2c_old);
    }
    KdPrint(("p2c: the current address = %x.\r\n",
        (void *)P2C_MAKELONG(idt_addr->offset_low,idt_addr->offset_high)));
}

// ��idt�����ҵ�һ�����е�idtentry��λ�á�Ȼ�󷵻����id.����Ϊ
// �������������µļ����жϴ�����ڡ�����Ҳ����ͷ���0����
// ��������޷���װ�µ��жϴ���
P2C_U8 p2cGetIdleIdtVec()
{
    P2C_U8 i;
    PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)p2cGetIdt();

    // ��vec20������2a���ɡ�
	for(i=0x20;i<0x2a;i++)
	{
        // �������Ϊ0˵���ǿ���λ�ã����ؼ��ɡ�
        if(idt_addr[i].type == 0)
		{
			return i;
		}
	}
    return 0;
}


P2C_U8 p2cCopyANewIdt93(P2C_U8 id,void *interrupt_proc)
{
    // ����д��һ���µ��ж��š��������ȫ����ԭ����0x93
    // �ϵ�idtentry��ֻ���жϴ������ĵ�ַ��ͬ��
    PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)p2cGetIdt();
    idt_addr[id] = idt_addr[0x93];
    idt_addr[id].offset_low = P2C_LOW16_OF_32(interrupt_proc);
    idt_addr[id].offset_high = P2C_HIGH16_OF_32(interrupt_proc);
    return id;
}

// ����IOAPIC��ü����жϣ������������ֵ��
P2C_U8 p2cSeachOrSetIrq1(P2C_U8 new_ch)
{
    // ѡ��Ĵ�����ѡ��Ĵ�����Ȼ��32λ�ļĴ���������ֻʹ��
    // ��8λ��������λ����������
	P2C_U8 *io_reg_sel;

    // ���ڼĴ�����������д��ѡ��Ĵ���ѡ���ֵ����32λ�ġ�
	P2C_U32 *io_win;
	P2C_U32 ch,ch1;

    // ����һ�������ַ�������ַΪ0xfec00000������IOAPIC
    // �Ĵ�������Windows�ϵĿ�ʼ��ַ
	PHYSICAL_ADDRESS	phys ;
	PVOID paddr;
	RtlZeroMemory(&phys,sizeof(PHYSICAL_ADDRESS));
	phys.u.LowPart = 0xfec00000;

    // �����ַ�ǲ���ֱ�Ӷ�д�ġ�MmMapIoSpace�������ַӳ��
    // Ϊϵͳ�ռ�������ַ��0x14����Ƭ�ռ�ĳ��ȡ�
	paddr = MmMapIoSpace(phys, 0x14, MmNonCached);

    // ���ӳ��ʧ���˾ͷ���0.
	if (!MmIsAddressValid(paddr))
		return 0;

    // ѡ��Ĵ�����ƫ��Ϊ0
	io_reg_sel = (P2C_U8 *)paddr;
    // ���ڼĴ�����ƫ��Ϊ0x10.
	io_win = (P2C_U32 *)((P2C_U8 *)(paddr) + 0x10);

    // ѡ���0x12���պ���irq1����
	*io_reg_sel = 0x12;
	ch = *io_win;

    // ���new_ch��Ϊ0�����Ǿ�������ֵ�������ؾ�ֵ��
    if(new_ch != 0)
    {
        ch1 = *io_win;
        ch1 &= 0xffffff00;
        ch1 |= (P2C_U32)new_ch;
        *io_win = ch1;
        KdPrint(("p2cSeachOrSetIrq1: set %2x to irq1.\r\n",(P2C_U8)new_ch));
    }

    // ���ڼĴ����������ֵ��32λ�ģ���������ֻ��Ҫ
    // һ���ֽھͿ����ˡ�����ֽھ����ж�������ֵ��
    // һ������Ҫ�޸����ֵ��
    ch &= 0xff;
	MmUnmapIoSpace(paddr, 0x14);
    KdPrint(("p2cSeachOrSetIrq1: the old vec of irq1 is %2x.\r\n",(P2C_U8)ch));
	return (P2C_U8)ch;
}

void p2cResetIoApic(BOOLEAN set_or_recovery)
{
    static P2C_U8 idle_id = 0;
    PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)p2cGetIdt();
    P2C_U8 old_id = 0;

    if(set_or_recovery)
    {
        // ����������µ�ioapic��λ����ô������g_p2c_old�б���
        // ԭ��������ڡ�
        idt_addr = (PP2C_IDTENTRY)p2cGetIdt();
        idt_addr += 0x93;
        g_p2c_old = (void *)P2C_MAKELONG(idt_addr->offset_low,idt_addr->offset_high);
 
        // Ȼ����һ������λ����irq1�����ж��Ÿ���һ����ȥ��
        // �������ת������дΪ���ǵ��µĴ�������
        idle_id = p2cGetIdleIdtVec();
        if(idle_id != 0)
        {
            p2cCopyANewIdt93(idle_id,p2cInterruptProc);
            // Ȼ�����¶�λ������жϡ�
            old_id = p2cSeachOrSetIrq1(idle_id);
            // ��32λWindowsXP������ж�Ĭ��Ӧ���Ƕ�λ��0x93�ġ�
            ASSERT(old_id == 0x93);
        }
    }
    else
    {
        // �����Ҫ�ָ�...
        old_id = p2cSeachOrSetIrq1(0x93);
        ASSERT(old_id == idle_id);
        // �����Ǹ��ж���û���ˣ�����type = 0ʹ֮����
        idt_addr[old_id].type = 0;
    }
}

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

void p2cUnload(PDRIVER_OBJECT drv)
{
	LARGE_INTEGER interval;
#ifdef BUILD_FOR_IDT_HOOK
    p2cHookInt93(FALSE);
#else
    p2cResetIoApic(FALSE);
#endif
    KdPrint (("p2c: unloading\n")); 
	// ˯��5�롣�ȴ�����irp�������
	interval.QuadPart = (5*1000 * DELAY_ONE_MILLISECOND);		
	KeDelayExecutionThread(KernelMode,FALSE,&interval);
}

NTSTATUS DriverEntry( 
                     IN PDRIVER_OBJECT DriverObject, 
                     IN PUNICODE_STRING RegistryPath 
                     ) 
{ 
    ULONG i; 
    KdPrint (("p2c: entering DriverEntry\n")); 
    // ж�غ�����
    DriverObject->DriverUnload = p2cUnload;
#ifdef BUILD_FOR_IDT_HOOK
    p2cHookInt93(TRUE);
#else
    p2cResetIoApic(TRUE);
#endif
    return  STATUS_SUCCESS; 
}

