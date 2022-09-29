## 0.0 - Intro
### 0.1 Abstract
This research project served to help me learn more about file system minifilter drivers and how a malicious actor may leverage a vulnerable driver to patch callbacks for minifilters. Along the way, I'd discovered previous research by Aviad Shamriz which helped me immensely in my endeavor.

https://aviadshamriz.medium.com/part-1-fs-minifilter-hooking-7e743b042a9d

As this article goes very in-depth into the mechanics of file system minifilter hooking with another loaded driver, I will focus on my research methods which led me to develop a PoC leveraging Dell's vulnerable "dbutil" driver to perform the same actions from user-mode, and some things I learned along the way.

### 0.2 Acknowledgements
Thank you to James Forshaw, Avid Shamriz, and MZakocs for your work which helped make this possible.
https://aviadshamriz.medium.com/part-1-fs-minifilter-hooking-7e743b042a9d
https://github.com/mzakocs/CVE-2021-21551-POC
https://googleprojectzero.blogspot.com/2021/01/hunting-for-bugs-in-windows-mini-filter.html

Shoutout to the vxug community and my friends for inspiration and guidance:
* ch3rn0byl
* s4r1n
* cb
* Jonas

### 0.3 Setup
My testing and development setup was original Windows 10 1809, though after a lovely VMWare crash which led me to lose my VM, I have since re-performed my research on the Microsoft-provided Windows 11 Enterprise Hyper-V evaluation image.

## 1.0 - What is a file system mini filter
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/about-file-system-filter-drivers
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/how-file-system-filter-drivers-are-similar-to-device-drivers
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/how-file-system-filter-drivers-are-different-from-device-drivers
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/storage-device-stacks--storage-volumes--and-file-system-stacks

File system minifilters are drivers which are used to inspect, log, modify, or prevent file system I/O operations. The filter manager driver (FltMgr.sys) effectively "sits in-between" the I/O Manager and the File System Driver, and is responsible for registration of file system minifilter drivers, and the invocation of their pre and post-operation callbacks. Such callbacks are provided by the minifilter, and are to be invoked before or after the I/O operation.

>A minifilter driver attaches to the file system stack indirectly, by registering with _FltMgr_ for the I/O operations that the minifilter driver chooses to filter.
*https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts*

FltMgr also maintains a list of volumes attached to the system, and is responsible for storing and invoking callbacks on a per-volume basis.

### 1.1 - Core concepts and APIs

#### Altitude
As previously mentioned, minifilters "sit in-between" the I/O manager and the filesystem driver. One of the fundamental questions and concepts which arose from the filtering behavior is: 
How do I know where in the "stack" my driver sits? 
What path does an IRP take from the I/O manager to the filesystem driver?

The minifilter's Altitude describes it's load order. For example, a minifilter with an altitude of "30000" will be loaded into the I/O stack before a minifilter with an altitude of "30100."

https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/load-order-groups-and-altitudes-for-minifilter-drivers

```
 ┌──────────────────────┐
 │                      │
 │     I/O Manager      │            ┌───────────────────┐
 │                      │            │  Minifilter2:     │
 └───────────┬──────────┘     ┌──────►  Altitude 42000   │
             │                │      │                   │
             │                │      └───────────────────┘
             │                │                
 ┌───────────▼──────────┐     │      ┌───────────────────┐
 │                      ◄─────┘      │  Minifilter1:     │
 │        FLTMGR        ◄────────────►  Altitude 30100   │
 │                      ◄─────┐      │                   │
 └───────────┬──────────┘     │      └───────────────────┘
             │                │                
             │                │      ┌───────────────────┐
             │                │      │  Minifilter0:     │
             │                └──────►  Altitude 30000   │
 ┌───────────▼──────────┐            │                   │
 │                      │            └───────────────────┘
 │ Storage driver stack │
 │                      │
 └───────────┬──────────┘
             │
             │
             ▼                          
			...
```
(Fig 1) Simplified version of figure 1: https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts

#### Frames
Frames describe a range of Altitudes, and the mini filters and volumes associated with them.
>For interoperability with legacy filter drivers, _FltMgr_ can attach filter device objects to a file system I/O stack in more than one location. Each of _FltMgr_'s filter device objects is called a _frame_. From the perspective of a legacy filter driver, each filter manager frame is just another legacy filter driver.
Each filter manager frame represents a range of altitudes. The filter manager can adjust an existing frame or create a new frame to allow minifilter drivers to attach at the correct location.
https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts

```
┌──────────────────────┐
│                      │
│                      │
│     I/O Manager      │
│                      │
│                      │               ┌────────────────────────┐
└──────────┬───────────┘               │                        │
           │                           │   Filter3              │
           │                    ┌──────►   Altitude: 365000     │
┌──────────▼───────────┐        │      │                        │
│                      │        │      └────────────────────────┘
│     Frame 1          ◄────────┘
│     Altitude:        │
│     305000 - 409500  ◄────────┐      ┌────────────────────────┐
│                      │        │      │                        │
└──────────┬───────────┘        │      │   Filter2              │
           │                    └──────►   Altitude: 325000     │
           │                           │                        │
┌──────────▼───────────┐               └────────────────────────┘
│                      │
│    Legacy Filter     │
│    (No Altitude)     │
│                      │
│                      │
└──────────┬───────────┘               ┌────────────────────────┐
           │                           │                        │
           │                           │   Filter1              │
┌──────────▼───────────┐        ┌──────►   Altitude: 165000     │
│                      │        │      │                        │
│      Frame 0         ◄────────┘      └────────────────────────┘
│      Altitude:       │
│      0 - 304999      ◄────────┐
│                      │        │      ┌────────────────────────┐
└──────────┬───────────┘        │      │                        │
           │                    │      │   Filter0              │
           │                    └──────►   Altitude: 145000     │
┌──────────▼───────────┐               │                        │
│                      │               └────────────────────────┘
│ Storage driver stack │
│                      │
└──────────────────────┘
```
(Fig 2) Simplified version of figure 2: https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts

#### FltRegisterFilter
The `FltRegisterFilter` function is the API used by a minifilter to register with FltMgr.

```
NTSTATUS FLTAPI FltRegisterFilter( 
	[in] PDRIVER_OBJECT Driver, 
	[in] const FLT_REGISTRATION *Registration, 
	[out] PFLT_FILTER *RetFilter 
);
```

#### FLT_REGISTRATION
A minifilter driver must provide a `FLT_REGISTRATION` structure containing, among other things, instance setup/teardown callbacks, filter unload callbacks, and a list of I/O operations to filter (`FLT_OPERATION_REGISTRATION OperationRegistration`).
```
kd> dt FLTMGR!_FLT_REGISTRATION
   +0x000 Size             : Uint2B
   +0x002 Version          : Uint2B
   +0x004 Flags            : Uint4B
   +0x008 ContextRegistration : Ptr64 _FLT_CONTEXT_REGISTRATION
   +0x010 OperationRegistration : Ptr64 _FLT_OPERATION_REGISTRATION
   +0x018 FilterUnloadCallback : Ptr64     long 
   +0x020 InstanceSetupCallback : Ptr64     long 
   +0x028 InstanceQueryTeardownCallback : Ptr64     long 
   +0x030 InstanceTeardownStartCallback : Ptr64     void 
   +0x038 InstanceTeardownCompleteCallback : Ptr64     void 
   +0x040 GenerateFileNameCallback : Ptr64     long 
   +0x048 NormalizeNameComponentCallback : Ptr64     long 
   +0x050 NormalizeContextCleanupCallback : Ptr64     void 
   +0x058 TransactionNotificationCallback : Ptr64     long 
   +0x060 NormalizeNameComponentExCallback : Ptr64     long 
   +0x068 SectionNotificationCallback : Ptr64     long
```

#### FLT_OPERATION_REGISTRATION
The `FLT_OPERATION_REGISTRATION` structure defines the I/O request Major Function to filter, and defines a pre and post-operation callback which will be invoked, respectively, before or after the I/O operation is passed down to / back up from the I/O stack.
```
typedef struct _FLT_OPERATION_REGISTRATION {

    UCHAR MajorFunction;
    FLT_OPERATION_REGISTRATION_FLAGS Flags;
    PFLT_PRE_OPERATION_CALLBACK PreOperation;
    PFLT_POST_OPERATION_CALLBACK PostOperation;

    PVOID Reserved1;

} FLT_OPERATION_REGISTRATION, *PFLT_OPERATION_REGISTRATION;
```

