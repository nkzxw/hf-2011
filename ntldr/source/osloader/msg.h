//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: BL_MSG_FIRST
//
// MessageText:
//
// BL_MSG_FIRST
//
#define BL_MSG_FIRST                     0x00002328L

//
// MessageId: LOAD_SW_INT_ERR_CLASS
//
// MessageText:
//
// Windows could not start because of an error in the software.
// Please report this problem as :
//
#define LOAD_SW_INT_ERR_CLASS            0x00002329L

//
// MessageId: LOAD_SW_MISRQD_FILE_CLASS
//
// MessageText:
//
// Windows could not start because the following file was not found
// and is required :
//
#define LOAD_SW_MISRQD_FILE_CLASS        0x0000232AL

//
// MessageId: LOAD_SW_BAD_FILE_CLASS
//
// MessageText:
//
// Windows could not start because of a bad copy of the 
// following file :
//
#define LOAD_SW_BAD_FILE_CLASS           0x0000232BL

//
// MessageId: LOAD_SW_MIS_FILE_CLASS
//
// MessageText:
//
// Windows could not start because the following file is missing 
// or corrupt:
//
#define LOAD_SW_MIS_FILE_CLASS           0x0000232CL

//
// MessageId: LOAD_HW_MEM_CLASS
//
// MessageText:
//
// Windows could not start because of a hardware memory 
// configuration problem.
//
#define LOAD_HW_MEM_CLASS                0x0000232DL

//
// MessageId: LOAD_HW_DISK_CLASS
//
// MessageText:
//
// Windows could not start because of a computer disk hardware
// configuration problem.
//
#define LOAD_HW_DISK_CLASS               0x0000232EL

//
// MessageId: LOAD_HW_GEN_ERR_CLASS
//
// MessageText:
//
// Windows could not start because of a general computer hardware
// configuration problem.
//
#define LOAD_HW_GEN_ERR_CLASS            0x0000232FL

//
// MessageId: LOAD_HW_FW_CFG_CLASS
//
// MessageText:
//
// Windows could not start because of the following ARC firmware
// boot configuration problem :
//
#define LOAD_HW_FW_CFG_CLASS             0x00002330L

//
// MessageId: DIAG_BL_MEMORY_INIT
//
// MessageText:
//
// Check hardware memory configuration and amount of RAM.
//
#define DIAG_BL_MEMORY_INIT              0x00002331L

//
// MessageId: DIAG_BL_CONFIG_INIT
//
// MessageText:
//
// Too many configuration entries.
//
#define DIAG_BL_CONFIG_INIT              0x00002332L

//
// MessageId: DIAG_BL_IO_INIT
//
// MessageText:
//
// Could not access disk partition tables
//
#define DIAG_BL_IO_INIT                  0x00002333L

//
// MessageId: DIAG_BL_FW_GET_BOOT_DEVICE
//
// MessageText:
//
// The 'osloadpartition' parameter setting is invalid.
//
#define DIAG_BL_FW_GET_BOOT_DEVICE       0x00002334L

//
// MessageId: DIAG_BL_OPEN_BOOT_DEVICE
//
// MessageText:
//
// Could not read from the selected boot disk.  Check boot path
// and disk hardware.
//
#define DIAG_BL_OPEN_BOOT_DEVICE         0x00002335L

//
// MessageId: DIAG_BL_FW_GET_SYSTEM_DEVICE
//
// MessageText:
//
// The 'systempartition' parameter setting is invalid.
//
#define DIAG_BL_FW_GET_SYSTEM_DEVICE     0x00002336L

//
// MessageId: DIAG_BL_FW_OPEN_SYSTEM_DEVICE
//
// MessageText:
//
// Could not read from the selected system boot disk.
// Check 'systempartition' path.
//
#define DIAG_BL_FW_OPEN_SYSTEM_DEVICE    0x00002337L

