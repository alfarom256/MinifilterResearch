#include "DBUtilMem.h"

BOOL DBUtilMem::VirtRead(PVOID lpAddr)
{
    return 0;
}

BOOL DBUtilMem::VirtWrite(PVOID lpAddr, DWORD64 qwValue)
{
    BOOL bRes = FALSE;
    PIOCTL_STRUCT lpData = new IOCTL_STRUCT();

   /* bRes = DeviceIoControl(
        hDevice, 

    );*/

    delete lpData;
    return bRes;
}

BOOL DBUtilMem::Connect()
{
    hDevice = CreateFileA(
        R"(\\.\DBUtil_2_3Z)", 
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, 
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );    
    return hDevice != NULL;
}