The list of operations is terminated by a `FLT_OPERATION_REGISTRATION` structure whose Major Function is `IRP_MJ_OPERATION_END`.
For example, a minifilter driver that only filters `IRP_MJ_CREATE` operations and only provides a pre-operation callback may use the following list of `FLT_REGISTRATION` structures:

```
const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{
		IRP_MJ_CREATE,
		0,
		(PFLT_PRE_OPERATION_CALLBACK) PreCreateCallback,
		(PFLT_POST_OPERATION_CALLBACK) NULL,
	},
	{ IRP_MJ_OPERATION_END } // list terminator
};
```

#### PFLT_PRE_OPERATION_CALLBACK
The function typedef for a pre-operation callback:
```
FLT_PREOP_CALLBACK_STATUS PfltPreOperationCallback( 
	[in, out] PFLT_CALLBACK_DATA Data, 
	[in] PCFLT_RELATED_OBJECTS FltObjects, 
	[out] PVOID *CompletionContext 
) { ... }
```
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nc-fltkernel-pflt_pre_operation_callback

#### PFLT_POST_OPERATION_CALLBACK
The function typedef for a post-operation callback:
```
FLT_POSTOP_CALLBACK_STATUS PfltPostOperationCallback( 
	[in, out] PFLT_CALLBACK_DATA Data, 
	[in] PCFLT_RELATED_OBJECTS FltObjects, 
	[in, optional] PVOID CompletionContext, 
	[in] FLT_POST_OPERATION_FLAGS Flags 
) {...}
```
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nc-fltkernel-pflt_pre_operation_callback

#### FltStartFiltering
The # FltStartFiltering API is used to, well, indicate to the filter manager to attach the filter to each volume and start filtering.
```
NTSTATUS FLTAPI FltStartFiltering( [in] PFLT_FILTER Filter );
```
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nf-fltkernel-fltstartfiltering

#### Filter (`_FLT_FILTER`)
A filter object represents a filter... truly breaking ground here.
Importantly for our purposes, the filter object contains a reference to the filter's name and callback table provided when the driver is registered by the api `FltRegisterFilter`

```
kd> dt FLTMGR!_FLT_FILTER
   +0x000 Base             : _FLT_OBJECT
   +0x030 Frame            : Ptr64 _FLTP_FRAME
   +0x038 Name             : _UNICODE_STRING
   +0x048 DefaultAltitude  : _UNICODE_STRING
   +0x058 Flags            : _FLT_FILTER_FLAGS
   +0x060 DriverObject     : Ptr64 _DRIVER_OBJECT
   +0x068 InstanceList     : _FLT_RESOURCE_LIST_HEAD
   +0x0e8 VerifierExtension : Ptr64 _FLT_VERIFIER_EXTENSION
   +0x0f0 VerifiedFiltersLink : _LIST_ENTRY
   +0x100 FilterUnload     : Ptr64     long 
   +0x108 InstanceSetup    : Ptr64     long 
   +0x110 InstanceQueryTeardown : Ptr64     long 
   +0x118 InstanceTeardownStart : Ptr64     void 
   +0x120 InstanceTeardownComplete : Ptr64     void 
   +0x128 SupportedContextsListHead : Ptr64 _ALLOCATE_CONTEXT_HEADER
   +0x130 SupportedContexts : [7] Ptr64 _ALLOCATE_CONTEXT_HEADER
   +0x168 PreVolumeMount   : Ptr64     _FLT_PREOP_CALLBACK_STATUS 
   +0x170 PostVolumeMount  : Ptr64     _FLT_POSTOP_CALLBACK_STATUS 
   +0x178 GenerateFileName : Ptr64     long 
   +0x180 NormalizeNameComponent : Ptr64     long 
   +0x188 NormalizeNameComponentEx : Ptr64     long 
   +0x190 NormalizeContextCleanup : Ptr64     void 
   +0x198 KtmNotification  : Ptr64     long 
   +0x1a0 SectionNotification : Ptr64     long 
   +0x1a8 Operations       : Ptr64 _FLT_OPERATION_REGISTRATION
   +0x1b0 OldDriverUnload  : Ptr64     void 
   +0x1b8 ActiveOpens      : _FLT_MUTEX_LIST_HEAD
   +0x208 ConnectionList   : _FLT_MUTEX_LIST_HEAD
   +0x258 PortList         : _FLT_MUTEX_LIST_HEAD
   +0x2a8 PortLock         : _EX_PUSH_LOCK_AUTO_EXPAND
```

You can view a list of filters via Windbg by issuing the command `!fltkd.filters`

```
kd> !fltkd.filters

Filter List: ffffcb0e0b3a50d0 "Frame 0" 
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

#### Instance (`_FLT_INSTANCE`)
>The attachment of a minifilter driver at a particular altitude on a particular volume is called an _instance_ of the minifilter driver.
https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts

```
kd> dt FLTMGR!_FLT_INSTANCE
   +0x000 Base             : _FLT_OBJECT
   +0x030 OperationRundownRef : Ptr64 _EX_RUNDOWN_REF_CACHE_AWARE
   +0x038 Volume           : Ptr64 _FLT_VOLUME
   +0x040 Filter           : Ptr64 _FLT_FILTER
   +0x048 Flags            : _FLT_INSTANCE_FLAGS
   +0x050 Altitude         : _UNICODE_STRING
   +0x060 Name             : _UNICODE_STRING
   +0x070 FilterLink       : _LIST_ENTRY
   +0x080 ContextLock      : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x090 Context          : Ptr64 _CONTEXT_NODE
   +0x098 TransactionContexts : _CONTEXT_LIST_CTRL
   +0x0a0 TrackCompletionNodes : Ptr64 _TRACK_COMPLETION_NODES
   +0x0a8 CallbackNodes    : [50] Ptr64 _CALLBACK_NODE
```

#### Volume (`_FLT_VOLUME`)
https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/storage-device-stacks--storage-volumes--and-file-system-stacks
https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/how-the-volume-is-mounted
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_vpb

A `_FLT_VOLUME` represents a mounted volume on the system (shocking, I know). Among other things, a volume object contains a list of mini filter instances attached to the volume, as well as an object referencing a list of Callbacks for all supported IRP Major Functions.

```
kd> dt FLTMGR!_FLT_VOLUME
   +0x000 Base             : _FLT_OBJECT
   +0x030 Flags            : _FLT_VOLUME_FLAGS
   +0x034 FileSystemType   : _FLT_FILESYSTEM_TYPE
   +0x038 DeviceObject     : Ptr64 _DEVICE_OBJECT
   +0x040 DiskDeviceObject : Ptr64 _DEVICE_OBJECT
   +0x048 FrameZeroVolume  : Ptr64 _FLT_VOLUME
   +0x050 VolumeInNextFrame : Ptr64 _FLT_VOLUME
   +0x058 Frame            : Ptr64 _FLTP_FRAME
   +0x060 DeviceName       : _UNICODE_STRING
   +0x070 GuidName         : _UNICODE_STRING
   +0x080 CDODeviceName    : _UNICODE_STRING
   +0x090 CDODriverName    : _UNICODE_STRING
   +0x0a0 InstanceList     : _FLT_RESOURCE_LIST_HEAD
   +0x120 Callbacks        : _CALLBACK_CTRL
   +0x508 ContextLock      : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x518 VolumeContexts   : _CONTEXT_LIST_CTRL
   +0x520 StreamListCtrls  : _FLT_RESOURCE_LIST_HEAD
   +0x5a0 FileListCtrls    : _FLT_RESOURCE_LIST_HEAD
   +0x620 NameCacheCtrl    : _NAME_CACHE_VOLUME_CTRL
   +0x6d8 MountNotifyLock  : _ERESOURCE
   +0x740 TargetedOpenActiveCount : Int4B
   +0x748 TxVolContextListLock : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x758 TxVolContexts    : _TREE_ROOT
   +0x760 SupportedFeatures : Int4B
   +0x764 BypassFailingFltNameLen : Uint2B
   +0x766 BypassFailingFltName : [32] Wchar
```

It is important to note that the invocation of the callbacks per-volume uses the volume's associated `_CALLBACK_CTRL` object, and that object's list of `_CALLBACK_NODE`  elements.

You can view the list of volumes in Windbg using the command `!fltkd.volumes`
```
kd> !fltkd.volumes