//
// MessageId: DIAG_BL_GET_SYSTEM_PATH
//
// MessageText:
//
// The 'osloadfilename' parameter does not point to a valid file.
//
#define DIAG_BL_GET_SYSTEM_PATH          0x00002338L

//
// MessageId: DIAG_BL_LOAD_SYSTEM_IMAGE
//
// MessageText:
//
// <Windows root>\system32\ntoskrnl.exe.
//
#define DIAG_BL_LOAD_SYSTEM_IMAGE        0x00002339L

//
// MessageId: DIAG_BL_FIND_HAL_IMAGE
//
// MessageText:
//
// The 'osloader' parameter does not point to a valid file.
//
#define DIAG_BL_FIND_HAL_IMAGE           0x0000233AL

//
// MessageId: DIAG_BL_LOAD_HAL_IMAGE_X86
//
// MessageText:
//
// <Windows root>\system32\hal.dll.
//
#define DIAG_BL_LOAD_HAL_IMAGE_X86       0x0000233BL

//
// MessageId: DIAG_BL_LOAD_HAL_IMAGE_ARC
//
// MessageText:
//
// 'osloader'\hal.dll
//
#define DIAG_BL_LOAD_HAL_IMAGE_ARC       0x0000233CL

//
// MessageId: DIAG_BL_LOAD_SYSTEM_IMAGE_DATA
//
// MessageText:
//
// Loader error 1.
//
#define DIAG_BL_LOAD_SYSTEM_IMAGE_DATA   0x0000233DL

//
// MessageId: DIAG_BL_LOAD_HAL_IMAGE_DATA
//
// MessageText:
//
// Loader error 2.
//
#define DIAG_BL_LOAD_HAL_IMAGE_DATA      0x0000233EL

//
// MessageId: DIAG_BL_LOAD_SYSTEM_DLLS
//
// MessageText:
//
// load needed DLLs for kernel.
//
#define DIAG_BL_LOAD_SYSTEM_DLLS         0x0000233FL

//
// MessageId: DIAG_BL_LOAD_HAL_DLLS
//
// MessageText:
//
// load needed DLLs for HAL.
//
#define DIAG_BL_LOAD_HAL_DLLS            0x00002340L

//
// MessageId: DIAG_BL_FIND_SYSTEM_DRIVERS
//
// MessageText:
//
// find system drivers.
//
#define DIAG_BL_FIND_SYSTEM_DRIVERS      0x00002342L

//
// MessageId: DIAG_BL_READ_SYSTEM_DRIVERS
//
// MessageText:
//
// read system drivers.
//
#define DIAG_BL_READ_SYSTEM_DRIVERS      0x00002343L

//
// MessageId: DIAG_BL_LOAD_DEVICE_DRIVER
//
// MessageText:
//
// did not load system boot device driver.
//
#define DIAG_BL_LOAD_DEVICE_DRIVER       0x00002344L

//
// MessageId: DIAG_BL_LOAD_SYSTEM_HIVE
//
// MessageText:
//
// load system hardware configuration file.
//
#define DIAG_BL_LOAD_SYSTEM_HIVE         0x00002345L

//
// MessageId: DIAG_BL_SYSTEM_PART_DEV_NAME
//
// MessageText:
//
// find system partition name device name.
//
#define DIAG_BL_SYSTEM_PART_DEV_NAME     0x00002346L

//
// MessageId: DIAG_BL_BOOT_PART_DEV_NAME
//
// MessageText:
//
// find boot partition name.
//
#define DIAG_BL_BOOT_PART_DEV_NAME       0x00002347L

//
// MessageId: DIAG_BL_ARC_BOOT_DEV_NAME
//
// MessageText:
//
// did not properly generate ARC name for HAL and system paths.
//
#define DIAG_BL_ARC_BOOT_DEV_NAME        0x00002348L

//
// MessageId: DIAG_BL_SETUP_FOR_NT
//
// MessageText:
//
// Loader error 3.
//
#define DIAG_BL_SETUP_FOR_NT             0x0000234AL

