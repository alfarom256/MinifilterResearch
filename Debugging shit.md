```
3: kd> !object @rcx
Object: ffff828a99cf4660  Type: (ffff828a8f9f9f00) FilterConnectionPort
    ObjectHeader: ffff828a99cf4630 (new version)
    HandleCount: 0  PointerCount: 0
3: kd> dt FLTMGR!_FLT_FILTER @rcx
   +0x000 Base             : _FLT_OBJECT
   +0x030 Frame            : 0xffff828a`8f8d6020 _FLTP_FRAME
   +0x038 Name             : _UNICODE_STRING "DemoMinifilter"
   +0x048 DefaultAltitude  : _UNICODE_STRING "25000"
   +0x058 Flags            : 0 (No matching name)
   +0x060 DriverObject     : 0xffff828a`9fded2e0 _DRIVER_OBJECT
   +0x068 InstanceList     : _FLT_RESOURCE_LIST_HEAD
   +0x0e8 VerifierExtension : (null) 
   +0x0f0 VerifiedFiltersLink : _LIST_ENTRY [ 0x00000000`00000000 - 0x00000000`00000000 ]
   +0x100 FilterUnload     : 0xfffff805`5b251080     long  DemoMinifilter!FsFilterUnload+0
   +0x108 InstanceSetup    : (null) 
   +0x110 InstanceQueryTeardown : (null) 
   +0x118 InstanceTeardownStart : (null) 
   +0x120 InstanceTeardownComplete : (null) 
   +0x128 SupportedContextsListHead : (null) 
   +0x130 SupportedContexts : [7] (null) 
   +0x168 PreVolumeMount   : (null) 
   +0x170 PostVolumeMount  : (null) 
   +0x178 GenerateFileName : (null) 
   +0x180 NormalizeNameComponent : (null) 
   +0x188 NormalizeNameComponentEx : (null) 
   +0x190 NormalizeContextCleanup : (null) 
   +0x198 KtmNotification  : (null) 
   +0x1a0 SectionNotification : (null) 
   +0x1a8 Operations       : 0xffff828a`99cf4910 _FLT_OPERATION_REGISTRATION
   +0x1b0 OldDriverUnload  : (null) 
   +0x1b8 ActiveOpens      : _FLT_MUTEX_LIST_HEAD
   +0x208 ConnectionList   : _FLT_MUTEX_LIST_HEAD
   +0x258 PortList         : _FLT_MUTEX_LIST_HEAD
   +0x2a8 PortLock         : _EX_PUSH_LOCK
3: kd> k
 # Child-SP          RetAddr               Call Site
00 ffffee05`662ae7f8 fffff80e`47b1aa02     FLTMGR!FltpLinkFilterIntoFrame
01 ffffee05`662ae800 fffff805`5b2510ce     FLTMGR!FltRegisterFilter+0x4a2
02 ffffee05`662ae8e0 fffff805`5b255020     DemoMinifilter!DriverEntry+0x2e [C:\Users\Michael\Desktop\Research\MinifilterStuff\drivers\DemoMinifilter\DemoMinifilter\DemoMinifilter\DemoMinifilter.c @ 48] 
03 ffffee05`662ae920 fffff805`57a774dd     DemoMinifilter!GsDriverEntry+0x20 [minkernel\tools\gs_support\kmodefastfail\gs_driverentry.c @ 47] 
04 ffffee05`662ae950 fffff805`57a7580a     nt!IopLoadDriver+0x4bd
05 ffffee05`662aeb30 fffff805`574c61ea     nt!IopLoadUnloadDriver+0x4a
06 ffffee05`662aeb70 fffff805`57498bc5     nt!ExpWorkerThread+0x16a
07 ffffee05`662aec10 fffff805`575cca3c     nt!PspSystemThreadStartup+0x55
08 ffffee05`662aec60 00000000`00000000     nt!KiStartSystemThread+0x1c


//
//
// The FLT_FILTER object is -0x10 from the LIST_ENTRY
//
//
//




3: kd> dt FLTMGR!_FLT_FILTER 0xffff828a905819c0-0x10
   +0x000 Base             : _FLT_OBJECT
   +0x030 Frame            : 0xffff828a`8f8d6020 _FLTP_FRAME
   +0x038 Name             : _UNICODE_STRING "WdFilter"
   +0x048 DefaultAltitude  : _UNICODE_STRING "328010"
   +0x058 Flags            : 0x32 (No matching name)
   +0x060 DriverObject     : 0xffff828a`9057edb0 _DRIVER_OBJECT
   +0x068 InstanceList     : _FLT_RESOURCE_LIST_HEAD
   +0x0e8 VerifierExtension : (null) 
   +0x0f0 VerifiedFiltersLink : _LIST_ENTRY [ 0x00000000`00000000 - 0x00000000`00000000 ]
   +0x100 FilterUnload     : 0xfffff80e`48682c10     long  +0
   +0x108 InstanceSetup    : 0xfffff80e`48682f30     long  +0
   +0x110 InstanceQueryTeardown : 0xfffff80e`48683120     long  +0
   +0x118 InstanceTeardownStart : (null) 
   +0x120 InstanceTeardownComplete : 0xfffff80e`48683180     void  +0
   +0x128 SupportedContextsListHead : 0xffff828a`90584060 _ALLOCATE_CONTEXT_HEADER
   +0x130 SupportedContexts : [7] (null) 
   +0x168 PreVolumeMount   : (null) 
   +0x170 PostVolumeMount  : 0xfffff80e`486546b0     _FLT_POSTOP_CALLBACK_STATUS  +0
   +0x178 GenerateFileName : (null) 
   +0x180 NormalizeNameComponent : (null) 
   +0x188 NormalizeNameComponentEx : (null) 
   +0x190 NormalizeContextCleanup : (null) 
   +0x198 KtmNotification  : 0xfffff80e`48689af0     long  +0
   +0x1a0 SectionNotification : (null) 
   +0x1a8 Operations       : 0xffff828a`90581c60 _FLT_OPERATION_REGISTRATION
   +0x1b0 OldDriverUnload  : (null) 
   +0x1b8 ActiveOpens      : _FLT_MUTEX_LIST_HEAD
   +0x208 ConnectionList   : _FLT_MUTEX_LIST_HEAD
   +0x258 PortList         : _FLT_MUTEX_LIST_HEAD
   +0x2a8 PortLock         : _EX_PUSH_LOCK
```

