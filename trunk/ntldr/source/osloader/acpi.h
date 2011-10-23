//********************************************************************
//	created:	11:8:2008   0:56
//	file:		acpi.h
//	author:		tiamo
//	purpose:	acpi table
//********************************************************************

#pragma once

#pragma pack(push,1)

//
// "RSD PTR "
//
#define ACPI_RSDP_SIGNATURE								0x2052545020445352

//
// "RSDT"
//
#define ACPI_RSDT_SIGNATURE								0x54445352

//
// "XSDT"
//
#define ACPI_XSDT_SIGNATURE								0x54445358

//
// "DSDT"
//
#define ACPI_DSDT_SIGNATURE								0x54445344

//
// "FACS"
//
#define ACPI_FACS_SIGNATURE								0x53434146

//
// "FACP"
//
#define ACPI_FADT_SIGNATURE								0x50434146

//
// "SPCR"
//
#define ACPI_SPCR_SIGNATURE								0x52435053

//
// "SRAT"
//
#define ACPI_SRAT_SIGNATURE								0x54415253

//
// root system description table pointer
//
typedef struct  _ACPI_RSDP
{
	//
	// 8 UCHAR table signature 'RSD PTR '
	//
	ULONGLONG											Signature;

	//
	// sum of all UCHARs of structure must = 0
	//
	UCHAR												Checksum;

	//
	// string that uniquely ID's the OEM
	//
	UCHAR												OEMID[6];

	//
	// must be 0
	//
	UCHAR												Reserved[1];

	//
	// physical address of Root System Description Table
	//
	ULONG												RsdtAddress;
}ACPI_RSDP,*PACPI_RSDP;

//
// firmware ACPI control,this table does not have a header, it is pointed to by the FADT
//
typedef struct _ACPI_FACS
{
	//
	// 'FACS'
	//
	ULONG												Signature;

	//
	// length of entire firmware ACPI control structure (must be 64 bytes or larger)
	//
	ULONG												Length;

	//
	// hardware signature
	//
	ULONG												HardwareSignature;

	//
	// physical address of location where the OS needs to put the firmware waking vector
	//
	ULONG												pFirmwareWakingVector;

	//
	// 32 bit structure used for sharing Embedded Controller
	//
	ULONG												GlobalLock;

	//
	// flags
	//
	ULONG												Flags;

	//
	// reserved
	//
	UCHAR												Reserved[40];
}ACPI_FACS,*PACPI_FACS;

//
// table header
//
typedef struct _ACPI_DESCRIPTION_HEADER
{
	//
	// signature used to identify the type of table
	//
	ULONG												Signature;

	//
	// length of entire table including the DESCRIPTION_HEADER
	//
	ULONG												Length;

	//
	// minor version of ACPI spec to which this table conforms
	//
	UCHAR												Revision;

	//
	// sum of all bytes in the entire TABLE should = 0
	//
	UCHAR												Checksum;

	//
	// string that uniquely ID's the OEM
	//
	UCHAR												OEMID[6];

	//
	// string that uniquely ID's this table (used for table patching and replacement).
	//
	UCHAR												OEMTableID[8];

	//
	// OEM supplied table revision number.  Bigger number = newer table.
	//
	ULONG												OEMRevision;

	//
	// vendor ID of utility which created this table.
	//
	UCHAR												CreatorID[4];

	//
	// revision of utility that created the table.
	//
	ULONG												CreatorRev;
}ACPI_DESCRIPTION_HEADER,*PACPI_DESCRIPTION_HEADER;

//
// root system description table
//
typedef struct _ACPI_RSDT
{
	//
	// common header
	//
	ACPI_DESCRIPTION_HEADER								Header;

	//
	// the structure contains an n length array of physical addresses each of which point to another table.
	//
	ULONG												Tables[1];
}ACPI_RSDT,*PACPI_RSDT;

//
// extended system description table
//
typedef struct _ACPI_XSDT
{
	//
	// common header
	//
	ACPI_DESCRIPTION_HEADER								Header;

	//
	// the structure contains an n length array of physical addresses each of which point to another table.
	//
	LONGLONG											Tables[1];
}ACPI_XSDT,*PACPI_XSDT;

//
// generic register address
typedef struct _ACPI_GEN_ADDR
{
	//
	// address space type
	//
	UCHAR												AddressSpaceID;

	//
	// width
	//
	UCHAR												BitWidth;

	//
	// offset
	//
	UCHAR												BitOffset;

	//
	// reserved
	//
	UCHAR												Reserved;

	//
	// address
	//
	PHYSICAL_ADDRESS									Address;
}ACPI_GEN_ADDR,*PACPI_GEN_ADDR;

