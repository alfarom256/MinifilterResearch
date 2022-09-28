## 0.0 - Intro
### 0.1 Abstract
This research project served to help me learn more about file system minifilter drivers and how a malicious actor may leverage a vulnerable driver to patch callbacks for minifilters. Along the way, I'd discovered previous research by Aviad Shamriz which helped me immensely in my endeavor.

https://aviadshamriz.medium.com/part-1-fs-minifilter-hooking-7e743b042a9d

As this article goes very in-depth into the mechanics of file system minifilter hooking with another loaded driver, I will focus on my research methods which led me to develop a PoC leveraging Dell's vulnerable "dbutil" driver to perform the same actions from user-mode, and some things I learned along the way.

Alongside this I will present (mildly redacted) technical details of exploits I've previously found in production anti-malware products.

### 0.2 Acknowledgements
Thank you to Avid and MZakocs for your work which helped make this possible.
https://aviadshamriz.medium.com/part-1-fs-minifilter-hooking-7e743b042a9d
https://github.com/mzakocs/CVE-2021-21551-POC

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

#### FltRegisterFilter
The `FltRegisterFilter` function is the API used by a minifilter to register with FltMgr.

```
NTSTATUS FLTAPI FltRegisterFilter( 
	[in] PDRIVER_OBJECT Driver, 
	[in] const FLT_REGISTRATION *Registration, 
	[out] PFLT_FILTER *RetFilter 
);
```

A minifilter driver must provide a list of structures containing the I/O operations to inspect as well as a pre-operation and/or post-operation callback. 

#### FLT_REGISTRATION
The FLT_REGISTRATION structure defines the I/O request Major Function to filter, and defines a pre and post-operation callback which will be invoked, respectively, before or after the I/O operation is passed down to / back up from the I/O stack.

```
typedef struct _FLT_OPERATION_REGISTRATION {

    UCHAR MajorFunction;
    FLT_OPERATION_REGISTRATION_FLAGS Flags;
    PFLT_PRE_OPERATION_CALLBACK PreOperation;
    PFLT_POST_OPERATION_CALLBACK PostOperation;

    PVOID Reserved1;

} FLT_OPERATION_REGISTRATION, *PFLT_OPERATION_REGISTRATION;
```

The list of operations is terminated by a FLT_REGISTRATION structure whose Major Function is `IRP_MJ_OPERATION_END`.
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

#### Frame (`_FLTP_FRAME`)
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

Examining the `_FLTP_FRAME` object in Windows, we can se a clearer relationship between frames, filters, and volumes.
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

To help visualize their association, the following chart describes a high level overview of a frame on a system with a single-frame: 

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

As shown in the type definition and the above graph, a frame contains a reference to all filter objects (`_FLT_FILTER`) assocaited with the frame, alongside a list of volumes (`_FLT_VOLUME`).

Most importantly, this highlights an important aspect of the proof-of-concept:
* In order to access the proper objects to remove their associated callbacks we must first examine the frame to find the registered filters. We loop over every registered filter until we find the target filter and note the callbacks supported by the filter.
* From there we must iterate over each callback table assocaited with the volume and, when we find a target callback in the list, modify the entry as desired to replace the callback for our target filter.

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

#### Callback Node (`_CALLBACK_NODE`)


#### FltRegisterFilter

The first step to registering a file system minifilter is to, well, invoke the `FltRegisterFilter` method.

```
NTSTATUS FLTAPI FltRegisterFilter( 
	[in] PDRIVER_OBJECT Driver, 
	[in] const FLT_REGISTRATION *Registration, 
	[out] PFLT_FILTER *RetFilter 
);
```
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nf-fltkernel-fltregisterfilter






### 1.2 - Writing a minifilter
To aid in research and testing, I created a very simple mini filter driver to get the PID of any given process which inevitably calls `NtCreateFile` on the file `C:\Users\<USER>\test.txt`, and log it via `DbgPrint`.
