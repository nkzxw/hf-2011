//********************************************************************
//	created:	11:8:2008   19:09
//	file:		comport.cpp
//	author:		tiamo
//	purpose:	comport support
//********************************************************************

#include "stdafx.h"

//
// read routine
//
typedef UCHAR (*COM_PORT_READ_UCHAR)(__in PUCHAR Port);

//
// write routine
//
typedef VOID (*COM_PORT_WRITE_UCHAR)(__in PUCHAR Port,__in UCHAR Value);

//
// data port
//
#define COM_DAT											0x00

//
// divisor latch least sig
//
#define COM_DLL											0x00

//
// interrupt enable register
//
#define COM_IEN											0x01

//
// divisor latch most sig
//
#define COM_DLM											0x01

//
// FIFO control
//
#define COM_FCR											0x02

//
// line control registers
//
#define COM_LCR											0x03

//
// modem control reg
//
#define COM_MCR											0x04

//
// line status register
//
#define COM_LSR											0x05

//
// modem status register
//
#define COM_MSR											0x06

//
// this bit controls the loopback testing mode of the device.  basically the outputs are connected to the inputs (and vice versa).
//
#define SERIAL_MCR_LOOP									0x10

//
// this bit is used for general purpose output.
//
#define SERIAL_MCR_OUT1									0x04

//
// this bit contains the (complemented) state of the clear to send (CTS) line.
//
#define SERIAL_MSR_CTS									0x10

//
// this bit contains the (complemented) state of the data set ready (DSR) line.
//
#define SERIAL_MSR_DSR									0x20

//
// this bit contains the (complemented) state of the ring indicator (RI) line.
//
#define SERIAL_MSR_RI									0x40

//
// this bit contains the (complemented) state of the data carrier detect (DCD) line.
//
#define SERIAL_MSR_DCD									0x80

//
// data ready
//
#define COM_DATRDY										0x01

//
// overrun error
//
#define COM_OE											0x02

//
// parity error
//
#define COM_PE											0x04

//
// framing error
//
#define COM_FE											0x08

//
// break interrupt
//
#define COM_BI											0x10

//
// output ready
//
#define COM_OUTRDY										0x20

//
// divisor latch access bit
//
#define LC_DLAB											0x80

//
// usart clock rate
//
#define CLOCK_RATE										0x1c200

//
// control bits to assert DTR and RTS
//
#define MC_DTRRTS										0x03

//
// status bits for dsr, cts and cd
//
#define MS_DSRCTSCD										0xB0

//
// carry detected
//
#define MS_CD											0x80

//
// baud rate not specified, using default
//
#define PORT_DEFAULTRATE								0x0001

//
// using modem controls
//
#define PORT_MODEMCONTROL								0x0002

//
// port is in saved state
//
#define PORT_SAVED										0x0004

//
// 'Carrier detect lost' time not set
//
#define PORT_NOCDLTIME									0x0010

//
// display baud rate abbrv
//
#define PORT_DISBAUD									0x0020

//
// sending modem string (don't recurse)
//
#define PORT_SENDINGSTRING								0x0040

//
// CD while in modem control mode
//
#define PORT_MDM_CD										0x0080

//
// com port struct
//
typedef struct _CPPORT
{
	//
	// base address
	//
	PUCHAR												Address;

	//
	// baudrate
	//
	ULONG												Baud;

	//
	// flags
	//
	USHORT												Flags;
}CPPORT,*PCPPORT;

//
// ring flags
//
UCHAR													CpRingFlags;

//
// port info
//
CPPORT													Port[4];

//
// read com port
//
COM_PORT_READ_UCHAR										READ_UCHAR = &READ_PORT_UCHAR;

//
// write com port
//
COM_PORT_WRITE_UCHAR									WRITE_UCHAR	= &WRITE_PORT_UCHAR;

//
// check port exist
//
BOOLEAN CpDoesPortExist(__in PUCHAR Address)
{
	BOOLEAN ReturnValue									= TRUE;

	//
	// save the old value of the modem control register.
	//
	UCHAR OldModemStatus								= READ_UCHAR(Address + COM_MCR);

	//
	// set the port into diagnostic mode.
	//
	WRITE_UCHAR(Address + COM_MCR,SERIAL_MCR_LOOP);

	//
	// bang on it again to make sure that all the lower bits are clear.
	//
	WRITE_UCHAR(Address + COM_MCR,SERIAL_MCR_LOOP);

	//
	// read the modem status register. the high for bits should be clear.
	//
	UCHAR ModemStatus									= READ_UCHAR(Address + COM_MSR);

	if(ModemStatus & (SERIAL_MSR_CTS | SERIAL_MSR_DSR | SERIAL_MSR_RI  | SERIAL_MSR_DCD))
	{
		ReturnValue = FALSE;
	}
	else
	{
		//
		// so far so good.now turn on OUT1 in the modem control register and this should turn on ring indicator in the modem status register.
		//
		WRITE_UCHAR(Address + COM_MCR,SERIAL_MCR_OUT1 | SERIAL_MCR_LOOP);

		ModemStatus										= READ_UCHAR(Address + COM_MSR);

		if(!(ModemStatus & SERIAL_MSR_RI))
			ReturnValue = FALSE;
	}

	//
	// put the modem control back into a clean state.
	//
	WRITE_UCHAR(Address + COM_MCR,OldModemStatus);

	return ReturnValue;

}