//
// fixed ACPI description table
//
typedef struct _ACPI_FADT
{
	//
	// common header
	//
	ACPI_DESCRIPTION_HEADER								Header;

	//
	// physical address of the Firmware ACPI Control Structure
	//
	ULONG												Facs;

	//
	// physical address of the Differentiated System Description Table
	//
	ULONG												Dsdt;

	//
	// system's Interrupt mode, 0=Dual PIC, 1=Multiple APIC, >1 reserved
	//
	UCHAR												InterruptModel;

	//
	// reserved
	//
	UCHAR												Reserved4;

	//
	// vector of SCI interrupt.
	//
	USHORT												SciInterruptVector;

	//
	// address in System I/O Space of the SMI Command port, used to enable and disable ACPI.
	//
	ULONG												SmiCmdIoPort;

	//
	// value out'd to smi_cmd_port to activate ACPI
	//
	UCHAR												AcpiOnValue;

	//
	// value out'd to smi_cmd_port to deactivate ACPI
	//
	UCHAR												AcpiOffValue;

	//
	// value to write to SMI_CMD to enter the S4 state.
	//
	UCHAR												S4BiosReqValue;

	//
	// reserved
	UCHAR												Reserved1;

	//
	// address in System I/O Space of the PM1a_EVT_BLK register block
	//
	ULONG												Pm1aEvtBlkIoPort;

	//
	// address in System I/O Space of the PM1b_EVT_BLK register block
	//
	ULONG												Pm1bEvtBlkIoPort;

	//
	// address in System I/O Space of the PM1a_CNT_BLK register block
	//
	ULONG												Pm1aCtrlBlkIoPort;

	//
	// address in System I/O Space of the PM1b_CNT_BLK register block
	//
	ULONG												Pm1bCtrlBlkIoPort;

	//
	// address in System I/O Space of the PM2_CNT_BLK register block
	//
	ULONG												Pm2CtrlBlkIoPort;

	//
	// address in System I/O Space of the PM_TMR register block
	//
	ULONG												PmTmrBlkIoPort;

	//
	// address in System I/O Space of the GP0 register block
	//
	ULONG												Gp0BlkIoPort;

	//
	// address in System I/O Space of the GP1 register block
	//
	ULONG												Gp1BlkIoPort;

	//
	// number of bytes decoded for PM1_BLK (must be >= 4)
	//
	UCHAR												Pm1EvtLength;

	//
	// number of bytes decoded for PM1_CNT (must be >= 2)
	//
	UCHAR												Pm1CtrlLength;

	//
	// number of bytes decoded for PM1a_CNT (must be >= 1)
	//
	UCHAR												Pm2CtrlLength;

	//
	// number of bytes decoded for PM_TMR (must be >= 4)
	//
	UCHAR												PmTmrLength;

	//
	// number of bytes decoded for GP0_BLK (must be multiple of 2)
	//
	UCHAR												Gp0BlkLength;

	//
	// number of bytes decoded for GP1_BLK (must be multiple of 2)
	//
	UCHAR												Gp1BlkLength;

	//
	// index at which GP1 based events start
	//
	UCHAR												Gp1Base;

	//
	// reserved
	//
	UCHAR												Reserved2;

	//
	// worst case latency in microseconds required to enter and leave the C2 processor state
	//
	USHORT												LeaveC2Latency;

	//
	// worst case latency in microseconds required to enter and leave the C3 processor state
	//
	USHORT												LeaveC3Latency;

	//
	// ignored if WBINVD flag is 1 -- indicates size of memory read to flush dirty lines from
	//
	USHORT												FlushSize;

	//
	// ignored if WBINVD flag is 1 -- the memory stride width, in bytes, to perform reads to flush
	// any processors memory caches. A size of zero indicates this is not supported.
	//
	USHORT												FlushStride;

	//
	// zero based index of where the processor's duty cycle setting is within the processor's P_CNT register.
	//
	UCHAR												DutyOffset;

	//
	// bit width of the processor's duty cycle setting value in the P_CNT register.
	//
	UCHAR												DutyWidth;

	//
	// day alarm
	//
	UCHAR												DayAlarmIndex;

	//
	// month
	//
	UCHAR												MonthAlarmIndex;

	//
	// century
	//
	UCHAR												CenturyAlarmIndex;

	//
	// boot arch
	//
	USHORT												BootArch;

	//
	// reserved
	//
	UCHAR												Reserved3[1];

	//
	// flags
	//
	ULONG												Flags;

	//
	// reset register
	//
	ACPI_GEN_ADDR										ResetRegister;

	//
	// reset value
	//
	UCHAR												ResetValue;

	//
	// reserved
	//
	UCHAR												Reserved5[3];

	//
	// 64bits facs
	//
	LONGLONG											Facs64;

	//
	// 64bits dsdt
	//
	LONGLONG											Dsdt64;

	//
	// extented PM1a_EVT_BLK register block
	//
	ACPI_GEN_ADDR										ExtPm1aEvtBlk;

	//
	// extented PM1b_EVT_BLK register block
	//
	ACPI_GEN_ADDR										ExtPm1bEvtBlk;

	//
	// extented PM1a_CNT_BLK register block
	//
	ACPI_GEN_ADDR										ExtPm1aCtrlBlk;

	//
	// extented PM1b_CNT_BLK register block
	//
	ACPI_GEN_ADDR										ExtPm1bCtrlBlk;

	//
	// extented PM2_CNT_BLK register block
	//
	ACPI_GEN_ADDR										ExtPm2CtrlBlk;

	//
	// extented PM_TMR register block
	//
	ACPI_GEN_ADDR										ExtPmTmrBlk;

	//
	// extented GP0 register block
	//
	ACPI_GEN_ADDR										ExtGp0Blk;

	//
	// extented GP1 register block
	//
	ACPI_GEN_ADDR										ExtGp1Blk;
}ACPI_FADT,*PACPI_FADT;