//
// MessageId: DIAG_BL_KERNEL_INIT_XFER
//
// MessageText:
//
// <Windows root>\system32\ntoskrnl.exe
//
#define DIAG_BL_KERNEL_INIT_XFER         0x0000234BL

//
// MessageId: LOAD_SW_INT_ERR_ACT
//
// MessageText:
//
// Please contact your support person to report this problem.
//
#define LOAD_SW_INT_ERR_ACT              0x0000234CL

//
// MessageId: LOAD_SW_FILE_REST_ACT
//
// MessageText:
//
// You can attempt to repair this file by starting Windows Setup
// using the original Setup CD-ROM.
// Select 'r' at the first screen to start repair.
//
#define LOAD_SW_FILE_REST_ACT            0x0000234DL

//
// MessageId: LOAD_SW_FILE_REINST_ACT
//
// MessageText:
//
// Please re-install a copy of the above file.
//
#define LOAD_SW_FILE_REINST_ACT          0x0000234EL

//
// MessageId: LOAD_HW_MEM_ACT
//
// MessageText:
//
// Please check the Windows documentation about hardware memory
// requirements and your hardware reference manuals for
// additional information.
//
#define LOAD_HW_MEM_ACT                  0x0000234FL

//
// MessageId: LOAD_HW_DISK_ACT
//
// MessageText:
//
// Please check the Windows documentation about hardware disk
// configuration and your hardware reference manuals for
// additional information.
//
#define LOAD_HW_DISK_ACT                 0x00002350L

//
// MessageId: LOAD_HW_GEN_ACT
//
// MessageText:
//
// Please check the Windows documentation about hardware
// configuration and your hardware reference manuals for additional
// information.
//
#define LOAD_HW_GEN_ACT                  0x00002351L

//
// MessageId: LOAD_HW_FW_CFG_ACT
//
// MessageText:
//
// Please check the Windows documentation about ARC configuration
// options and your hardware reference manuals for additional
// information.
//
#define LOAD_HW_FW_CFG_ACT               0x00002352L

//
// MessageId: BL_LKG_MENU_HEADER
//
// MessageText:
//
//      Hardware Profile/Configuration Recovery Menu
// 
// This menu allows you to select a hardware profile
// to be used when Windows is started.
// 
// If your system is not starting correctly, then you may switch to a 
// previous system configuration, which may overcome startup problems.
// IMPORTANT: System configuration changes made since the last successful
// startup will be discarded.
//
#define BL_LKG_MENU_HEADER               0x00002353L

//
// MessageId: BL_LKG_MENU_TRAILER
//
// MessageText:
//
// Use the up and down arrow keys to move the highlight
// to the selection you want. Then press ENTER.
//
#define BL_LKG_MENU_TRAILER              0x00002354L

//
// MessageId: BL_LKG_MENU_TRAILER_NO_PROFILES
//
// MessageText:
//
// No hardware profiles are currently defined. Hardware profiles
// can be created from the System applet of the control panel.
//
#define BL_LKG_MENU_TRAILER_NO_PROFILES  0x00002355L

//
// MessageId: BL_SWITCH_LKG_TRAILER
//
// MessageText:
//
// To switch to the Last Known Good configuration, press 'L'.
// To Exit this menu and restart your computer, press F3.
//
#define BL_SWITCH_LKG_TRAILER            0x00002356L

//
// MessageId: BL_SWITCH_DEFAULT_TRAILER
//
// MessageText:
//
// To switch to the default configuration, press 'D'.
// To Exit this menu and restart your computer, press F3.
//
#define BL_SWITCH_DEFAULT_TRAILER        0x00002357L

//
// MessageId: BL_LKG_SELECT_MNEMONIC
//
// MessageText:
//
// L
//
#define BL_LKG_SELECT_MNEMONIC           0x00002358L