Volume List: ffffcb0e0b3a5150 "Frame 0" 
   FLT_VOLUME: ffffcb0e0bb26750 "\Device\Mup"
      FLT_INSTANCE: ffffcb0e0bb5fa80 "WdFilter Instance" "328010"
      FLT_INSTANCE: ffffcb0e10886aa0 "AltitudeAndFlags" "123456"
      FLT_INSTANCE: ffffcb0e0bb279a0 "FileInfo" "40500"
   FLT_VOLUME: ffffcb0e0bc62480 "\Device\HarddiskVolume4"
      FLT_INSTANCE: ffffcb0e0f1e04e0 "bindflt Instance" "409800"
      FLT_INSTANCE: ffffcb0e0bda38b0 "WdFilter Instance" "328010"
      FLT_INSTANCE: ffffcb0e0b393010 "luafv" "135000"
      FLT_INSTANCE: ffffcb0e10876aa0 "AltitudeAndFlags" "123456"
      FLT_INSTANCE: ffffcb0e0bc6bb20 "Wof Instance" "40700"
      FLT_INSTANCE: ffffcb0e0bc698a0 "FileInfo" "40500"
   FLT_VOLUME: ffffcb0e0be71010 "\Device\NamedPipe"
      FLT_INSTANCE: ffffcb0e0be738a0 "npsvctrig" "46000"
   FLT_VOLUME: ffffcb0e0be72010 "\Device\Mailslot"
   FLT_VOLUME: ffffcb0e0bfc0520 "\Device\HarddiskVolume2"
      FLT_INSTANCE: ffffcb0e0be2f010 "WdFilter Instance" "328010"
      FLT_INSTANCE: ffffcb0e10875aa0 "AltitudeAndFlags" "123456"
      FLT_INSTANCE: ffffcb0e0bad18a0 "FileInfo" "40500"
   FLT_VOLUME: ffffcb0e0df46010 "\Device\HarddiskVolume1"
      FLT_INSTANCE: ffffcb0e0df4d930 "WdFilter Instance" "328010"
      FLT_INSTANCE: ffffcb0e10b32aa0 "AltitudeAndFlags" "123456"
      FLT_INSTANCE: ffffcb0e0df52b00 "Wof Instance" "40700"
      FLT_INSTANCE: ffffcb0e0df771e0 "FileInfo" "40500"
```

#### Frames (Cont.) (`_FLTP_FRAME`)
Examining the `_FLTP_FRAME` object in Windbg, we can se a clearer relationship between frames, filters, and volumes.
```
kd> dt FLTMGR!_FLTP_FRAME
   +0x000 Type             : _FLT_TYPE
   +0x008 Links            : _LIST_ENTRY
   +0x018 FrameID          : Uint4B
   +0x020 AltitudeIntervalLow : _UNICODE_STRING
   +0x030 AltitudeIntervalHigh : _UNICODE_STRING
   +0x040 LargeIrpCtrlStackSize : UChar
   +0x041 SmallIrpCtrlStackSize : UChar
   +0x048 RegisteredFilters : _FLT_RESOURCE_LIST_HEAD
   +0x0c8 AttachedVolumes  : _FLT_RESOURCE_LIST_HEAD
   +0x148 MountingVolumes  : _LIST_ENTRY
   +0x158 AttachedFileSystems : _FLT_MUTEX_LIST_HEAD
   +0x1a8 ZombiedFltObjectContexts : _FLT_MUTEX_LIST_HEAD
   +0x1f8 KtmResourceManagerHandle : Ptr64 Void
   +0x200 KtmResourceManager : Ptr64 _KRESOURCEMANAGER
   +0x208 FilterUnloadLock : _ERESOURCE
   +0x270 DeviceObjectAttachLock : _FAST_MUTEX
   +0x2a8 Prcb             : Ptr64 _FLT_PRCB
   +0x2b0 PrcbPoolToFree   : Ptr64 Void
   +0x2b8 LookasidePoolToFree : Ptr64 Void
   +0x2c0 IrpCtrlStackProfiler : _FLTP_IRPCTRL_STACK_PROFILER
   +0x400 SmallIrpCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x480 LargeIrpCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x500 ReserveIrpCtrls  : _RESERVE_IRPCTRL
```

To help visualize their association, the following chart describes a high level overview of a frame on a system with a single frame: 

```
           
           
         _FLTP_FRAME
        ┌─────────────────────────────────────────────────────────┐
        │                                                         │
        │  Type: _FLT_TYPE                                        │
        │  Links: _LIST_ENTRY                                     │
        │  FrameID: 0                                             │
        │  AltitudeIntervalLow: "0"                               │
        │  AltitudeIntervalHigh: "409500"                         │
        │  ...                                                    │
   ┌────┼─ RegisteredFilters: _FLT_RESOURCE_LIST_HEAD             │
┌──┼────┤  AttachedVolumes: _FLT_RESOURCE_LIST_HEAD               │
│  │    │  ...                                                    │
│  │    └─────────────────────────────────────────────────────────┘
│  │
│  │     _FLT_RESOURCE_LIST_HEAD (Filters)
│  │    ┌─────────────────────┐
│  └────► rLock: _ERESOURCE   │            ┌───────────────┐
│       │ rList: _LIST_ENTRY  ├────────────► FLT_FILTER 0  ◄─────┐
│       │ Count: 0xb          │            └───────┬───────┘     │
│       └─────────────────────┘                    │             │
│                                          ┌───────▼───────┐     │
│                                          │ FLT_FILTER 1  │     │
│                                          └───────┬───────┘     │
│                                                  │             │
│                                          ┌───────▼───────┐     │
│                                          │ FLT_FILTER 2  │     │
│                                          └───────┬───────┘     │
│                                                  │             │
│                                          ┌───────▼───────┐     │
│                                          │ FLT_FILTER 3  ├─────┘
│                                          └───────────────┘
│
│
│
│
│        _FLT_RESOURCE_LIST_HEAD (Volumes)
│       ┌──────────────────────────┐
└───────► rLock: _ERESOURCE        │
        │ rList: _LIST_ENTRY ───┐  │
        │ Count: 0x6            │  │
        └───────────────────────┼──┘
                                │
                                │
                                │
                                │
                _FLT_VOLUME     │
                ┌───────────────▼──────────────────────────┐
                │ \Device\Mup                              │
                │ Callbacks: _CALLBACK_CTRL                ◄───────┐
                │ InstanceList: _FLT_RESOURCE_LIST_HEAD    │       │
                │                                          │       │
                └───────────────┬──────────────────────────┘       │
                                │                                  │
                _FLT_VOLUME     │                                  │
                ┌───────────────▼──────────────────────────┐       │
                │ \Device\HarddiskVolume4                  │       │
                │ Callbacks: _CALLBACK_CTRL                │       │
                │ InstanceList: _FLT_RESOURCE_LIST_HEAD    │       │
                │                                          │       │
                └───────────────┬──────────────────────────┘       │
                                │                                  │
                ┌───────────────▼──────────────────────────┐       │
                │                                          │       │
                │        ... The rest of the list ...      ├───────┘
                │                                          │
                └──────────────────────────────────────────┘
```
(Fig 3) Association between `_FLTP_FRAME` and `_FLT_VOLUME`

As shown in the type definition and Fig. 3, a frame contains a reference to all filter objects (`_FLT_FILTER`) assocaited with the frame, alongside a list of volumes (`_FLT_VOLUME`).

Most importantly, this highlights an important aspect of the proof-of-concept:
* In order to access the proper objects to remove their associated callbacks we must first examine the frame to find the registered filters. We loop over every registered filter until we find the target filter and note the callbacks supported by the filter.
* From there we must iterate over each callback table assocaited with the volume and, when we find a target callback in the list, modify the entry as desired to replace the callback for our target filter.

You can view the frames through Windbg with the command `!fltkd.frames`
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

#### Callback Ctrl (`_CALLBACK_CTRL`)
The `_CALLBACK_CTRL` structure defines a list of callback operations, indexed by their Major Function + 22.
E.g. `IRP_MJ_CONTROL (0)` would be at index 22.
```
kd> dt FLTMGR!_CALLBACK_CTRL
   +0x000 OperationLists   : [50] _LIST_ENTRY
   +0x320 OperationFlags   : [50] _CALLBACK_NODE_FLAGS