//
// enable fifo
//
VOID CpEnableFifo(__in PUCHAR Address,__in UCHAR Fifo)
{
	WRITE_UCHAR(Address + COM_FCR,Fifo);
}

//
// read lsr
//
UCHAR CpReadLsr(__in PCPPORT Port,__in UCHAR WaitingFlags)
{
	//
	// read lsr
	//
	UCHAR lsr											= READ_UCHAR(Port->Address + COM_LSR);

	//
	// caller is waiting for it
	//
	if(lsr & WaitingFlags)
		return lsr;

	//
	// read msr to check ring flags
	//
	UCHAR msr											= READ_UCHAR(Port->Address + COM_MSR);
	CpRingFlags											|= (msr & SERIAL_MSR_RI) ? 1 : 2;
	if(CpRingFlags == 3)
		Port->Flags										|= PORT_MODEMCONTROL;

	return lsr;
}

//
// set baudrate
//
VOID CpSetBaud(__in PCPPORT Port,__in ULONG Rate)
{
	//
	// compute the divsor
	//
	ULONG Divisorlatch									= CLOCK_RATE / Rate;

	//
	// set the divisor latch access bit (DLAB) in the line control reg
	//
	UCHAR lcr											= READ_UCHAR(Port->Address + COM_LCR);
	lcr													|= LC_DLAB;
	WRITE_UCHAR(Port->Address + COM_LCR,lcr);

	//
	// set the divisor latch value.
	//
	WRITE_UCHAR(Port->Address + COM_DLM,static_cast<UCHAR>((Divisorlatch >> 8) & 0xff));

	//
	// divisor latch lsb
	//
	WRITE_UCHAR(Port->Address + COM_DLL,static_cast<UCHAR>(Divisorlatch & 0xff));

	//
	// set LCR to 3.  (3 is a magic number in the original assembler)
	//
	WRITE_UCHAR(Port->Address + COM_LCR,3);

	//
	// remember the baud rate
	//
	Port->Baud											= Rate;
}

//
// fetch a byte
//
ULONG CpGetByte(__in PCPPORT  Port,__out PUCHAR Byte,__in BOOLEAN WaitForByte,__in BOOLEAN CheckOnly)
{
	//
	// check to make sure the CPPORT we were passed has been initialized.
	//
	if(!Port->Address)
		return CP_GET_NODATA;

	ULONG LimitCount								= WaitForByte ? 204800 : 1;
	while(LimitCount)
	{
		LimitCount									-= 1;

		//
		// check data ready
		//
		UCHAR lsr									= CpReadLsr(Port,COM_DATRDY);
		if((lsr & COM_DATRDY) == COM_DATRDY)
		{
			//
			// check for errors
			//
			if(lsr & (COM_FE | COM_PE | COM_OE))
			{
				*Byte = 0;
				return CP_GET_ERROR;
			}

			//
			// check data ready only
			//
			if(CheckOnly)
				return CP_GET_SUCCESS;

			//
			// fetch the byte
			//
			UCHAR value								= READ_PORT_UCHAR(Port->Address + COM_DAT);

			if(Port->Flags & PORT_MODEMCONTROL)
			{
				//
				// using modem control. if no CD, then skip this byte.
				//
				if((READ_UCHAR(Port->Address + COM_MSR) & MS_CD) == 0)
					continue;
			}

			*Byte									= value & 0xff;

			return CP_GET_SUCCESS;
		}
	}

	CpReadLsr(Port,0);

	return CP_GET_NODATA;
}

