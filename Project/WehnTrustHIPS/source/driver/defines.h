/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_DEFINES_H
#define _WEHNTRUST_DRIVER_DEFINES_H

////
//
// General Constants
//
////


//
// Architecture alignment requirement in bytes
//
#define ARCHITECTURE_ALIGNMENT_REQUIREMENT 1

//
// Architecture specific page size
//
#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#ifndef PAGE_ALIGN
#define PAGE_ALIGN(x) ((ULONG_PTR)(x) & ~(PAGE_SIZE - 1))
#endif

//
// This define controls the size of the buffer used when reading data in from
// files for checksum computation.
//
#define CHECKSUM_BUFFER_SIZE 16384

//
// The registry key path where driver configuration is stored
//
#define WEHNTRUST_REGISTRY_CONFIG_PATH L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\baserand"

//
// System level configuration that is shared by the user-mode applications and
// the device driver.
//
#define WEHNTRUST_REGISTRY_SYSCONFIG_PATH WEHNTRUST_REGISTRY_CONFIG_PATH L"\\Config"

//
// The physical base storage path that randomized image files should be stored
// at.
//
#define RANDOMIZED_FILE_STORAGE_PATH L"\\SystemRoot\\RandCache\\"

//
// The physical path to the NRER user-mode DLL.
//
#define NRER_USER_MODE_DLL_PATH      L"\\SystemRoot\\System32\\NRER.dll"

////
//
// Performance
//
////

//
// This define expresses the period to use when triggering the expiration of
// unreferenced image mappings.  When the period expires, a timer object is
// signaled and each image set's mapping list is enumerated.  In each set,
// any mapping that is found to have no references other than the one held by the
// set itself is forcibly expired, thus freeing up memory from being
// unnecessarily held.  This period may at some point need to be calculated
// based off the amount of avaiable RAM on the machine.  For now the period is
// 15 minutes, as the number of images loaded during that time span should be
// minimal.
//
// This period is measured in milliseconds.
//
#define MAPPING_EXPIRATION_PERIOD 900000

////
//
// Randomization
//
////

//
// This define disables randomization of library images if defined.  This is not
// defined by default.
//
//#define DISABLE_LIBRARY_RANDOMIZATION

//
// This define disables the randomization of stack/heap sections.  This is not
// defined by default.
//
//#define DISABLE_STACKHEAP_RANDOMIZATION

//
// This define disables the randomization of PEB/TEB.  This is not defined by
// default.
//
//#define DISABLE_PEBTEB_RANDOMIZATION

//
// This define disables the randomization of non-relocatable executables.  This
// is defined by default.
//
#define DISABLE_NRE_RANDOMIZATION

//
// Added in 0.9.7:
//
// This define disables SEH overwrite prevention.  This is not defined by
// default.
//
//#define DISABLE_SEH_OVERWRITE_PREVENTION


//
// This define allows you to disable McAfee compatibility hacks that could
// introduce subtle privilege escalation vulnerabilities by allowing
// in-process randomization.
//
// For best security this define should be enabled.  However, for compatibility
// reasons, it is disabled by default.
//
//#define DISABLE_MCAFEE_COMPAT_HACK

////
//
// Image Sets
//
////

//
// Undefine this to enable support for multiple image sets in the driver.
// Currently, there is not enough code to support this, but in the future there
// will be.
//
// #define MULTIPLE_IMAGE_SETS

//
// This define controls the amount of randomization that can be obtained for an
// actual image base address.  This base address is not restricted to any
// alignment considerations other than that which are held by the architecture
// being compiled against (cache alignment, for instance).  For this reason it
// allows for a much wider range of optizmiation than would otherwise be
// possible when limiting one's self to 16-page or 1-page alignment of the base
// address.
//
// In the future this should be set to cache alignment if possible.  At the
// moment there is some problem with the code or with some oddity in the loader
// that causes images loaded at non-16 page aligned addresses to fail to walk
// the import descriptor:
//
// LDR: LdrpWalkImportDescriptor() failed to probe C:\WINDOWS\system32\netman.dll for its manifest, ntstatus 0xc0000005
// 
//
#define BASE_RANDOMIZATION_IMAGE_MASK   0x7fff0000

//
// This define is used to mask a randomized image base to the actual base
// address of the memory range that will be allocated by the underlying memory
// manager.  On 32-bit versions of Windows, VAD entries are allocated along 64KB
// boundaries, and as such this mask is used to align an allocated image base
// along those boundaries.  The randomizer can them compensate for the base
// address adjustment between the actual image base and the mapping base by
// tracking the offset between the two.  
//
// This works perfectly fine due to the fact that unmapping of views can be done
// by passing any virtual address that exists inside a section, and as such the
// fact that the actual base address of the mapping is not passed back is of no
// concern.
//
// The default mapping base alignment is on 16-page boundaries (64KB) for IA32
//
#define BASE_RANDOMIZATION_MAPPING_MASK 0x7fff0000

#endif