```

The OperationFlags list is a parallel array of flags per Major Function.
```
kd> dt FLTMGR!_CALLBACK_NODE_FLAGS
   CBNFL_SKIP_PAGING_IO = 0n1
   CBNFL_SKIP_CACHED_IO = 0n2
   CBNFL_USE_NAME_CALLBACK_EX = 0n4
   CBNFL_SKIP_NON_DASD_IO = 0n8
   CBNFL_SKIP_NON_CACHED_NON_PAGING_IO = 0n16
```

The cause for this offset-indexing comes from 

#### Callback Node (`_CALLBACK_NODE`)
A callback node represents a filter operation for a single I/O operation.
```
kd> dt FLTMGR!_CALLBACK_NODE
   +0x000 CallbackLinks    : _LIST_ENTRY
   +0x010 Instance         : Ptr64 _FLT_INSTANCE
   +0x018 PreOperation     : Ptr64     _FLT_PREOP_CALLBACK_STATUS 
   +0x020 PostOperation    : Ptr64     _FLT_POSTOP_CALLBACK_STATUS 
   +0x018 GenerateFileName : Ptr64     long 
   +0x018 NormalizeNameComponent : Ptr64     long 
   +0x018 NormalizeNameComponentEx : Ptr64     long 
   +0x020 NormalizeContextCleanup : Ptr64     void 
   +0x028 Flags            : _CALLBACK_NODE_FLAGS
```

The filter manager is responsible for the conversion of `FLT_REGISTRATION_OPERATION` into `_CALLBACK_NODE` structures associated with each filter instance and volume.

### 1.2 - Writing a minifilter
To aid in research and testing, I created a very simple mini filter driver to get the PID of any given process which inevitably invokes the `IRP_MJ_CREATE` operation on the file `C:\Users\<USER>\test.txt`, and log it via `DbgPrint`.

```
#include <fltKernel.h>
#include <ntddk.h>

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT pDrvObj, _In_ PUNICODE_STRING pRegPath);
NTSTATUS FsFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

PFLT_FILTER g_FilterHandle;
static const UNICODE_STRING g_TargetFileName = RTL_CONSTANT_STRING(L"\\Users\\Student\\Desktop\\test.txt");


FLT_PREOP_CALLBACK_STATUS PreCreateCallback(
	_Inout_ PFLT_CALLBACK_DATA lpFltCallbackData,
	_In_ PCFLT_RELATED_OBJECTS lpFltRelatedObj, 
	_Out_ PVOID* lpCompletionContext)
{

	UNREFERENCED_PARAMETER(lpFltCallbackData);
	*lpCompletionContext = NULL;
	PFILE_OBJECT lpFileObject = lpFltRelatedObj->FileObject;
	PUNICODE_STRING lpFileName = &lpFileObject->FileName;

	// if someone's opening the target file
	if (RtlCompareUnicodeString(&g_TargetFileName, lpFileName, TRUE) == 0) {
		HANDLE hPid = PsGetCurrentProcessId();

		// print the PID and filename to the debug console
		DbgPrint("[DEMOFLT] PID %p - Create - %wZ\n", hPid, lpFileName);
		
	}

	// do not invoke post-callbacks (there are none)
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{
		IRP_MJ_CREATE,
		0,
		PreCreateCallback,
		NULL,
	},
	{ IRP_MJ_OPERATION_END }
};

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/ns-fltkernel-_flt_registration
const FLT_REGISTRATION FltRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	NULL,
	Callbacks,
	FsFilterUnload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

NTSTATUS FsFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags) {
	UNREFERENCED_PARAMETER(Flags);
	FltUnregisterFilter(g_FilterHandle);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT pDrvObj, _In_ PUNICODE_STRING pRegPath) {
	UNREFERENCED_PARAMETER(pDrvObj);
	UNREFERENCED_PARAMETER(pRegPath);
	NTSTATUS status = 0;

	status = FltRegisterFilter(pDrvObj, &FltRegistration, &g_FilterHandle);
	if (!NT_SUCCESS(status)) {
		FltUnregisterFilter(g_FilterHandle);
		return status;
	}

	FltStartFiltering(g_FilterHandle);
	return status;
}
```
(Fig 4) DemoMinifilter.c

### 1.3 Debugging (god help me)
Now that we have a prerequisite understanding of some of the functions and APIs used, let's dig further in towards our goal of silencing callbacks by debugging the filter.

I cannot assert that this is a sane approach to debugging. YMMV.

I first started by placing a breakpoing within the `PreCreateCallback` routine at the DbgPrint statement (so I wasn't bombarded with a break at every single create operation). The breakpoint was hit by a simple `echo 1 > .\Desktop\test.txt` command.

```
kd> bp `DemoMinifilter.c:25`
kd> g
Breakpoint 0 hit
DemoMinifilter!PreCreateCallback+0x5f:
fffff805`0bf9107f 4c8b442420      mov     r8,qword ptr [rsp+20h]
```

Examining the call stack showed me the functions I needed to further inspect to trace down the functions responsible for invoking the callbacks.

```
kd> k
 # Child-SP          RetAddr               Call Site
00 fffff38b`515fa100 fffff805`0fc96f73     DemoMinifilter!PreCreateCallback+0x5f [DemoMinifilter.c @ 25] 
01 fffff38b`515fa150 fffff805`0fc96a26     FLTMGR!FltpPerformPreCallbacksWorker+0x373
02 fffff38b`515fa260 fffff805`0fccdac0     FLTMGR!FltpPassThroughInternal+0xc6
03 fffff38b`515fa2b0 fffff805`0d08a6a5     FLTMGR!FltpCreate+0x300
04 fffff38b`515fa360 fffff805`0d548d77     nt!IofCallDriver+0x55
05 fffff38b`515fa3a0 fffff805`0d539541     nt!IopParseDevice+0x897
06 fffff38b`515fa560 fffff805`0d538541     nt!ObpLookupObjectName+0xac1
07 fffff38b`515fa700 fffff805`0d4823a5     nt!ObOpenObjectByNameEx+0x1f1
08 fffff38b`515fa830 fffff805`0d22d378     nt!NtQueryAttributesFile+0x1c5
09 fffff38b`515faae0 00007ffb`c2f84324     nt!KiSystemServiceCopyEnd+0x28
```

By following execution into `FltpPerformPreCallbacksWorker`, I saw the callback nodes being iterated over, but was still confused as to how they were created/populated.

My next, naive approach was to assume that the callbacks in the `FLT_FILTER` object were the ones being invoked.
Spoiler alert: No.

After patching those through a routine in my driver that just replaced all the ones I saw in the `PFLT_FILTER` I got from `FltRegisterFilter`, I saw that they were still being invoked. Back to the drawing board.

I decided to go aaaaaaaall the way back to `FltRegisterFilter` to examine any routines I thought might be doing anything "important" with the filter. Viewing `FltStartFiltering` in IDA shows the process of filter initialization.

```
NTSTATUS __stdcall FltStartFiltering(PFLT_FILTER Filter)
{
  int v2; // ebx
  unsigned __int64 HighLimit; // [rsp+48h] [rbp+10h] BYREF
  unsigned __int64 LowLimit; // [rsp+50h] [rbp+18h] BYREF

  v2 = FltObjectReference(Filter);
  if ( v2 < 0
    || ((Filter->Flags & 2) != 0 ? (v2 = 0xC000000D) : (v2 = FltpDoVolumeNotificationForNewFilter(Filter)),
        FltObjectDereference(Filter),
        v2 < 0) )
  {
    FltpLogEventWithObjectID(&FLTMGR_START_FILTERING_FAILED, 0i64);
  }
  if ( hProvider > 5u )
  {
    HighLimit = 0i64;
    LowLimit = 0i64;
    IoGetStackLimits(&LowLimit, &HighLimit);
    if ( (unsigned __int64)&HighLimit - LowLimit < 0x200 )
      _InterlockedIncrement(&dword_1C002CAB0);
    else
      FltpTelemetryFilterStartFiltering((unsigned int)v2, Filter);
  }
  return v2;
}
```

The first steps are to check the filter's flags for the value `FLTFL_FILTERING_INITIATED`, and if the filter is initiated, return an error status.