//
// send byte
//
VOID CpPutByte(__in PCPPORT Port,__in UCHAR Byte)
{
	//
	// if modem control, make sure DSR, CTS and CD are all set before sending any data.
	//
	UCHAR msr;
	while((Port->Flags & PORT_MODEMCONTROL)  && (msr = READ_UCHAR(Port->Address + COM_MSR) & MS_DSRCTSCD) != MS_DSRCTSCD)
	{
		//
		// if no CD, and there's a charactor ready, eat it
		//
		if((msr & MS_CD) == 0  && (CpReadLsr(Port,0) & COM_DATRDY) == COM_DATRDY)
			READ_UCHAR(Port->Address + COM_DAT);
	}

	//
	//  wait for port to not be busy
	//
	while(!(CpReadLsr(Port, COM_OUTRDY) & COM_OUTRDY));

	//
	// send the byte
	//
	WRITE_UCHAR(Port->Address + COM_DAT, Byte);
}

//
// initialize port
//
VOID CpInitialize(__in PCPPORT Port,__in PUCHAR Address,__in ULONG Rate)
{
	Port->Address										= Address;
	Port->Baud											= 0;

	CpSetBaud(Port,Rate);

	//
	// assert DTR, RTS.
	//
	WRITE_UCHAR(Port->Address + COM_MCR,MC_DTRRTS);

	//
	// disable interrupt
	//
	WRITE_UCHAR(Port->Address + COM_IEN,0);
}

//
// wrapper enable fifo
//
VOID BlEnableFifo(__in ULONG DeviceId,__in UCHAR Fifo)
{
	CpEnableFifo(Port[DeviceId].Address,Fifo);
}

//
// wrapper get byte,block wait
//
ULONG BlPortGetByte(__in ULONG DeviceId,__out PUCHAR Value)
{
	return CpGetByte(Port + DeviceId,Value,TRUE,FALSE);
}

//
// wrapper put byte
//
VOID BlPortPutByte(__in ULONG DeviceId,__in UCHAR Value)
{
	CpPutByte(Port + DeviceId,Value);
}

//
// poll port
//
ULONG BlPortPollOnly(__in ULONG DeviceId)
{
	UCHAR Dummy;
	return CpGetByte(Port + DeviceId,&Dummy,FALSE,TRUE);
}

//
// poll byte,without wait
//
ULONG BlPortPollByte(__in ULONG DeviceId,__out PUCHAR Value)
{
	return CpGetByte(Port + DeviceId,Value,FALSE,FALSE);
}

//
// initialize port
//
BOOLEAN BlPortInitialize(__in ULONG Baudrate,__in ULONG PortNum,__in ULONG PortAddr,__in BOOLEAN ReInitialize,__out PULONG DeviceId)
{
	//
	// set default baudrate if caller did not give us
	//
	if(!Baudrate)
		Baudrate										= 19200;

	//
	// setup function pointers
	//
	if(LoaderRedirectionInfo.MemorySpace)
	{
		READ_UCHAR										= &READ_REGISTER_UCHAR;
		WRITE_UCHAR										= &WRITE_REGISTER_UCHAR;
	}
	else
	{

		READ_UCHAR										= &READ_PORT_UCHAR;
		WRITE_UCHAR										= &WRITE_PORT_UCHAR;
	}

	if(!PortNum)
	{
		//
		// try COM1,COM2 with default value
		//
		PortAddr										= 0x3f8;
		PortNum											= 2;
		if(CpDoesPortExist(reinterpret_cast<PUCHAR>(0x2f8)))
			PortAddr									= 0x2f8;
		else if(CpDoesPortExist(reinterpret_cast<PUCHAR>(0x3f8)))
			PortNum										= 3;
		else
			return FALSE;
	}
	else if(!PortAddr)
	{
		switch(PortNum)
		{
		case 1:
			PortAddr									= 0x3f8;
			break;

		case 2:
			PortAddr									= 0x2f8;
			break;

		case 3:
			PortAddr									= 0x3e8;
			break;

		default:
			PortAddr									= 0x2e8;
			PortNum										= 4;
			break;
		}
	}

	PUCHAR BaseAddress									= 0;
	if(LoaderRedirectionInfo.MemorySpace)
	{
		//
		// map registers
		//
		PHYSICAL_ADDRESS Address;
		Address.QuadPart								= PortAddr;
		BaseAddress										= static_cast<PUCHAR>(MmMapIoSpace(Address,7,MmNonCached));
	}
	else
	{
		BaseAddress										= reinterpret_cast<PUCHAR>(PortAddr);
	}


	//
	// if base address is a memory mapped register,WHY?
	//
	if(!CpDoesPortExist(BaseAddress) && !LoaderRedirectionInfo.MemorySpace)
		return FALSE;

	//
	// for reinitialize request,port object's base address must not be zero
	// otherwise port object's base address must be zero
	//
	if(ReInitialize == !Port[PortNum - 1].Address)
		return FALSE;

	CpInitialize(Port + PortNum - 1,BaseAddress,Baudrate);

	//
	// device id is port number - 1
	//
	*DeviceId											= PortNum - 1;

	return TRUE;
}