//
// MessageId: BL_DEFAULT_SELECT_MNEMONIC
//
// MessageText:
//
// D
//
#define BL_DEFAULT_SELECT_MNEMONIC       0x00002359L

//
// MessageId: BL_LKG_TIMEOUT
//
// MessageText:
//
// Seconds until highlighted choice will be started automatically: %d
//
#define BL_LKG_TIMEOUT                   0x0000235AL

//
// MessageId: BL_LKG_MENU_PROMPT
//
// MessageText:
//
// 
// Press spacebar NOW to invoke Hardware Profile/Last Known Good menu
//
#define BL_LKG_MENU_PROMPT               0x0000235BL

//
// MessageId: BL_BOOT_DEFAULT_PROMPT
//
// MessageText:
//
// Boot default hardware configuration
//
#define BL_BOOT_DEFAULT_PROMPT           0x0000235CL

//
// MessageId: BL_SYSTEM_RESTARTING_SPACE_TO_INTERRUPT
//
// MessageText:
//
// The system is being restarted from its previous location.
// Press the spacebar to interrupt.
//
#define BL_SYSTEM_RESTARTING_SPACE_TO_INTERRUPT 0x0000235DL

//
// MessageId: BL_SYSTEM_RESTART_FAILED
//
// MessageText:
//
// The system could not be restarted from its previous location
//
#define BL_SYSTEM_RESTART_FAILED         0x0000235EL

//
// MessageId: BL_SYSTEM_RESTART_MEMORY_ERROR
//
// MessageText:
//
// due to no memory.
//
#define BL_SYSTEM_RESTART_MEMORY_ERROR   0x0000235FL

//
// MessageId: BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT
//
// MessageText:
//
// because the restoration image is corrupt.
//
#define BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT 0x00002360L

//
// MessageId: BL_SYSTEM_RESTART_IMAGE_FILE_CONFIG_ERROR
//
// MessageText:
//
// because the restoration image is not compatible with the current
// configuration.
//
#define BL_SYSTEM_RESTART_IMAGE_FILE_CONFIG_ERROR 0x00002361L

//
// MessageId: BL_SYSTEM_RESTART_INTERNAL_ERROR
//
// MessageText:
//
// due to an internal error.
//
#define BL_SYSTEM_RESTART_INTERNAL_ERROR 0x00002362L

//
// MessageId: BL_SYSTEM_RESTART_INTERNAL_ERROR2
//
// MessageText:
//
// due to an internal error.
//
#define BL_SYSTEM_RESTART_INTERNAL_ERROR2 0x00002363L

//
// MessageId: BL_SYSTEM_RESTART_READ_FAILURE
//
// MessageText:
//
// due to a read failure.
//
#define BL_SYSTEM_RESTART_READ_FAILURE   0x00002364L

//
// MessageId: BL_SYSTEM_RESTART_PAUSED
//
// MessageText:
//
// System restart has been paused:
//
#define BL_SYSTEM_RESTART_PAUSED         0x00002365L

//
// MessageId: BL_DELETE_RESTORATION_DATA_AND_DISPLAY_BOOT_MENU
//
// MessageText:
//
// Delete restoration data and proceed to system boot menu
//
#define BL_DELETE_RESTORATION_DATA_AND_DISPLAY_BOOT_MENU 0x00002366L

//
// MessageId: BL_CONTINUE_WITH_RESTART
//
// MessageText:
//
// Continue with system restart
//
#define BL_CONTINUE_WITH_RESTART         0x00002367L

//
// MessageId: BL_RESTART_FAILED_TRY_AGAIN
//
// MessageText:
//
// The last attempt to restart the system from its previous location
// failed.  Attempt to restart again?
//
#define BL_RESTART_FAILED_TRY_AGAIN      0x00002368L

//
// MessageId: BL_CONTINUE_DBG_BREAK_ON_WAKE
//
// MessageText:
//
// Continue with debug breakpoint on system wake
//
#define BL_CONTINUE_DBG_BREAK_ON_WAKE    0x00002369L

