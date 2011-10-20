  
     ___
    |. .|
    | _ |
    |   |
   /     \   __        __   _         _____               _
  //|   |\\  \ \      / /__| |__  _ _|_   _| __ _   _ ___| |_
  \\|   |//   \ \ /\ / / _ \ '_ \| '_ \| || '__| | | / __| __|
  (_)   (_)    \ V  V /  __/ | | | | | | || |  | |_| \__ \ |_
    |___|       \_/\_/ \___|_| |_|_| |_|_||_|   \__,_|___/\__|
    | | |
    | | |                http://www.wehnus.com
    |_|_|
   (__|__)
         
Overview
========

WehnTrust is a Host-based Intrusion Prevention System (HIPS) for Windows 2000,
XP, and Server 2003. It includes support for exploit mitigations that are
designed to make exploitation more difficult by preventing the use of specific
exploitation techniques and by making exploitation unreliable.

How it works
============

WehnTrust randomizes the base addresses of memory allocations to make it more
difficult to exploit software vulnerabilities such as buffer overflows. This
technique is commonly known as Address Space Layout Randomization (ASLR) and
was originally conceived by the PaX team. Microsoft has recently incorporated
support for ASLR into Windows Vista and Windows Server 2008. In addition to
ASLR, WehnTrust generically mitigates the SEH overwrite exploitation technique
by dynamically validating a thread's exception handler chain prior to allowing
exceptions to be dispatched.

Recommendations
===============

Using WehnTrust in combination with hardware-enforced DEP (non-executable
pages) as included with Windows XP SP2 and Windows Server 2003 provides the
greatest level of security. Non-executable pages help to counter some of the
inherent weaknesses of ASLR.

Features
========

The following features are included:

  * Address Space Layout Randomization
    o Randomized image file mappings (relocations required)
    o Randomized memory allocations (e.g. VirtualAlloc)
    o Randomized PEB/TEB
    o Basic brute force detection and prevention
  * SEH Overwrite Prevention
  * Format string vulnerability prevention
  * Logging and notification of exploitation attempts
    o Balloon tip nofication
    o Native windows event logging
  * Application and image file exemptions

Limitations
===========

WehnTrust isn't perfect.  It is not a panacea and cannot prevent all
exploitation attempts.  Here are some of the key limitations to be aware of when
using this software:

  * ASLR weaknesses
    - Address Space Layout Randomization has a number of weaknesses including
      partial overwrites, non-relocatable executables, information disclosure,
      and brute forcability. 
  
  * Local security
    - WehnTrust's implementation of ASLR does not provide any security
      enhancements as it relates to local privilege escalations.  The SEH
      overwrite and format string prevention techniques, however, can provide
      some level of protection.   It is also possible that WehnTrust may
      introduce local privilege escalation vulnerabilities.  While the software
      has been security reviewed, it is entirely possible that something has
      been missed.

Components
==========

WehnTrust is composed of the following major components:

  * baserand.sys
    - Required: This boot-start device driver provides kernel-mode support for
      system-wide ASLR and for per-process SEH overwrite prevention.  This
      component is the only component required to support ASLR.
  * NRER.dll
    - Optional: This DLL is injected into the context of every process by the
      baserand device driver to provide user-mode support for SEH overwrite
      prevention, format string vulnerability prevention, and exploitation
      attempt logging.
  * WehnServ.exe
    - Optional: This binary is a LocalSystem Windows service that is the arbiter
      for exploitation attempt notifications.  It also tracks statistics
      relating to brute force attempts.
  * WehnTrust.exe
    - Optional: This executable provides the user interface which allows users
      to monitor statistics, manage exemptions, and otherwise interact with the
      software as a whole.

Future work
===========

There are a number of improvements that can be made to WehnTrust in the future.
In no specific order, these improvements include:

  * Support for non-relocatable executable randomization
    - Non-relocatable executables (executables without relocations) are
      currently an issue that results in a predictable address region that could
      be used by an attacker.  Randomizing these regions would help to improve
      security be eliminating a known address region.

  * Support for multiple image sets
    - Multiple image sets would allow administrative and non-administrative
      users to have different base addresses for mapped images.  This could help
      to improve local security.

  * Support for software non-executable pages
    - Integrating support for PaX-style non-executable pages for use on
      computers and operating systems (such as Windows 2000) that do not support
      hardware non-executable pages would allow greater levels of security.

  * Support for Windows x64

Status
======

WehnTrust is no longer being actively developed by Wehnus LLC.  Its code has
been made available to spread knowledge and to allow third parties to enhance
the software.

License
=======

WehnTrust is licensed under the WehnTrust Software License 1.0.