Read the callbacks array until we get the IRP_MJ_END

```
0: kd> dx -id 0,0,ffff9d08f727d380 -r1 ((FLTMGR!_FLT_OPERATION_REGISTRATION *)0xffff9d08fb190b50)
((FLTMGR!_FLT_OPERATION_REGISTRATION *)0xffff9d08fb190b50)                 : 0xffff9d08fb190b50 [Type: _FLT_OPERATION_REGISTRATION *]
    [+0x000] MajorFunction    : 0x0 [Type: unsigned char]
    [+0x004] Flags            : 0x0 [Type: unsigned long]
    [+0x008] PreOperation     : 0xfffff801490912b4 : amsdk+0x112b4 [Type: _FLT_PREOP_CALLBACK_STATUS (__cdecl*)(_FLT_CALLBACK_DATA *,_FLT_RELATED_OBJECTS *,void * *)]
    [+0x010] PostOperation    : 0xfffff801490912fc : amsdk+0x112fc [Type: _FLT_POSTOP_CALLBACK_STATUS (__cdecl*)(_FLT_CALLBACK_DATA *,_FLT_RELATED_OBJECTS *,void *,unsigned long)]
    [+0x018] Reserved1        : 0x0 [Type: void *]
0: kd> dq 0xffff9d08fb190b50
ffff9d08`fb190b50  00000000`00000000 fffff801`490912b4
ffff9d08`fb190b60  fffff801`490912fc 00000000`00000000
ffff9d08`fb190b70  00000000`00000004 fffff801`49091470
ffff9d08`fb190b80  00000000`00000000 00000000`00000000
ffff9d08`fb190b90  00000000`00000080 00000000`00000000
ffff9d08`fb190ba0  00000000`00000000 00000000`00000000
ffff9d08`fb190bb0  00640073`006d0061 00000000`0000006b
ffff9d08`fb190bc0  ff495121`8f162408 00000000`0000000e
```

Once that's done, to patch the callbacks you can just do something cheap and easy:
* Find `xor eax, eax; ret;`
* Set post-callback first
	* `FLT_POSTOP_FINISHED_PROCESSING = 0`
* Set pre-callback second
	* `FLT_PREOP_SUCCESS_WITH_CALLBACK = 0`

Setting both to the same value would cause the PreOp to return a status indicating the PostOp should be invoked. The PostOp would then return 0, indicating success.

Changing the major function to IRP_MJ_OPERATION_END did not work.
Callstack to PreCreateCallback:
```
2: kd> k
 # Child-SP          RetAddr               Call Site
00 fffff08c`13333100 fffff802`7c04555c     DemoMinifilter!PreCreateCallback+0x75 [C:\Users\Michael\Desktop\Research\MinifilterStuff\drivers\DemoMinifilter\DemoMinifilter\DemoMinifilter\Callbacks.c @ 15] 
01 fffff08c`13333150 fffff802`7c0450bc     FLTMGR!FltpPerformPreCallbacks+0x2fc
02 fffff08c`13333260 fffff802`7c07d545     FLTMGR!FltpPassThroughInternal+0x8c
03 fffff08c`13333290 fffff802`796f5109     FLTMGR!FltpCreate+0x2e5
04 fffff08c`13333340 fffff802`796ee0c4     nt!IofCallDriver+0x59
05 fffff08c`13333380 fffff802`79c774a7     nt!IoCallDriverWithTracing+0x34
06 fffff08c`133333d0 fffff802`79c7f6f9     nt!IopParseDevice+0x11e7
07 fffff08c`13333540 fffff802`79c7e2ef     nt!ObpLookupObjectName+0x719
08 fffff08c`13333710 fffff802`79d42e3d     nt!ObOpenObjectByNameEx+0x1df
09 fffff08c`13333850 fffff802`79885608     nt!NtQueryAttributesFile+0x1cd
0a fffff08c`13333b00 00007ff9`9cdcf924     nt!KiSystemServiceCopyEnd+0x28
0b 00000063`658f9d58 00007ff9`990c0635     ntdll!NtQueryAttributesFile+0x14
```