//
// MessageId: BL_KERNEL_ERROR
//
// MessageText:
//
// Windows could not start because the specified kernel does 
// not exist or is not compatible with this processor.
//
#define BL_KERNEL_ERROR                  0x0000236AL

//
// MessageId: BL_STARTING_WINDOWS
//
// MessageText:
//
// Starting Windows...
//
#define BL_STARTING_WINDOWS              0x0000236BL

//
// MessageId: BL_RESUMING_WINDOWS
//
// MessageText:
//
// Resuming Windows...
//
#define BL_RESUMING_WINDOWS              0x0000236CL

//
// MessageId: BL_READING_NVRAM_FAILED
//
// MessageText:
//
// Windows could not start because there was an error reading
// the boot settings from NVRAM.
// 
// Please check your firmware settings.  You may need to restore your
// NVRAM settings from a backup.
//
#define BL_READING_NVRAM_FAILED          0x0000236DL

//
// MessageId: BL_ENABLED_KD_TITLE
//
// MessageText:
//
//  [debugger enabled]
//
#define BL_ENABLED_KD_TITLE              0x00002711L

//
// MessageId: BL_DEFAULT_TITLE
//
// MessageText:
//
// Windows (default)
//
#define BL_DEFAULT_TITLE                 0x00002712L

//
// MessageId: BL_MISSING_BOOT_INI
//
// MessageText:
//
// NTLDR: BOOT.INI file not found.
//
#define BL_MISSING_BOOT_INI              0x00002713L

//
// MessageId: BL_MISSING_OS_SECTION
//
// MessageText:
//
// NTLDR: no [operating systems] section in boot.txt.
//
#define BL_MISSING_OS_SECTION            0x00002714L

//
// MessageId: BL_BOOTING_DEFAULT
//
// MessageText:
//
// Booting default kernel from %s.
//
#define BL_BOOTING_DEFAULT               0x00002715L

//
// MessageId: BL_SELECT_OS
//
// MessageText:
//
// Please select the operating system to start:
//
#define BL_SELECT_OS                     0x00002716L

//
// MessageId: BL_MOVE_HIGHLIGHT
//
// MessageText:
//
// 
// 
// Use the up and down arrow keys to move the highlight to your choice.
// Press ENTER to choose.
//
#define BL_MOVE_HIGHLIGHT                0x00002717L

//
// MessageId: BL_TIMEOUT_COUNTDOWN
//
// MessageText:
//
// Seconds until highlighted choice will be started automatically:
//
#define BL_TIMEOUT_COUNTDOWN             0x00002718L

//
// MessageId: BL_INVALID_BOOT_INI
//
// MessageText:
//
// Invalid BOOT.INI file
// Booting from %s
//
#define BL_INVALID_BOOT_INI              0x00002719L

//
// MessageId: BL_REBOOT_IO_ERROR
//
// MessageText:
//
// I/O Error accessing boot sector file
// %s\BOOTSECT.DOS
//
#define BL_REBOOT_IO_ERROR               0x0000271AL

//
// MessageId: BL_DRIVE_ERROR
//
// MessageText:
//
// NTLDR: Couldn't open drive %s
//
#define BL_DRIVE_ERROR                   0x0000271BL

//
// MessageId: BL_READ_ERROR
//
// MessageText:
//
// NTLDR: Fatal Error %d reading BOOT.INI
//
#define BL_READ_ERROR                    0x0000271CL

//
// MessageId: BL_NTDETECT_MSG
//
// MessageText:
//
// 
// NTDETECT V5.0 Checking Hardware ...
// 
//
#define BL_NTDETECT_MSG                  0x0000271DL

//
// MessageId: BL_NTDETECT_FAILURE
//
// MessageText:
//
// NTDETECT failed
//
#define BL_NTDETECT_FAILURE              0x0000271EL

