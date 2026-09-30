/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/*
 * This library is used for calculating how much memory is available/used.
 * Certain platforms offer physical memory statistics, so we wrap around
 * those functions. For platforms where we can't retrieve this information,
 * we estimate sizes based on a few key variables and symbols. These estimated
 * values should be tolerable for general use.
 *
 * 2026-06-24 - Removing legacy console and static device support. This
 * includes removing fixed physical memory branches for each respective port.
 * Supported platforms use host platform APIs or the generic estimate path.
 * Future cleanup can modernize these functions and fully remove the system
 * RAM fallback tree.
 *
 * Linux RAM info also looks suspicious and needs further investigation. 
 */

/////////////////////////////////////////////////////////////////////////////
// Libraries

#ifdef WIN
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <psapi.h>
#elif DARWIN
#include <sys/sysctl.h>
#include <mach/task.h>
#include <mach/mach.h>
#include <mach/mach_init.h>
#elif LINUX
#include <sys/sysinfo.h>
#include <unistd.h>
#include <features.h>
#endif

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "globals.h"
#include "utils.h"
#include "ram.h"

/////////////////////////////////////////////////////////////////////////////
// Globals

static u64 systemRam = 0x00000000;

#if !(defined(WIN) || defined(LINUX) || defined(DARWIN))
static unsigned long elfOffset = 0x00000000;
static unsigned long stackSize = 0x00000000;
#endif

/////////////////////////////////////////////////////////////////////////////
// Symbols

#if !(defined(WIN) || defined(LINUX) || defined(DARWIN))
#if (__GNUC__ > 3)
extern unsigned long _end;
extern unsigned long _start;
#else
extern unsigned long end;
extern unsigned long start;
#define _end end
#define _start start
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
//  Functions

u64 getFreeRam(int byte_size)
{
#if WIN
    MEMORYSTATUSEX stat;
    memset(&stat, 0, sizeof(MEMORYSTATUSEX));
    stat.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&stat);
    return stat.ullAvailPhys / byte_size;
#elif DARWIN
    vm_size_t size;
    unsigned int count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vms;
    mach_port_t hostPort = mach_host_self();
    if(host_page_size(hostPort, &size) != KERN_SUCCESS)
    {
        return 0;
    }
    if(host_statistics(hostPort, HOST_VM_INFO, (host_info_t)&vms, &count) != KERN_SUCCESS)
    {
        return 0;
    }
    return (u64)(((vms.inactive_count + vms.free_count) * size) / byte_size);
#elif LINUX
    struct sysinfo info;
    sysinfo(&info);
    return ((u64)info.freeram) * info.mem_unit;
#elif SYMBIAN
    return GetFreeAmount();
#else
    struct mallinfo mi = mallinfo();
#ifdef _INCLUDE_MALLOC_H_
    // Standard ANSI C Implementation
    return (systemRam - (mi.arena + stackSize)) / byte_size;
#else
    return (systemRam - (mi.usmblks + stackSize)) / byte_size;
#endif
#endif
}

void setSystemRam()
{
#if WIN
    MEMORYSTATUSEX stat;
    memset(&stat, 0, sizeof(MEMORYSTATUSEX));
    stat.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&stat);
    systemRam = stat.ullTotalPhys;
#elif DARWIN
    u64 mem;
    size_t len = sizeof(mem);
    sysctlbyname("hw.memsize", &mem, &len, NULL, 0);
    systemRam = mem;
#elif LINUX
    struct sysinfo info;
    sysinfo(&info);
    systemRam = ((u64)info.totalram) * info.mem_unit;
#else
    elfOffset = 0x00000000;
    stackSize = 0x00000000;
    systemRam = getFreeRam(BYTES);
#endif
#if !(defined(WIN) || defined(LINUX) || defined(DARWIN) || defined(SYMBIAN))
    stackSize = (int)&_end - (int)&_start + ((int)&_start - elfOffset);
#endif
    getRamStatus(BYTES);
}

u64 getSystemRam(int byte_size)
{
    return systemRam / byte_size;
}

u64 getUsedRam(int byte_size)
{
#ifdef WIN
    PROCESS_MEMORY_COUNTERS pmc;
    memset(&pmc, 0, sizeof(PROCESS_MEMORY_COUNTERS));
    pmc.cb = sizeof(pmc);
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, pmc.cb);
    return pmc.WorkingSetSize / byte_size;
#elif DARWIN
    struct task_basic_info  info;
    kern_return_t           rval = 0;
    mach_port_t             task = mach_task_self();
    mach_msg_type_number_t  tcnt = TASK_BASIC_INFO_COUNT;
    task_info_t             tptr = (task_info_t) &info;
    memset(&info, 0, sizeof(info));
    rval = task_info(task, TASK_BASIC_INFO, tptr, &tcnt);
    if (!(rval == KERN_SUCCESS))
    {
        return 0;
    }
    return info.resident_size / byte_size;
#elif LINUX
    unsigned long vm = 0;
    FILE *file = fopen("/proc/self/statm", "r");
    if (file == NULL)
    {
        return 0;
    }
    if (fscanf (file, "%lu", &vm) <= 0)
    {
        return 0;
    }
    fclose (file);
    return (vm * getpagesize() / 8) / byte_size;
#else
    return (systemRam - getFreeRam(BYTES)) / byte_size;
#endif
}

void getRamStatus(int byte_size)
{
  u64 system_ram = getSystemRam(byte_size);
  u64 free_ram = getFreeRam(byte_size);
  u64 used_ram = getUsedRam(byte_size);

  printf("Total Ram: %11"PRIu64" Bytes ( %5"PRIu64" MB )\n Free Ram: %11"PRIu64" Bytes ( %5"PRIu64" MB )\n Used Ram: %11"PRIu64" Bytes ( %5"PRIu64" MB )\n\n",
           system_ram, system_ram >> 20,
           free_ram, free_ram >> 20,
           used_ram, used_ram >> 20);
}

