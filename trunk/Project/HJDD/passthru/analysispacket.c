#include "precomp.h"
#include "ntstrsafe.h"

typedef struct in_addr {
	union {
		struct { UCHAR s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { USHORT s_w1,s_w2; } S_un_w;
		ULONG S_addr;
	} S_un;
} IN_ADDR, *PIN_ADDR, FAR *LPIN_ADDR;

typedef struct IP_HEADER
{
	unsigned char  VIHL;          // Version and IHL
	unsigned char  TOS;           // Type Of Service
	short          TotLen;        // Total Length
	short          ID;            // Identification
	short          FlagOff;       // Flags and Fragment Offset
	unsigned char  TTL;           // Time To Live
	unsigned char  Protocol;      // Protocol
	unsigned short Checksum;      // Checksum
	struct in_addr        iaSrc;  // Internet Address - Source
	struct in_addr        iaDst;  // Internet Address - Destination
}IP_HEADER, *PIP_HEADER;

#define IP_OFFSET                               0x0E

//IP Protocol Types
#define PROT_ICMP                               0x01 
#define PROT_TCP                                0x06 
#define PROT_UDP                                0x11 


// ���������
//	Packet�� ��������NDIS��������
//	bRecOrSend: ����ǽ��հ���ΪTRUE;���Ϊ���Ͱ���ΪFALSE��
// ����ֵ��
//	���������£�������ͨ������ֵ�Ծ�����δ���NDIS����������ʧ�ܡ�ת��
FILTER_STATUS AnalysisPacket(PNDIS_PACKET Packet,  BOOLEAN bRecOrSend)
{
	FILTER_STATUS status = STATUS_PASS; // Ĭ��ȫ��ͨ��
	PNDIS_BUFFER NdisBuffer ;
	UINT TotalPacketLength = 0;
	UINT copysize = 0;
	UINT DataOffset = 0 ;
	UINT PhysicalBufferCount;
	UINT BufferCount   ;
	PUCHAR pPacketContent = NULL;
	char* tcsPrintBuf = NULL;
	PUCHAR tembuffer = NULL ; 
	UINT j;

	__try{

		status = NdisAllocateMemoryWithTag( &pPacketContent, 2048, TAG); 
		if( status != NDIS_STATUS_SUCCESS ){
			status = NDIS_STATUS_FAILURE ;
			__leave;
		}

		NdisZeroMemory( pPacketContent, 2048 ) ;

		// �ҵ���һ��Ndis_Buffer��Ȼ��ͨ��ͨ��NdisGetNextBuffer����ú�����NDIS_BUFFER��
		// ���ֻ���ҵ�һ���ڵ㣬�����ҷ���ķ����ǵ���NdisGetFirstBufferFromPacket��
		NdisQueryPacket(Packet,  // NDIS_PACKET        
			&PhysicalBufferCount,// �ڴ��е��������
			&BufferCount,		 // ���ٸ�NDIS_BUFFER��
			&NdisBuffer,         // �����ص�һ����
			&TotalPacketLength	 // �ܹ��İ����ݳ���
			);

		while(TRUE){

			// ȡ��Ndis_Buffer�д洢�������������ַ��
			// �����������һ���汾��NdisQueryBuffer��
			// ������ϵͳ��Դ�ͻ��������ľ���ʱ�򣬻����Bug Check������������
			NdisQueryBufferSafe(NdisBuffer,
				&tembuffer,// ��������ַ
				&copysize, // ��������С
				NormalPagePriority
				);

			// ���tembufferΪNULL��˵����ǰϵͳ��Դ�ѷ���
			if(tembuffer != NULL){
				NdisMoveMemory( pPacketContent + DataOffset , tembuffer, copysize) ;			
				DataOffset += copysize;
			}

			// �����һ��NDIS_BUFFER��
			// ����õ�����һ��NULLָ�룬˵���Ѿ�������ʽ��������ĩβ�����ǵ�ѭ��Ӧ�ý����ˡ�
			NdisGetNextBuffer(NdisBuffer , &NdisBuffer ) ;

			if( NdisBuffer == NULL )
				break ;
		}

		// ȡ�����ݰ����ݺ����潫�������ݽ��й��ˡ�
		// ��������������е�ʵ�֣������򵥵ش�ӡһЩ�ɶ���Log��Ϣ��
		if(pPacketContent[12] == 8 &&  pPacketContent[13] == 0 )  //is ip packet
		{	
			PIP_HEADER pIPHeader = (PIP_HEADER)(pPacketContent + IP_OFFSET);
			switch(pIPHeader->Protocol)
			{
			case PROT_ICMP:
				if(bRecOrSend)
					DbgPrint("Receive ICMP packet");
				else
					DbgPrint("Send ICMP packet");

				//
				// ȡ��ICMPͷ��������Ĺ����жϡ�
				// 

				break;
			case PROT_UDP:
				if(bRecOrSend)
					DbgPrint("Receive UDP packet");
				else
					DbgPrint("Send UDP packet");

				//
				// ȡ��UDPͷ��������Ĺ����жϡ�
				//

				break;
			case PROT_TCP:
				if(bRecOrSend)
					DbgPrint("Receive TCP packet");
				else
					DbgPrint("Send TCP packet");

				//
				// ȡ��TCPͷ��������Ĺ����жϡ�
				//

				break;
			}
		}else if(pPacketContent[12] == 8 &&  pPacketContent[13] == 6 ){
			if(bRecOrSend)
				DbgPrint("Receive ARP packet");
			else
				DbgPrint("Send ARP packet");
		}else{
			if(bRecOrSend)
				DbgPrint("Receive unknown packet");
			else
				DbgPrint("Send unknown packet");
		}

		// �򵥴�ӡ������������
		status = NdisAllocateMemoryWithTag( &tcsPrintBuf, 2048*3, TAG);  //�����ڴ��
		if( status != NDIS_STATUS_SUCCESS ){
			status = NDIS_STATUS_FAILURE ;
			__leave;
		}
		for(j=0;j<=DataOffset;j++)
			RtlStringCbPrintfA(tcsPrintBuf+j*3, 2048*3-j*3, "%2x ",pPacketContent[j]);

		DbgPrint(tcsPrintBuf);

	}__finally{
		if(pPacketContent)NdisFreeMemory(pPacketContent, 0, 0);
		if(tcsPrintBuf)NdisFreeMemory(tcsPrintBuf, 0, 0);
	}

	return STATUS_PASS;
}