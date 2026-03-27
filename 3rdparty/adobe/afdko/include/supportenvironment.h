#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

/*
* NOTE: This is a custom shim to workaround the original supportenvironment.h
* which is seriously flawed. See https://github.com/podofo/podofo/pull/259#issuecomment-2910248942
*/

#define os_ps         1 
#define os_bsd        2 
#define os_sysv       4 
#define os_aux        5 
#define os_xenix      6 
#define os_vms        7 
#define os_domain     9 
#define os_mpw       10 
#define os_vm370     11 
#define os_mvs370    12 
#define os_os2       13 
#define os_ultrix    14 
#define os_aix       15 
#define os_pharlap   16 
#define os_mach      17 
#define os_msdos     18 
#define os_thinkc    19 
#define os_windows3  20 
#define os_os2_32bit 21 
#define os_macgcc    22 
#define os_ncd       23 
#define os_irix      25 
#define os_hpux      26 
#define os_irixV     27 
#define os_osf1      28 
#define os_windowsNT 29 
#define os_win32     29 
#define os_windows95 30 
#define os_win64     31 

#define os_osx       98 
#define os_linux     99 

#define os_mac os_thinkc

#if defined(_WIN32) && !defined(_WIN64)
#define OS os_win32
#elif defined(_WIN32) && defined(_WIN64)
#define OS os_win64
#elif defined(__APPLE__)
#define OS os_osx
#elif defined(__linux__)
#define OS os_linux
#else
#define OS os_unknown
#endif

#if INTPTR_MAX == INT64_MAX
#define ARCH_64BIT 1
#else
#define ARCH_64BIT 0
#endif

#define ANSI_C 1

#ifndef DEVELOP
#define DEVELOP 1
#endif
#ifndef EXPORT
#define EXPORT 2
#endif
#define STAGE EXPORT

#ifndef mips
#define volatile
#endif

#define IEEEFLOAT 1
#define IEEESOFT 0

#endif /* ENVIRONMENT_H */