//
// MessageId: BL_TRACE_OS_SELECTION_INFO
//
// MessageText:
//
// Current Selection:
//   Title..: %s
//   Path...: %s
//   Options: %s
//
#define BL_TRACE_OS_SELECTION_INFO       0x0000271FL

//
// MessageId: BL_PRMOPT_ENTER_NEW_LOAD_OPTIONS
//
// MessageText:
//
// Enter new load options:
//
#define BL_PRMOPT_ENTER_NEW_LOAD_OPTIONS 0x00002720L

//
// MessageId: BL_ENABLED_EMS_TITLE
//
// MessageText:
//
//  [EMS enabled]
//
#define BL_ENABLED_EMS_TITLE             0x00002721L

//
// MessageId: BL_INVALID_BOOT_INI2
//
// MessageText:
//
// Invalid BOOT.INI file
//
#define BL_INVALID_BOOT_INI2             0x00002722L

//
// MessageId: BL_ADVANCED_BOOT_MENU_PROMPT
//
// MessageText:
//
// Windows Advanced Options Menu
// Please select an option:
//
#define BL_ADVANCED_BOOT_MENU_PROMPT     0x00002AF9L

//
// MessageId: BL_ADVANCED_BOOT_SAFE_MODE
//
// MessageText:
//
// Safe Mode
//
#define BL_ADVANCED_BOOT_SAFE_MODE       0x00002AFAL

//
// MessageId: BL_ADVANCED_BOOT_SAFE_MODE_WITH_NETWORK
//
// MessageText:
//
// Safe Mode with Networking
//
#define BL_ADVANCED_BOOT_SAFE_MODE_WITH_NETWORK 0x00002AFBL

//
// MessageId: BL_ADVANCED_BOOT_STEP_BY_STEP_CONFIRMATION_MODE
//
// MessageText:
//
// Step-by-Step Confirmation Mode
//
#define BL_ADVANCED_BOOT_STEP_BY_STEP_CONFIRMATION_MODE 0x00002AFCL

//
// MessageId: BL_ADVANCED_BOOT_SAFE_MODE_WITH_CMD
//
// MessageText:
//
// Safe Mode with Command Prompt
//
#define BL_ADVANCED_BOOT_SAFE_MODE_WITH_CMD 0x00002AFDL

//
// MessageId: BL_ADVANCED_BOOT_VGA_MODE
//
// MessageText:
//
// VGA Mode
//
#define BL_ADVANCED_BOOT_VGA_MODE        0x00002AFEL

//
// MessageId: BL_ADVANCED_BOOT_DIRECTORY_SERVICES_RESTORE_MODE
//
// MessageText:
//
// Directory Services Restore Mode (Windows domain controllers only)
//
#define BL_ADVANCED_BOOT_DIRECTORY_SERVICES_RESTORE_MODE 0x00002AFFL

//
// MessageId: BL_ADVANCED_BOOT_USE_LAST_KNOWN_GOOD
//
// MessageText:
//
// Last Known Good Configuration (your most recent settings that worked)
//
#define BL_ADVANCED_BOOT_USE_LAST_KNOWN_GOOD 0x00002B00L

//
// MessageId: BL_ADVANCED_BOOT_DEBUG_MODE
//
// MessageText:
//
// Debugging Mode
//
#define BL_ADVANCED_BOOT_DEBUG_MODE      0x00002B01L

//
// MessageId: BL_ADVANCED_BOOT_DISABLE_CRASH_AUTO_REBOOT
//
// MessageText:
//
// Disable automatic restart on system failure
//
#define BL_ADVANCED_BOOT_DISABLE_CRASH_AUTO_REBOOT 0x00002B02L

//
// MessageId: BL_ADVANCED_BOOT_ENABLE_BOOT_LOGGING
//
// MessageText:
//
// Enable Boot Logging
//
#define BL_ADVANCED_BOOT_ENABLE_BOOT_LOGGING 0x00002B05L