```
kd> dt FLTMGR!_FLT_FILTER_FLAGS
   FLTFL_MANDATORY_UNLOAD_IN_PROGRESS = 0n1
   FLTFL_FILTERING_INITIATED = 0n2
   FLTFL_NAME_PROVIDER = 0n4
   FLTFL_SUPPORTS_PIPES_MAILSLOTS = 0n8
   FLTFL_BACKED_BY_PAGEFILE = 0n16
   FLTFL_SUPPORTS_DAX_VOLUME = 0n32
   FLTFL_SUPPORTS_WCOS = 0n64
   FLTFL_FILTERS_READ_WRITE = 0n128
```

Otherwise, `FltStartFiltering` calls `FltpDoVolumeNotificationForNewFilter`, which in turn calls `FltpEnumerateRegistryInstances`. 

```
__int64 __fastcall FltpDoVolumeNotificationForNewFilter(_FLT_FILTER *lpFilter)
{
  _FLTP_FRAME *Frame; // rbx
  NTSTATUS v3; // edi
  struct _ERESOURCE *p_rLock; // rbp
  _LIST_ENTRY *p_rList; // r15
  _LIST_ENTRY *Flink; // rbx
  PFLT_VOLUME lpVolume; // rsi

  Frame = lpFilter->Frame;
  lpFilter->Flags |= 2u;
  v3 = 0;
  KeEnterCriticalRegion();
  p_rLock = &Frame->AttachedVolumes.rLock;
  ExAcquireResourceSharedLite(&Frame->AttachedVolumes.rLock, 1u);
  p_rList = &Frame->AttachedVolumes.rList;
  Flink = Frame->AttachedVolumes.rList.Flink;
  while ( Flink != p_rList )
  {
    lpVolume = (PFLT_VOLUME)&Flink[-1];
    v3 = FltObjectReference(&Flink[-1]);
    if ( v3 < 0 )
    {
      Flink = Flink->Flink;
      v3 = 0;
    }
    else if ( (lpVolume->Flags & 4) != 0 )
    {
	  // (lpVolume->Flags & VOLFL_MOUNT_SETUP_NOTIFIES_CALLED) != 0
      ExReleaseResourceLite(p_rLock);
      KeLeaveCriticalRegion();
      ((void (__fastcall *)(_QWORD *))FltpEnumerateRegistryInstances)(lpFilter);
      v3 = 0;
      KeEnterCriticalRegion();
      ExAcquireResourceSharedLite(p_rLock, 1u);
      Flink = Flink->Flink;
      FltObjectDereference(lpVolume);
    }
    else
    {
      FltObjectDereference(&Flink[-1]);
      Flink = Flink->Flink;
    }
  }
  ExReleaseResourceLite(p_rLock);
  KeLeaveCriticalRegion();
  return (unsigned int)v3;
}
```

After loads of debugging and staring at IDA I found a chain of function calls stemming from `FltStartFiltering` that led me to an api called `FltpSetCallbacksForInstance` which looked like a pretty good candidate for the function responsible for... you know... setting the callbacks for a `_FLT_INSTANCE`.  I'd found the first three functions in the call-chain in FltMgr correctly, but something was missing... So I set a breakpoint on  `FltpSetCallbacksForInstance` and reloaded my minifilter.

```
__int64 __fastcall FltpSetCallbacksForInstance(
        _FLT_INSTANCE *lpFilterInstance,
        __int64 lpCallbackNode,
        int dwCountCallbacks)
{
  Volume = lpFilterInstance->Volume;
  v6 = qword_1C002B920;
  Operations = lpFilterInstance->Filter->Operations;
  KeEnterGuardedRegion();
  v9 = ExAcquireCacheAwarePushLockSharedEx(v6, 0i64);
  
  // loop over the callback structure array for the filter
  // until we see our terminating element (IRP_MJ_OPERATION_END)
  
  while ( Operations->MajorFunction != 0x80 && dwCountCallbacks )
  {
    if ( (unsigned __int8)(Operations->MajorFunction + 20) > 1u
      && (Operations->PreOperation || Operations->PostOperation) )
    {

	 // !!! 
	 // THIS is where I discovered that the MajorFunction + 22 was
	 // the offset into the callback node array
	 // !!! 
	 
      byteIndex = Operations->MajorFunction + 22;
      if ( byteIndex < 0x32u )
      {
        if ( lpFilterInstance->CallbackNodes[v10] )
        {
          ExReleaseCacheAwarePushLockSharedEx(v9, 0i64);
          KeLeaveGuardedRegion();
          return 3223060493i64;
        }
        FltpInitializeCallbackNode(
          lpCallbackNode,
          (__int64)Operations,
          0i64,
          0i64,
          0i64,
          0i64,
          (__int64)lpFilterInstance,
          byteIndex);
          
        // increment pointer + sizeof(_CALLBACK_NODE)
        lpCallbackNode += 0x30i64;
        --dwCountCallbacks;
      }
    }
    ++Operations;
  }

  // <SNIP>
  // truncated for readability
  // <SNIP>

  if ( (lpFilterInstance->Base.Flags & 1) == 0 )
  {
    v17 = 0;
    OperationFlags = Volume->Callbacks.OperationFlags;
    CallbackNodes = lpFilterInstance->CallbackNodes;
    do
    {
      if ( *CallbackNodes )
      {
        FltpInsertCallback(lpFilterInstance, Volume, v17);
        *OperationFlags &= (*CallbackNodes)->Flags;
      }
      ++v17;
      ++CallbackNodes;
      ++OperationFlags;
    }
    while ( v17 < 0x32 );
  }
  
  // <SNIP>
  // truncated for readability
  // <SNIP>
  
  return 0i64;
}
```
Decompilation of `FltpSetCallbacksForInstance`

As expected, when the breakpoint hit, the call chain I thought I would see appeared right in front of my eyes. Almost like computers aren't boxes of magic powered by electricity!

```
kd> bp FLTMGR!FltpSetCallbacksForInstance
kd> g
Breakpoint 1 hit
FLTMGR!FltpSetCallbacksForInstance:
fffff805`0fc91aa4 48895c2408      mov     qword ptr [rsp+8],rbx
kd> k
 # Child-SP          RetAddr               Call Site
00 fffff38b`51f3d4b8 fffff805`0fcc92a1     FLTMGR!FltpSetCallbacksForInstance
01 fffff38b`51f3d4c0 fffff805`0fcc88f4     FLTMGR!FltpInitInstance+0x565
02 fffff38b`51f3d550 fffff805`0fcc86b3     FLTMGR!FltpCreateInstanceFromName+0x1e0
03 fffff38b`51f3d630 fffff805`0fcdd3a5     FLTMGR!FltpEnumerateRegistryInstances+0xe3
04 fffff38b`51f3d6c0 fffff805`0fcdd1cb     FLTMGR!FltpDoVolumeNotificationForNewFilter+0xa5
05 fffff38b`51f3d700 fffff805`0c1610fa     FLTMGR!FltStartFiltering+0x2b
06 fffff38b`51f3d740 fffff805`0c165020     DemoMinifilter!DriverEntry+0x5a [DemoMinifilter.c @ 78] 
07 fffff38b`51f3d780 fffff805`0d5cbf44     DemoMinifilter!GsDriverEntry+0x20 
08 fffff38b`51f3d7b0 fffff805`0d5cbc86     nt!PnpCallDriverEntry+0x4c
09 fffff38b`51f3d810 fffff805`0d5ca247     nt!IopLoadDriver+0x8ba
0a fffff38b`51f3d9c0 fffff805`0d13903f     nt!IopLoadUnloadDriver+0x57
0b fffff38b`51f3da00 fffff805`0d167d95     nt!ExpWorkerThread+0x14f
0c fffff38b`51f3dbf0 fffff805`0d21edd4     nt!PspSystemThreadStartup+0x55
0d fffff38b`51f3dc40 00000000`00000000     nt!KiStartSystemThread+0x34
```

As it turns out, the only xref I could find for this function was from within `FltpInitInstance`, so I felt like I was on the right track. Inspecting the pool for the first argument, I found that the value stored in `rcx` pointed to, as expected, an `_FLT_INSTANCE` structure for our newly-reloaded minifilter.

```
kd> !pool @rcx
Pool page ffffcb0e11386cb0 region is Nonpaged pool
 ffffcb0e11386000 size:  640 previous size:    0  (Allocated)  KDNF
 ffffcb0e11386650 size:  640 previous size:    0  (Allocated)  KDNF
