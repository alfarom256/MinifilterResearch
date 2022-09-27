```
kd> !fltkd.frames

Frame List: fffff8050fcbb780 
   FLTP_FRAME: ffffcb0e0b3a5020 "Frame 0" "0 to 409800"
      FLT_FILTER: ffffcb0e0b386010 "bindflt" "409800"
         FLT_INSTANCE: ffffcb0e0f1e04e0 "bindflt Instance" "409800"
      FLT_FILTER: ffffcb0e0b3ba020 "WdFilter" "328010"
         FLT_INSTANCE: ffffcb0e0bb5fa80 "WdFilter Instance" "328010"
         FLT_INSTANCE: ffffcb0e0bda38b0 "WdFilter Instance" "328010"
         FLT_INSTANCE: ffffcb0e0be2f010 "WdFilter Instance" "328010"
         FLT_INSTANCE: ffffcb0e0df4d930 "WdFilter Instance" "328010"
      FLT_FILTER: ffffcb0e0b3957e0 "storqosflt" "244000"
      FLT_FILTER: ffffcb0e0b397920 "wcifs" "189900"
      FLT_FILTER: ffffcb0e0b391aa0 "CldFlt" "180451"
      FLT_FILTER: ffffcb0e0bdb4050 "FileCrypt" "141100"
      FLT_FILTER: ffffcb0e0b397010 "luafv" "135000"
         FLT_INSTANCE: ffffcb0e0b393010 "luafv" "135000"
      FLT_FILTER: ffffcb0e10887aa0 "DemoMinifilter" "123456"
         FLT_INSTANCE: ffffcb0e10886aa0 "AltitudeAndFlags" "123456"
         FLT_INSTANCE: ffffcb0e10876aa0 "AltitudeAndFlags" "123456"
         FLT_INSTANCE: ffffcb0e10875aa0 "AltitudeAndFlags" "123456"
         FLT_INSTANCE: ffffcb0e10b32aa0 "AltitudeAndFlags" "123456"
      FLT_FILTER: ffffcb0e0d156700 "npsvctrig" "46000"
         FLT_INSTANCE: ffffcb0e0be738a0 "npsvctrig" "46000"
      FLT_FILTER: ffffcb0e0b3837f0 "Wof" "40700"
         FLT_INSTANCE: ffffcb0e0bc6bb20 "Wof Instance" "40700"
         FLT_INSTANCE: ffffcb0e0df52b00 "Wof Instance" "40700"
      FLT_FILTER: ffffcb0e0b9beaa0 "FileInfo" "40500"
         FLT_INSTANCE: ffffcb0e0bb279a0 "FileInfo" "40500"
         FLT_INSTANCE: ffffcb0e0bc698a0 "FileInfo" "40500"
         FLT_INSTANCE: ffffcb0e0bad18a0 "FileInfo" "40500"
         FLT_INSTANCE: ffffcb0e0df771e0 "FileInfo" "40500"
```

List the frames and the filters/instances associated with each frame.
Inspecting the first (and only) frame.

```         
kd> dt FLTMGR!_FLTP_FRAME ffffcb0e0b3a5020
   +0x000 Type             : _FLT_TYPE
   +0x008 Links            : _LIST_ENTRY [ 0xfffff805`0fcbb780 - 0xfffff805`0fcbb780 ]
   +0x018 FrameID          : 0
   +0x020 AltitudeIntervalLow : _UNICODE_STRING "0"
   +0x030 AltitudeIntervalHigh : _UNICODE_STRING "409800"
   +0x040 LargeIrpCtrlStackSize : 0x6 ''
   +0x041 SmallIrpCtrlStackSize : 0x1 ''
   +0x048 RegisteredFilters : _FLT_RESOURCE_LIST_HEAD
   +0x0c8 AttachedVolumes  : _FLT_RESOURCE_LIST_HEAD
   +0x148 MountingVolumes  : _LIST_ENTRY [ 0xffffcb0e`0b3a5168 - 0xffffcb0e`0b3a5168 ]
   +0x158 AttachedFileSystems : _FLT_MUTEX_LIST_HEAD
   +0x1a8 ZombiedFltObjectContexts : _FLT_MUTEX_LIST_HEAD
   +0x1f8 KtmResourceManagerHandle : 0xffffffff`800001cc Void
   +0x200 KtmResourceManager : 0xffffcb0e`0b359c00 _KRESOURCEMANAGER
   +0x208 FilterUnloadLock : _ERESOURCE
   +0x270 DeviceObjectAttachLock : _FAST_MUTEX
   +0x2a8 Prcb             : 0xffffcb0e`0b2a3dc0 _FLT_PRCB
   +0x2b0 PrcbPoolToFree   : 0xffffcb0e`0b2a3da0 Void
   +0x2b8 LookasidePoolToFree : 0xffffcb0e`0b3f3940 Void
   +0x2c0 IrpCtrlStackProfiler : _FLTP_IRPCTRL_STACK_PROFILER
   +0x400 SmallIrpCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x480 LargeIrpCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x500 ReserveIrpCtrls  : _RESERVE_IRPCTRL