//
// MessageId: BL_GOTO_ADVANCED_BOOT_F8
//
// MessageText:
//
// For troubleshooting and advanced startup options for Windows, press F8.
//
#define BL_GOTO_ADVANCED_BOOT_F8         0x00002B06L

//
// MessageId: BL_ADVANCED_BOOT_BASE_VIDEO
//
// MessageText:
//
// Enable VGA Mode
//
#define BL_ADVANCED_BOOT_BASE_VIDEO      0x00002B07L

//
// MessageId: BL_PRESS_ESC_TO_DISABLE_SAFE_BOOT
//
// MessageText:
//
// 
// Press ESCAPE to disable safeboot and boot normally.
//
#define BL_PRESS_ESC_TO_DISABLE_SAFE_BOOT 0x00002B08L

//
// MessageId: BL_ADVANCED_BOOT_START_WINDOWS_NORMALLY
//
// MessageText:
//
// Start Windows Normally
//
#define BL_ADVANCED_BOOT_START_WINDOWS_NORMALLY 0x00002B09L

//
// MessageId: BL_ADVANCED_BOOT_RETURN_TO_OS_SELECT
//
// MessageText:
//
// Return to OS Choices Menu
//
#define BL_ADVANCED_BOOT_RETURN_TO_OS_SELECT 0x00002B0AL

//
// MessageId: BL_ADVANCED_BOOT_REBOOT_COMPUTER
//
// MessageText:
//
// Reboot
//
#define BL_ADVANCED_BOOT_REBOOT_COMPUTER 0x00002B0BL

//
// MessageId: BL_ADVANCED_BOOT_START_FAILED
//
// MessageText:
//
// We apologize for the inconvenience, but Windows did not start successfully.  A 
// recent hardware or software change might have caused this.
// 
// If your computer stopped responding, restarted unexpectedly, or was 
// automatically shut down to protect your files and folders, choose Last Known 
// Good Configuration to revert to the most recent settings that worked.
// 
// If a previous startup attempt was interrupted due to a power failure or because 
// the Power or Reset button was pressed, or if you aren't sure what caused the 
// problem, choose Start Windows Normally.
//
#define BL_ADVANCED_BOOT_START_FAILED    0x00002B0CL

//
// MessageId: BL_ADVANCED_BOOT_SHUTDOWN_FAILED
//
// MessageText:
//
// Windows was not shutdown successfully.  If this was due to the system not 
// responding, or if the system shutdown to protect data, you might be able to 
// recover by choosing one of the Safe Mode configurations from the menu below:
//
#define BL_ADVANCED_BOOT_SHUTDOWN_FAILED 0x00002B0DL

//
// MessageId: BL_ADVANCED_BOOT_TIMEOUT
//
// MessageText:
//
// Seconds until Windows starts:
//
#define BL_ADVANCED_BOOT_TIMEOUT         0x00002B0EL

//
// MessageId: BL_PROGRESS_BAR_FRONT_CHAR
//
// MessageText:
//
// Û
//
#define BL_PROGRESS_BAR_FRONT_CHAR       0x00002CF9L

//
// MessageId: BL_PROGRESS_BAR_BACK_CHAR
//
// MessageText:
//
// Þ
//
#define BL_PROGRESS_BAR_BACK_CHAR        0x00002CFAL

//
// MessageId: BL_RAMDISK_BOOT_FAILED
//
// MessageText:
//
// Windows could not start due to an error while booting from a RAMDISK.
//
#define BL_RAMDISK_BOOT_FAILED           0x00003A98L

//
// MessageId: BL_RAMDISK_OPEN_FAILED
//
// MessageText:
//
// Windows failed to open the RAMDISK image.
//
#define BL_RAMDISK_OPEN_FAILED           0x00003A9BL

//
// MessageId: BL_RAMDISK_LOADING
//
// MessageText:
//
// Loading RAMDISK image...
//
#define BL_RAMDISK_LOADING               0x00003AA2L

