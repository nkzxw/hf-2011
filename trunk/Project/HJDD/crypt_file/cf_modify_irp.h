///
/// @file         cf_modify_irp.c
/// @author    crazy_chu
/// @date       2009-2-1
/// @brief      ʵ�ֶ�irp����ͽ�����޸� 
/// 
/// ��ȨЭ��
/// ����������ڹ���crypt_file.��crazy_chuΪ��������������
/// Windows�ں˱������Ϣ��ȫ������д���ļ�͸������ʾ����
/// �����̽���֧��WindowsXP�£�FastFat�ļ�ϵͳ�¼��±���
/// ���ܡ�δ������ɱ��������������ļ�������������������
/// ������ȫ��Ȩ��Ϊ���߱�������������ѧϰ���Ķ�ʹ�á���
/// �������ŵ����ֱ�Ӹ��ơ����߻��ڴ˴�������޸ġ�����
/// �˴����ṩ��ȫ�����߲��ּ���������ҵ���������������
/// �����Ļ�����Ϊ������Υ����ͬ�⸶������ʮ��֮�⳥��
/// �Ķ��˴��룬���Զ���Ϊ����������ȨЭ�顣�粻���ܴ�Э
/// ���ߣ��벻Ҫ�Ķ��˴��롣

#ifndef _CF_MODIFY_IRP_HEADER_
#define _CF_MODIFY_IRP_HEADER_

void cfIrpSetInforPre(
    PIRP irp,
    PIO_STACK_LOCATION irpsp);

void cfIrpQueryInforPost(PIRP irp,PIO_STACK_LOCATION irpsp);

void cfIrpDirectoryControlPost(PIRP irp,PIO_STACK_LOCATION irpsp);

void cfIrpReadPre(PIRP irp,PIO_STACK_LOCATION irpsp);

void cfIrpReadPost(PIRP irp,PIO_STACK_LOCATION irpsp);

BOOLEAN cfIrpWritePre(PIRP irp,PIO_STACK_LOCATION irpsp,void **context);

void cfIrpWritePost(PIRP irp,PIO_STACK_LOCATION irpsp,void *context);

#endif // _CF_MODIFY_IRP_HEADER_