```

Walking the list of registered filters by first inspecting the resource list head object.

```
kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 (*((FLTMGR!_FLT_RESOURCE_LIST_HEAD *)0xffffcb0e0b3a5068))
(*((FLTMGR!_FLT_RESOURCE_LIST_HEAD *)0xffffcb0e0b3a5068))                 [Type: _FLT_RESOURCE_LIST_HEAD]
    [+0x000] rLock            : Unowned Resource [Type: _ERESOURCE]
    [+0x068] rList            [Type: _LIST_ENTRY]
    [+0x078] rCount           : 0xb [Type: unsigned long]
```

Inspecting the actual list head to get to the linked list.

```

kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 (*((FLTMGR!_LIST_ENTRY *)0xffffcb0e0b3a50d0))
(*((FLTMGR!_LIST_ENTRY *)0xffffcb0e0b3a50d0))                 [Type: _LIST_ENTRY]
    [+0x000] Flink            : 0xffffcb0e0b386020 [Type: _LIST_ENTRY *]
    [+0x008] Blink            : 0xffffcb0e0b9beab0 [Type: _LIST_ENTRY *]


// Offset subtracted to align correctly

kd> dt fltmgr!_FLT_FILTER 0xffffcb0e0b386020-0x10
   +0x000 Base             : _FLT_OBJECT
   +0x030 Frame            : 0xffffcb0e`0b3a5020 _FLTP_FRAME
   +0x038 Name             : _UNICODE_STRING "bindflt"
   +0x048 DefaultAltitude  : _UNICODE_STRING "409800"
   +0x058 Flags            : 0xd6 (No matching name)
   +0x060 DriverObject     : 0xffffcb0e`0b3c0e30 _DRIVER_OBJECT
   +0x068 InstanceList     : _FLT_RESOURCE_LIST_HEAD
   +0x0e8 VerifierExtension : (null) 
   +0x0f0 VerifiedFiltersLink : _LIST_ENTRY [ 0x00000000`00000000 - 0x00000000`00000000 ]
   +0x100 FilterUnload     : 0xfffff805`1c643210     long  bindflt!BfFltUnload+0
   +0x108 InstanceSetup    : 0xfffff805`1c63c4b0     long  bindflt!BfInstanceSetup+0
   +0x110 InstanceQueryTeardown : 0xfffff805`1c63d150     long  bindflt!BfInstanceQueryTeardown+0
   +0x118 InstanceTeardownStart : 0xfffff805`1c63d250     void  bindflt!BfInstanceTeardownStart+0
   +0x120 InstanceTeardownComplete : 0xfffff805`1c63d240     void  bindflt!BfInstanceTeardownComplete+0
   +0x128 SupportedContextsListHead : 0xffffcb0e`103f3df0 _ALLOCATE_CONTEXT_HEADER
   +0x130 SupportedContexts : [7] (null) 
   +0x168 PreVolumeMount   : 0xfffff805`1c63a060     _FLT_PREOP_CALLBACK_STATUS  bindflt!BfCommonPreOp+0
   +0x170 PostVolumeMount  : (null) 
   +0x178 GenerateFileName : 0xfffff805`1c638960     long  bindflt!BfGenerateFileNameCallback+0
   +0x180 NormalizeNameComponent : (null) 
   +0x188 NormalizeNameComponentEx : 0xfffff805`1c641c30     long  bindflt!BfNormalizeNameComponentExCallback+0
   +0x190 NormalizeContextCleanup : (null) 
   +0x198 KtmNotification  : (null) 
   +0x1a0 SectionNotification : (null) 
   +0x1a8 Operations       : 0xffffcb0e`0b3862c8 _FLT_OPERATION_REGISTRATION
   +0x1b0 OldDriverUnload  : (null) 
   +0x1b8 ActiveOpens      : _FLT_MUTEX_LIST_HEAD
   +0x208 ConnectionList   : _FLT_MUTEX_LIST_HEAD
   +0x258 PortList         : _FLT_MUTEX_LIST_HEAD
   +0x2a8 PortLock         : _EX_PUSH_LOCK_AUTO_EXPAND
```

That's all well and good, but those are the `FLT_FILTER` structs, where are the `FLT_INSTANCE` structs???
To get to them we first have to inspect each volume. Remember: Instances are associated with volumes, and are not referenced in `_FLTP_FRAME->RegisteredFilters`.

```
kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 (*((FLTMGR!_FLT_RESOURCE_LIST_HEAD *)0xffffcb0e0b3a50e8))
(*((FLTMGR!_FLT_RESOURCE_LIST_HEAD *)0xffffcb0e0b3a50e8))                 [Type: _FLT_RESOURCE_LIST_HEAD]
    [+0x000] rLock            : Unowned Resource [Type: _ERESOURCE]
    [+0x068] rList            [Type: _LIST_ENTRY]
    [+0x078] rCount           : 0x6 [Type: unsigned long]
    
kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 (*((FLTMGR!_LIST_ENTRY *)0xffffcb0e0b3a5150))
(*((FLTMGR!_LIST_ENTRY *)0xffffcb0e0b3a5150))                 [Type: _LIST_ENTRY]
    [+0x000] Flink            : 0xffffcb0e0bb26760 [Type: _LIST_ENTRY *]
    [+0x008] Blink            : 0xffffcb0e0df46020 [Type: _LIST_ENTRY *]

// Go to FLINK (list head)

kd> dt FLTMGR!_FLT_VOLUME 0xffffcb0e0bb26760-0x10
   +0x000 Base             : _FLT_OBJECT
   +0x030 Flags            : 0x1e5 (No matching name)
   +0x034 FileSystemType   : d ( FLT_FSTYPE_MUP )
   +0x038 DeviceObject     : 0xffffcb0e`0ba2cb10 _DEVICE_OBJECT
   +0x040 DiskDeviceObject : (null) 
   +0x048 FrameZeroVolume  : 0xffffcb0e`0bb26750 _FLT_VOLUME
   +0x050 VolumeInNextFrame : (null) 
   +0x058 Frame            : 0xffffcb0e`0b3a5020 _FLTP_FRAME
   +0x060 DeviceName       : _UNICODE_STRING "\Device\Mup"
   +0x070 GuidName         : _UNICODE_STRING ""
   +0x080 CDODeviceName    : _UNICODE_STRING "\Device\Mup"
   +0x090 CDODriverName    : _UNICODE_STRING "\FileSystem\Mup"
   +0x0a0 InstanceList     : _FLT_RESOURCE_LIST_HEAD
   +0x120 Callbacks        : _CALLBACK_CTRL
   +0x508 ContextLock      : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x518 VolumeContexts   : _CONTEXT_LIST_CTRL
   +0x520 StreamListCtrls  : _FLT_RESOURCE_LIST_HEAD
   +0x5a0 FileListCtrls    : _FLT_RESOURCE_LIST_HEAD
   +0x620 NameCacheCtrl    : _NAME_CACHE_VOLUME_CTRL
   +0x6d8 MountNotifyLock  : _ERESOURCE
   +0x740 TargetedOpenActiveCount : 0n0
   +0x748 TxVolContextListLock : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x758 TxVolContexts    : _TREE_ROOT
   +0x760 SupportedFeatures : 0n0
   +0x764 BypassFailingFltNameLen : 0
   +0x766 BypassFailingFltName : [32]  ""

// Go to FLINK

kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 ((FLTMGR!_LIST_ENTRY *)0xffffcb0e0bb26760)
((FLTMGR!_LIST_ENTRY *)0xffffcb0e0bb26760)                 : 0xffffcb0e0bb26760 [Type: _LIST_ENTRY *]
    [+0x000] Flink            : 0xffffcb0e0bc62490 [Type: _LIST_ENTRY *]
    [+0x008] Blink            : 0xffffcb0e0b3a5150 [Type: _LIST_ENTRY *]
kd> dt FLTMGR!_FLT_VOLUME 0xffffcb0e0bc62490-0x10
   +0x000 Base             : _FLT_OBJECT
   +0x030 Flags            : 0x564 (No matching name)
   +0x034 FileSystemType   : 2 ( FLT_FSTYPE_NTFS )
   +0x038 DeviceObject     : 0xffffcb0e`0ba329d0 _DEVICE_OBJECT
   +0x040 DiskDeviceObject : 0xffffcb0e`0bb238f0 _DEVICE_OBJECT
   +0x048 FrameZeroVolume  : 0xffffcb0e`0bc62480 _FLT_VOLUME
   +0x050 VolumeInNextFrame : (null) 
   +0x058 Frame            : 0xffffcb0e`0b3a5020 _FLTP_FRAME
   +0x060 DeviceName       : _UNICODE_STRING "\Device\HarddiskVolume4"
   +0x070 GuidName         : _UNICODE_STRING "\??\Volume{980944d3-e7a1-400d-a9d7-4a890dc7dcee}"
   +0x080 CDODeviceName    : _UNICODE_STRING "\Ntfs"
   +0x090 CDODriverName    : _UNICODE_STRING "\FileSystem\Ntfs"
   +0x0a0 InstanceList     : _FLT_RESOURCE_LIST_HEAD
   +0x120 Callbacks        : _CALLBACK_CTRL
   +0x508 ContextLock      : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x518 VolumeContexts   : _CONTEXT_LIST_CTRL
   +0x520 StreamListCtrls  : _FLT_RESOURCE_LIST_HEAD
   +0x5a0 FileListCtrls    : _FLT_RESOURCE_LIST_HEAD
   +0x620 NameCacheCtrl    : _NAME_CACHE_VOLUME_CTRL
   +0x6d8 MountNotifyLock  : _ERESOURCE
   +0x740 TargetedOpenActiveCount : 0n562
   +0x748 TxVolContextListLock : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x758 TxVolContexts    : _TREE_ROOT
   +0x760 SupportedFeatures : 0n12
   +0x764 BypassFailingFltNameLen : 0
   +0x766 BypassFailingFltName : [32]  ""


... And so on
```

Now that we have a `FLT_VOLUME`, enumerate all `FLT_INSTANCE` structs associated with the volume.