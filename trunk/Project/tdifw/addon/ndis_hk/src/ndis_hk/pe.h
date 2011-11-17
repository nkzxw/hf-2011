// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: pe.h,v 1.2 2003/05/13 12:48:13 dev Exp $

/**
 * @file pe.h
 * Some prototypes from winnt.h ported to compile with driver
 */

#ifndef _pe_h_
#define _pe_h_

#pragma pack(1)

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ

/** DOS .EXE header */
typedef struct _IMAGE_DOS_HEADER {
    USHORT  e_magic;                     /**< Magic number */
    USHORT  e_cblp;                      /**< Bytes on last page of file */
    USHORT  e_cp;                        /**< Pages in file */
    USHORT  e_crlc;                      /**< Relocations */
    USHORT  e_cparhdr;                   /**< Size of header in paragraphs */
    USHORT  e_minalloc;                  /**< Minimum extra paragraphs needed */
    USHORT  e_maxalloc;                  /**< Maximum extra paragraphs needed */
    USHORT  e_ss;                        /**< Initial (relative) SS value */
    USHORT  e_sp;                        /**< Initial SP value */
    USHORT  e_csum;                      /**< Checksum */
    USHORT  e_ip;                        /**< Initial IP value */
    USHORT  e_cs;                        /**< Initial (relative) CS value */
    USHORT  e_lfarlc;                    /**< File address of relocation table */
    USHORT  e_ovno;                      /**< Overlay number */
    USHORT  e_res[4];                    /**< Reserved words */
    USHORT  e_oemid;                     /**< OEM identifier (for e_oeminfo) */
    USHORT  e_oeminfo;                   /**< OEM information; e_oemid specific */
    USHORT  e_res2[10];                  /**< Reserved words */
    LONG   e_lfanew;                     /**< File address of new exe header */
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_EXPORT_DIRECTORY {
    ULONG   Characteristics;
    ULONG   TimeDateStamp;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    ULONG   Name;
    ULONG   Base;
    ULONG   NumberOfFunctions;
    ULONG   NumberOfNames;
    ULONG   AddressOfFunctions;     /**< RVA from base of image */
    ULONG   AddressOfNames;         /**< RVA from base of image */
    ULONG   AddressOfNameOrdinals;  /**< RVA from base of image */
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _IMAGE_FILE_HEADER {
    USHORT  Machine;
    USHORT  NumberOfSections;
    ULONG   TimeDateStamp;
    ULONG   PointerToSymbolTable;
    ULONG   NumberOfSymbols;
    USHORT  SizeOfOptionalHeader;
    USHORT  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

/**
 * Directory format.
 */
typedef struct _IMAGE_DATA_DIRECTORY {
    ULONG   VirtualAddress;
    ULONG   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

/**
 * Optional header format.
 */
typedef struct _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

    USHORT  Magic;
    UCHAR   MajorLinkerVersion;
    UCHAR   MinorLinkerVersion;
    ULONG   SizeOfCode;
    ULONG   SizeOfInitializedData;
    ULONG   SizeOfUninitializedData;
    ULONG   AddressOfEntryPoint;
    ULONG   BaseOfCode;
    ULONG   BaseOfData;

    //
    // NT additional fields.
    //

    ULONG   ImageBase;
    ULONG   SectionAlignment;
    ULONG   FileAlignment;
    USHORT  MajorOperatingSystemVersion;
    USHORT  MinorOperatingSystemVersion;
    USHORT  MajorImageVersion;
    USHORT  MinorImageVersion;
    USHORT  MajorSubsystemVersion;
    USHORT  MinorSubsystemVersion;
    ULONG   Win32VersionValue;
    ULONG   SizeOfImage;
    ULONG   SizeOfHeaders;
    ULONG   CheckSum;
    USHORT  Subsystem;
    USHORT  DllCharacteristics;
    ULONG   SizeOfStackReserve;
    ULONG   SizeOfStackCommit;
    ULONG   SizeOfHeapReserve;
    ULONG   SizeOfHeapCommit;
    ULONG   LoaderFlags;
    ULONG   NumberOfRvaAndSizes;
    
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_NT_HEADERS {
    ULONG						Signature;
    IMAGE_FILE_HEADER			FileHeader;
    IMAGE_OPTIONAL_HEADER32		OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

#define IMAGE_DIRECTORY_ENTRY_EXPORT         0   /**< Export Directory */

#pragma pack()

#endif
