#pragma once

#define FILE_DEVICE_VADPURGE        0x00008013
#define IOCTL_VADPURGE_PURGE        CTL_CODE(FILE_DEVICE_VADPURGE, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_VADPURGE_ENABLECHANGE CTL_CODE(FILE_DEVICE_VADPURGE, 0x810, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef struct _VmRegionEntry
{
    unsigned long long startAddr;
    unsigned long long size;

} VmRegionEntry, PVmRegionEntry;

typedef struct _PURGE_DATA
{
    int procID;
    int numOfEntries;
    VmRegionEntry entries[1];

} PURGE_DATA, *PPURGE_DATA;