*ffffcb0e11386ca0 size:  2b0 previous size:    0  (Allocated) *FMis
		Pooltag FMis : FLT_INSTANCE structure, Binary : fltmgr.sys
 ffffcb0e11386f50 size:   90 previous size:    0  (Free)       .t[|
kd> dt FLTMGR!_FLT_INSTANCE @rcx
   +0x000 Base             : _FLT_OBJECT
   +0x030 OperationRundownRef : 0xffffcb0e`0de1b970 _EX_RUNDOWN_REF_CACHE_AWARE
   +0x038 Volume           : 0xffffcb0e`0bb26750 _FLT_VOLUME
   +0x040 Filter           : 0xffffcb0e`11604cb0 _FLT_FILTER
   +0x048 Flags            : 4 ( INSFL_INITING )
   +0x050 Altitude         : _UNICODE_STRING "123456"
   +0x060 Name             : _UNICODE_STRING "AltitudeAndFlags"
   +0x070 FilterLink       : _LIST_ENTRY [ 0xffffcb0e`11604d80 - 0xffffcb0e`11604d80 ]
   +0x080 ContextLock      : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x090 Context          : (null) 
   +0x098 TransactionContexts : _CONTEXT_LIST_CTRL
   +0x0a0 TrackCompletionNodes : 0xffffcb0e`11af0580 _TRACK_COMPLETION_NODES
   +0x0a8 CallbackNodes    : [50] (null)
```

Using the ***PHENOMENAL*** Windbg plugin, ret-sync (https://github.com/bootleg/ret-sync), I continued tracing execution into `FltpSetCallbacksForInstance`. This plugin allowed me to synchronize my debugging session between Windbg and IDA, and was indispensible during my research.

Once the first argument type was found, I discovered that the second argument `rdx` was simply offset `0x238` from our `_FLT_INSTANCE`. Continuing debugging, I traced execution to `FltpInitializeCallbackNode`, and corrected the argument types in IDA to give the following decompilation:

```
__int64 __fastcall FltpInitializeCallbackNode(
        _CALLBACK_NODE *lpCallbackNode,
        _FLT_OPERATION_REGISTRATION *lpFilterOperations,
        _FLT_PREOP_CALLBACK_STATUS *a3,
        _FLT_PREOP_CALLBACK_STATUS *a4,
        _FLT_PREOP_CALLBACK_STATUS *a5,
        _FLT_POSTOP_CALLBACK_STATUS *a6,
        _FLT_INSTANCE *lpFilterInstance,
        unsigned int byteIndex)
{
  _CALLBACK_NODE_FLAGS v9; // eax
  unsigned int Flags; // edx
  __int64 result; // rax
  _FLT_PREOP_CALLBACK_STATUS *v12; // rax

  lpCallbackNode->Flags = 0;
  lpCallbackNode->Instance = lpFilterInstance;
  if ( lpFilterOperations )
  {
    lpCallbackNode->PreOperation = lpFilterOperations->PreOperation;
    lpCallbackNode->PostOperation = lpFilterOperations->PostOperation;
    v9 = 0;
    Flags = lpFilterOperations->Flags;
    if ( (Flags & 1) != 0 )
    {
      lpCallbackNode->Flags = CBNFL_SKIP_PAGING_IO;
      v9 = CBNFL_SKIP_PAGING_IO;
      Flags = lpFilterOperations->Flags;
    }
    if ( (Flags & 2) != 0 )
    {
      v9 |= 2u;
      lpCallbackNode->Flags = v9;
      Flags = lpFilterOperations->Flags;
    }
    if ( (Flags & 4) != 0 )
    {
      v9 |= 8u;
      lpCallbackNode->Flags = v9;
      Flags = lpFilterOperations->Flags;
    }
    if ( (Flags & 8) != 0 )
      lpCallbackNode->Flags = v9 | 0x10;
  }
  else if ( a3 )
  {
    lpCallbackNode->PreOperation = a3;
  }
  else
  {
    v12 = a5;
    if ( a5 )
    {
      lpCallbackNode->Flags = CBNFL_USE_NAME_CALLBACK_EX;
    }
    else
    {
      if ( !a4 )
        goto LABEL_10;
      v12 = a4;
    }
    lpCallbackNode->PreOperation = v12;
    lpCallbackNode->PostOperation = a6;
  }
LABEL_10:
  result = byteIndex;
  lpCallbackNode->CallbackLinks.Flink = 0i64;
  lpFilterInstance->CallbackNodes[byteIndex] = lpCallbackNode;
  return result;
}
```
Decompilation of `FltpInitializeCallbackNode`

Once the node completed initialization, I then returned back into `FltpSetCallbacksForInstance` where the next step was to insert the created callback node into the volume by the function `FltpInsertCallback`.

Breaking on `FltpInsertCallback`, I observed the referenced second argument of `_FLT_VOLUME`, and enumerated the callback table on function entry and return. But first I noted the address of my pre-create routine:
```
kd> x DemoMinifilter!PreCreateCallback
fffff805`0c161020 DemoMinifilter!PreCreateCallback (struct _FLT_CALLBACK_DATA *, struct _FLT_RELATED_OBJECTS *, void **)
```

I then inspected the volume passed in via `rdx`, and it's associated callback table. Examining the list of callbacks, since I know my filter is only registering IRP_MJ_CREATE, I only need to monitor the callbacks at index 22:
`(IRP_MJ_CREATE + 22) == 0 + 22 == 22`
```
kd> dt FLTMGR!_FLT_VOLUME @rdx
   +0x000 Base             : _FLT_OBJECT
   +0x030 Flags            : 0x164 (No matching name)
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
   +0x740 TargetedOpenActiveCount : 0n1175
   +0x748 TxVolContextListLock : _EX_PUSH_LOCK_AUTO_EXPAND
   +0x758 TxVolContexts    : _TREE_ROOT
   +0x760 SupportedFeatures : 0n12
   +0x764 BypassFailingFltNameLen : 0
   +0x766 BypassFailingFltName : [32]  ""

// getting the _CALLBACK_CTRL object
kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 (*((FLTMGR!_CALLBACK_CTRL *)0xffffcb0e0bc625a0))
(*((FLTMGR!_CALLBACK_CTRL *)0xffffcb0e0bc625a0))                 [Type: _CALLBACK_CTRL]
    [+0x000] OperationLists   [Type: _LIST_ENTRY [50]]
    [+0x320] OperationFlags   [Type: _CALLBACK_NODE_FLAGS [50]]

kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 (*((FLTMGR!_LIST_ENTRY (*)[50])0xffffcb0e0bc625a0))
(*((FLTMGR!_LIST_ENTRY (*)[50])0xffffcb0e0bc625a0))                 [Type: _LIST_ENTRY [50]]
    ... TRUNCATED
    [22]             [Type: _LIST_ENTRY] // list of create callbacks
    ... TRUNCATED
```

From there, I issued the `dl` command to walk the linked list of callbacks at the initial breakpoint, before our callback was inserted.

```
// walking the linked list before function return
kd> dx -id 0,0,ffffcb0e0b2eb040 -r1 (*((FLTMGR!_LIST_ENTRY *)0xffffcb0e0bc62700))
(*((FLTMGR!_LIST_ENTRY *)0xffffcb0e0bc62700))                 [Type: _LIST_ENTRY]
    [+0x000] Flink            : 0xffffcb0e0f1e0718 [Type: _LIST_ENTRY *]
    [+0x008] Blink            : 0xffffcb0e0bc69ad8 [Type: _LIST_ENTRY *]
kd> dl 0xffffcb0e0bc62700
ffffcb0e`0bc62700  ffffcb0e`0f1e0718 ffffcb0e`0bc69ad8 (_LIST_ENTRY)
ffffcb0e`0bc62710  ffffcb0e`0f1e0748 ffffcb0e`0b3935d8

ffffcb0e`0f1e0718  ffffcb0e`0bda3b18 ffffcb0e`0bc62700 (_LIST_ENTRY)
ffffcb0e`0f1e0728  ffffcb0e`0f1e04e0 fffff805`1c63d350

ffffcb0e`0bda3b18  ffffcb0e`0b3935a8 ffffcb0e`0f1e0718 (_LIST_ENTRY)
ffffcb0e`0bda3b28  ffffcb0e`0bda38b0 fffff805`10a77360

ffffcb0e`0b3935a8  ffffcb0e`0bc6bd58 ffffcb0e`0bda3b18 (_LIST_ENTRY)
ffffcb0e`0b3935b8  ffffcb0e`0b393010 fffff805`1c5a1460

ffffcb0e`0bc6bd58  ffffcb0e`0bc69ad8 ffffcb0e`0b3935a8 (_LIST_ENTRY)
ffffcb0e`0bc6bd68  ffffcb0e`0bc6bb20 fffff805`10a20010

ffffcb0e`0bc69ad8  ffffcb0e`0bc62700 ffffcb0e`0bc6bd58 (_LIST_ENTRY)
ffffcb0e`0bc69ae8  ffffcb0e`0bc698a0 fffff805`109eb4b0

```

Once enumerated, I continued execution until the function returned, and re-walked the linked list of callbacks and found the pre-create callback had successfully been inserted into the volume's callback table.

```

kd> pt
FLTMGR!FltpInsertCallback+0x44:
fffff805`0fc91de8 c3              ret
kd> dl 0xffffcb0e0bc62700
ffffcb0e`0bc62700  ffffcb0e`0f1e0718 ffffcb0e`0bc69ad8 (_LIST_ENTRY)
ffffcb0e`0bc62710  ffffcb0e`0f1e0748 ffffcb0e`0b3935d8

ffffcb0e`0f1e0718  ffffcb0e`0bda3b18 ffffcb0e`0bc62700 (_LIST_ENTRY)
ffffcb0e`0f1e0728  ffffcb0e`0f1e04e0 fffff805`1c63d350

ffffcb0e`0bda3b18  ffffcb0e`0b3935a8 ffffcb0e`0f1e0718 (_LIST_ENTRY)
ffffcb0e`0bda3b28  ffffcb0e`0bda38b0 fffff805`10a77360

ffffcb0e`0b3935a8  ffffcb0e`1214df68 ffffcb0e`0bda3b18 (_LIST_ENTRY)
ffffcb0e`0b3935b8  ffffcb0e`0b393010 fffff805`1c5a1460

ffffcb0e`1214df68  ffffcb0e`0bc6bd58 ffffcb0e`0b3935a8 (_LIST_ENTRY)
ffffcb0e`1214df78  ffffcb0e`1214dd30 fffff805`0c161020 // pre create routine inserted into volume callback node

ffffcb0e`0bc6bd58  ffffcb0e`0bc69ad8 ffffcb0e`1214df68 (_LIST_ENTRY)
ffffcb0e`0bc6bd68  ffffcb0e`0bc6bb20 fffff805`10a20010

ffffcb0e`0bc69ad8  ffffcb0e`0bc62700 ffffcb0e`0bc6bd58 (_LIST_ENTRY)
ffffcb0e`0bc69ae8  ffffcb0e`0bc698a0 fffff805`109eb4b0

```

With ALL of that out of the way, I then had a decent understanding of what I had to do to get at the callbacks I needed to patch:

1. Somehow find the frame
2. Find all the volumes in the frame
3. For every volume in the frame, find the `_CALLBACK_CTRL` object
4. Inside every `_CALLBACK_CTRL` object, index into it's list of  `_CALLBACK_NODE` lists with `MajorFunction+22` as the index
5. Inside that list, compare the `_CALLBACK_NODE` pre and post operations and, if they match our target minifilters callbacks, patch them.

There were two small problems, though:
1. How do I find the frame?
2. What the hell am I going to patch the callbacks with?

#### 1.3.1 How do I find the frame?
I started poking around various get/set/enumerate functions to see if there was somewhere I could find a reference to a frame or list of frames when I came across `FltEnumerateFilters`. Turns out, and through trial and error, I'd found a reference to a global variable called...  \*drum roll\*  `FLTMGR!FltGlobals` through an exported function.

```
kd> uf FLTMGR!FltEnumerateFilters
FLTMGR!FltEnumerateFilters:
fffff805`0fce5a60 488bc4          mov     rax,rsp
fffff805`0fce5a63 48895808        mov     qword ptr [rax+8],rbx
fffff805`0fce5a67 48896810        mov     qword ptr [rax+10h],rbp
fffff805`0fce5a6b 48897020        mov     qword ptr [rax+20h],rsi
fffff805`0fce5a6f 4c894018        mov     qword ptr [rax+18h],r8
fffff805`0fce5a73 57              push    rdi
fffff805`0fce5a74 4154            push    r12
fffff805`0fce5a76 4155            push    r13
fffff805`0fce5a78 4156            push    r14
fffff805`0fce5a7a 4157            push    r15
fffff805`0fce5a7c 4883ec20        sub     rsp,20h
fffff805`0fce5a80 33db            xor     ebx,ebx
fffff805`0fce5a82 8bea            mov     ebp,edx
fffff805`0fce5a84 8bfb            mov     edi,ebx
fffff805`0fce5a86 4d8bf0          mov     r14,r8
fffff805`0fce5a89 488bf1          mov     rsi,rcx
fffff805`0fce5a8c 4c8b15a5c6fdff  mov     r10,qword ptr [FLTMGR!_imp_KeEnterCriticalRegion (fffff805`0fcc2138)]
fffff805`0fce5a93 e858e23dfd      call    nt!KeEnterCriticalRegion (fffff805`0d0c3cf0)
fffff805`0fce5a98 b201            mov     dl,1

// ding ding ding
fffff805`0fce5a9a 488d0d775cfdff  lea     rcx,[FLTMGR!FltGlobals+0x58 (fffff805`0fcbb718)]
// ding ding ding

fffff805`0fce5aa1 4c8b1588c6fdff  mov     r10,qword ptr [FLTMGR!_imp_ExAcquireResourceSharedLite (fffff805`0fcc2130)]
fffff805`0fce5aa8 e803103dfd      call    nt!ExAcquireResourceSharedLite (fffff805`0d0b6ab0)
fffff805`0fce5aad 4c8b3dcc5cfdff  mov     r15,qword ptr [FLTMGR!FltGlobals+0xc0 (fffff805`0fcbb780)]
fffff805`0fce5ab4 488d05c55cfdff  lea     rax,[FLTMGR!FltGlobals+0xc0 (fffff805`0fcbb780)]
fffff805`0fce5abb 4c3bf8          cmp     r15,rax
fffff805`0fce5abe 747f            je      FLTMGR!FltEnumerateFilters+0xdf (fffff805`0fce5b3f)  Branch
```

`FltGlobals` unsurprisingly has the type `FLTMGR!_GLOBALS`.

```
kd> dt FLTMGR!_GLOBALS
   +0x000 DebugFlags       : Uint4B
   +0x008 TraceFlags       : Uint8B
   +0x010 GFlags           : Uint4B
   +0x018 RegHandle        : Uint8B
   +0x020 NumProcessors    : Uint4B
   +0x024 CacheLineSize    : Uint4B
   +0x028 AlignedInstanceTrackingListSize : Uint4B
   +0x030 ControlDeviceObject : Ptr64 _DEVICE_OBJECT
   +0x038 DriverObject     : Ptr64 _DRIVER_OBJECT
   +0x040 KtmTransactionManagerHandle : Ptr64 Void
   +0x048 TxVolKtmResourceManagerHandle : Ptr64 Void
   +0x050 TxVolKtmResourceManager : Ptr64 _KRESOURCEMANAGER
   +0x058 FrameList        : _FLT_RESOURCE_LIST_HEAD
   +0x0d8 Phase2InitLock   : _FAST_MUTEX
   +0x110 RegistryPath     : _UNICODE_STRING
   +0x120 RegistryPathBuffer : [160] Wchar
   +0x260 GlobalVolumeOperationLock : Ptr64 _EX_PUSH_LOCK_CACHE_AWARE_LEGACY
   +0x268 FltpServerPortObjectType : Ptr64 _OBJECT_TYPE
   +0x270 FltpCommunicationPortObjectType : Ptr64 _OBJECT_TYPE
   +0x278 MsgDeviceObject  : Ptr64 _DEVICE_OBJECT
   +0x280 ManualDeviceAttachTimer : Ptr64 _EX_TIMER
   +0x288 ManualDeviceAttachWork : _WORK_QUEUE_ITEM
   +0x2a8 ManualDeviceAttachLimit : Int4B
   +0x2ac ManualAttachDelayCounter : Int4B
   +0x2b0 FastManualAttachTimerPeriod : Uint4B
   +0x2b4 ManualAttachTimerPeriod : Uint4B
   +0x2b8 ManualAttachDelay : Uint4B
   +0x2bc ManualAttachIgnoredDevices : UChar
   +0x2bd ManualAttachOnlyOnceDevices : UChar
   +0x2be ManualAttachFastAttachDevices : UChar
   +0x2c0 CallbackStackSwapThreshold : Uint4B
   +0x300 TargetedIoCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x380 IoDeviceHintLookasideList : _PAGED_LOOKASIDE_LIST
   +0x400 StreamListCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x480 FileListCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x500 NameCacheCreateCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x580 AsyncIoContextLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x600 WorkItemLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x680 NameControlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x700 OperationStatusCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x780 NameGenerationContextLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x800 FileLockLookasideList : _PAGED_LOOKASIDE_LIST
   +0x880 TxnParameterBlockLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x900 TxCtxExtensionNPagedLookasideList : _NPAGED_LOOKASIDE_LIST
   +0x980 TxVolCtxLookasideList : _NPAGED_LOOKASIDE_LIST
   +0xa00 TxVolStreamListCtrlEntryLookasideList : _PAGED_LOOKASIDE_LIST
   +0xa80 SectionListCtrlLookasideList : _NPAGED_LOOKASIDE_LIST
   +0xb00 SectionCtxExtensionLookasideList : _NPAGED_LOOKASIDE_LIST
   +0xb80 OpenReparseListLookasideList : _PAGED_LOOKASIDE_LIST
   +0xc00 OpenReparseListEntryLookasideList : _PAGED_LOOKASIDE_LIST
   +0xc80 QueryOnCreateLookasideList : _PAGED_LOOKASIDE_LIST
   +0xd00 NameBufferLookasideList : _PAGED_LOOKASIDE_LIST
   +0xd80 NameCacheNodeLookasideLists : [7] _PAGED_LOOKASIDE_LIST
   +0x1100 FltpParameterOffsetTable : [28] <unnamed-tag>
   +0x11e0 ThrottledWorkCtrl : _THROTTLED_WORK_ITEM_CTRL
   +0x1230 LostItemDelayInSeconds : Uint4B
   +0x1238 VerifiedFiltersList : _LIST_ENTRY
   +0x1248 VerifiedFiltersLock : Uint8B
   +0x1250 VerifiedResourceLinkFailures : Int4B
   +0x1254 VerifiedResourceUnlinkFailures : Int4B
   +0x1258 PerfTraceRoutines : Ptr64 _WMI_FLTIO_NOTIFY_ROUTINES
   +0x1260 DummyPerfTraceRoutines : _WMI_FLTIO_NOTIFY_ROUTINES
   +0x1290 RenameCounter    : _LARGE_INTEGER
   +0x1298 FilterSupportedFeaturesMode : Int4B
   +0x12a0 InitialRundownSize : Uint8B
```

To find the list of frames, I must load access the `_FLT_RESOURCE_LIST_HEAD` for the frame list, and iterate over every element (one per frame). Once the frames were found, iterating over every frame to find every volume/instance would follow the exact same process. Perfect! 

But that just leaves me with the second question...

#### 1.3.2 What the hell am I going to patch the callbacks with?
This one was easy.
For pre-operation callbacks, the following return values indicate statuses back to FltMgr:

| Status | Value | Description |
| --- | --- | --- |
| FLT_PREOP_SUCCESS_WITH_CALLBACK | 0 | The callback was successful. Pass on the IO request and get a post-operation callback after completion. |
| FLT_PREOP_SUCCESS_NO_CALLBACK | 1 | The callback was successful. Pass on the IO request. No callback required. |
| FLT_PREOP_PENDING | 2 | Mark the IO operation as pending. |
| FLT_PREOP_DISALLOW_FASTIO | 3 | If handling a Fast IO operation, fail it to force the operation as a normal IO Request. |
| FLT_PREOP_COMPLETE | 4 | The operation has been completed. Do not pass on the IO request to any other drivers, even other filters in the stack. |
| FLT_PREOP_SYNCHRONIZE | 5 | Synchronize the post-operation callback in the same thread. |
| FLT_PREOP_DISALLOW_FSFILTER_IO | 6 | Disallow FastIO file creation. |
https://googleprojectzero.blogspot.com/2021/01/hunting-for-bugs-in-windows-mini-filter.html

So to patch out the pre-operation I needed to either return 1 or 4.
Searching for gadgets within Ntoskrnl led me to `KeIsEmptyAffinityEx`:

```
kd> uf nt!KeIsEmptyAffinityEx
nt!KeIsEmptyAffinityEx:
fffff805`0d00f060 440fb701        movzx   r8d,word ptr [rcx]
fffff805`0d00f064 33c0            xor     eax,eax
fffff805`0d00f066 66413bc0        cmp     ax,r8w
fffff805`0d00f06a 7318            jae     nt!KeIsEmptyAffinityEx+0x24 (fffff805`0d00f084)  Branch

nt!KeIsEmptyAffinityEx+0xc:
fffff805`0d00f06c 0f1f4000        nop     dword ptr [rax]

nt!KeIsEmptyAffinityEx+0x10:
fffff805`0d00f070 0fb7d0          movzx   edx,ax
fffff805`0d00f073 48837cd10800    cmp     qword ptr [rcx+rdx*8+8],0
fffff805`0d00f079 7510            jne     nt!KeIsEmptyAffinityEx+0x2b (fffff805`0d00f08b)  Branch

nt!KeIsEmptyAffinityEx+0x1b:
fffff805`0d00f07b 66ffc0          inc     ax
fffff805`0d00f07e 66413bc0        cmp     ax,r8w
fffff805`0d00f082 72ec            jb      nt!KeIsEmptyAffinityEx+0x10 (fffff805`0d00f070)  Branch

// gadget to return 1
nt!KeIsEmptyAffinityEx+0x24:
fffff805`0d00f084 b801000000      mov     eax,1
fffff805`0d00f089 c3              ret

// gadget to return 0
nt!KeIsEmptyAffinityEx+0x2b:
fffff805`0d00f08b 33c0            xor     eax,eax
fffff805`0d00f08d c3              ret
```

So all I had to do was patch the pre-operation callback to `nt!KeIsEmptyAffinityEx+0x24` to always return `FLT_PREOP_SUCCESS_WITH_CALLBACK`.

For post-operation callbacks, the process is identical, and the following return values indicate statuses back to FltMgr:
| Status | Value | Description |
| --- | --- | --- |
| FLT_POSTOP_FINISHED_PROCESSING | 0 | The callback was successful. No further processing required. |
| FLT_POSTOP_MORE_PROCESSING_REQUIRED | 1 | Halts completion of the IO request. The operation will be pending until the filter driver completes it. |
| FLT_POSTOP_DISALLOW_FSFILTER_IO | 2 | Disallow FastIO file creation.|

Luckily I could reuse the same function for my gadget to patch the callback with at `nt!KeIsEmptyAffinityEx+0x2b` to always return `FLT_POSTOP_FINISHED_PROCESSING`.

An astute reader may note that this would be easy to forensically identify as the callback patch destination is outside of the module's address space. You're absolutely right (thank you for pointing this out to me, qkumba <3). Not to worry, as this is only a PoC and I can hand-wave this concern away by just yelling "PoC" repeatedly.

With all of that done and dusted, we're left with simple steps to patch minifilter callbacks on a system using a virtual read/write primitive.

1. Find the base address of FltMgr
2. Find the base address of our target minifilter to patch
3. Find FltGlobals
4. Find the list of frames
5. For every frame in the list of frames:
	1. walk the filter list until we find our target filter
	2. read all of our target filter's `_FLT_OPERATION_REGISTRATION` objects
	3. walk the volumes attached to the frame
4. For every volume in the target frame:
	1. Access the `_CALLBACK_CTRL` object
	2. For every callback we want to patch:
		1. Index into `_CALLBACK_CTRL->_LIST_ENTRY[50]` with the callbacks major function to get the list of callbacks supported for that major function
		2. For every element in the list of `_CALLBACK_NODE` objects:
			1. Compare our pre/post operations and patch them if they match