//
// serial port console redirection table
//
typedef struct _ACPI_SPCR
{
	//
	// head
	//
	ACPI_DESCRIPTION_HEADER								Header;

	//
	// base address
	//
	ACPI_GEN_ADDR										BaseAddress;

	//
	// interrup type
	//
	UCHAR												InterruptType;

	//
	// irq
	//
	UCHAR												Irq;

	//
	// global system interrupt
	//
	ULONG												GlobalSystemInterrupt;

	//
	// baudrate
	//
	UCHAR												Baudrate;

	//
	// parity
	//
	UCHAR												Parity;

	//
	// stop bits
	//
	UCHAR												StopBits;

	//
	// flow control
	//
	UCHAR												FlowControl;

	//
	// type
	//
	UCHAR												TerminalType;

	//
	// reserved
	//
	UCHAR												Reserved;

	//
	// pci device id
	//
	USHORT												DeviceId;

	//
	// pci vendor id
	//
	USHORT												VendorId;

	//
	// bus
	//
	UCHAR												BusNumber;

	//
	// device
	//
	UCHAR												DeviceNumber;

	//
	// function
	//
	UCHAR												FunctionNumber;

	//
	// flags
	//
	ULONG												Flags;

	//
	// segment
	//
	UCHAR												Segment;

	//
	// reserved
	//
	UCHAR												Reserved2[4];
}ACPI_SPCR,*PACPI_SPCR;

//
// srat
//
typedef struct _ACPI_SRAT
{
	//
	// header
	//
	ACPI_DESCRIPTION_HEADER								Header;

	//
	// reserver
	//
	ULONG												Reserved[3];
}ACPI_SRAT,*PACPI_SRAT;

//
// memory srat
//
typedef struct _ACPI_MEMORY_AFFINITY
{
	//
	// type
	//
	UCHAR												Type;

	//
	// length
	//
	UCHAR												Length;

	//
	// proximity domain
	//
	ULONG												ProximityDomain;

	//
	// reserved
	//
	USHORT												Reserved;

	//
	// base address
	//
	ULONGLONG											BaseAddress;

	//
	// length
	//
	ULONGLONG											MemoryLength;

	//
	// reserved
	//
	ULONG												Reserved1;

	//
	// enabled
	//
	ULONG												Enabled : 1;

	//
	// hotpluggable
	//
	ULONG												HotPluggable : 1;

	//
	// reserved
	//
	ULONG												Reserved3 : 30;

	//
	// reserved
	//
	ULONG												Reserved4[2];
}ACPI_MEMORY_AFFINITY,*PACPI_MEMORY_AFFINITY;

#pragma pack(pop)

//
// get redirection info
//
BOOLEAN BlRetrieveBIOSRedirectionInformation(__inout PLOADER_REDIRECTION_INFORMATION Info);

//
// detect legacy free bios
//
BOOLEAN BlDetectLegacyFreeBios();

//
// load guid
//
VOID BlLoadGUID(__out GUID* Guid);

//
// check SRAT acpi table
//
BOOLEAN Blx86NeedPaeForHotPlugMemory();