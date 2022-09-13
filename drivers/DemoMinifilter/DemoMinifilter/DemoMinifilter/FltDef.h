#pragma once
#include <ntddk.h>



typedef struct _FLT_RESOURCE_LIST_HEAD {
	ERESOURCE rLock;
	LIST_ENTRY rList;
	ULONG rCount;
} FLT_RESOURCE_LIST_HEAD, *PFLT_RESOURCE_LIST_HEAD;

typedef struct _FLT_MUTEX_LIST_HEAD {
	FAST_MUTEX mLock;
	LIST_ENTRY mList;
	ULONG mCount;
	// mInvalid is the 0th bit of mCount
}FLT_MUTEX_LIST_HEAD, *PFLT_MUTEX_LIST_HEAD;

typedef struct _FLTPP_LOOKASIDE_LIST {
	NPAGED_LOOKASIDE_LIST* P;
	NPAGED_LOOKASIDE_LIST* L;
}FLTPP_LOOKASIDE_LIST, *PFLTPP_LOOKASIDE_LIST;

typedef struct _FLT_PRCB {
	FLTPP_LOOKASIDE_LIST PPIrpCtrlLookasideLists[2];
}FLT_PRCB, *PFLT_PRCB;

typedef struct _FLTP_IRPCTRL_STACK_PROFILER {
	PVOID Frame;
	ULONG Profile[10];
	KTIMER timer;
	KDPC Dpc;
	WORK_QUEUE_ITEM WorkItem;
	FAST_MUTEX Mutex;
	ULONG WorkItemFlags;
	ULONG Flags;
	ULONG AllocCount;
}FLTP_IRPCTRL_STACK_PROFILER, * PFLTP_IRPCTRL_STACK_PROFILER;

typedef struct _FLTP_FRAME {
	DWORD64 type;
	LIST_ENTRY Links;
	ULONG FrameId;
	UNICODE_STRING AltitudeIntervalLow;
	UNICODE_STRING AltitudeIntervalHigh;
	UCHAR LargeIrpCtrlStackSize;
	UCHAR SmallIrpCtrlStackSize;
	FLT_RESOURCE_LIST_HEAD RegisteredFilters;
	FLT_RESOURCE_LIST_HEAD AttachedVolumes;
	LIST_ENTRY MountingVolumes;
	FLT_MUTEX_LIST_HEAD AttachedFileSystems;
	FLT_MUTEX_LIST_HEAD ZombiedFltObjectContexts;
	PVOID64 KtmResourceManagerHandle;
	PKRESOURCEMANAGER KtmResourceManager;
	ERESOURCE FilterUnloadLock;
	FAST_MUTEX DeviceObjectAttachLock;
	PFLT_PRCB Prcb;
	PVOID PrcbPoolToFree;
	PVOID LookasidePoolToFree;
	FLTP_IRPCTRL_STACK_PROFILER IrpCtrlStackProfiler;
	NPAGED_LOOKASIDE_LIST SmallIrpCtrlLookasideList;
	NPAGED_LOOKASIDE_LIST LargeIrpCtrlLookasideList;
	PVOID ReserveIrpCtrls; // fuck that define it yourself
} FLTP_FRAME, *PFLTP_FRAME;

/*
0: kd> dt FLTMGR!_FLTP_FRAME
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
*/

typedef struct _FLT_OBJECT {
	ULONG Flags;
	ULONG PointerCount;
	EX_RUNDOWN_REF RundownRef;
	LIST_ENTRY PrimaryLink;
	GUID UniqueIdenfitier;
} FLT_OBJECT, *PFLT_OBJECT;


typedef struct _FLT_FILTER {
	FLT_OBJECT Base;
	PVOID Frame;
	UNICODE_STRING Name;
	UNICODE_STRING DefaultAltitude;
	DWORD64 Flags;
	PDRIVER_OBJECT DriverObject;
	FLT_RESOURCE_LIST_HEAD InstanceList;
	PVOID VerifierExtension;
	LIST_ENTRY VerifiedFiltersLink;
	PULONG FilterUnload;
	PULONG InstanceSetup;
	PULONG InstanceQueryTeardown;
	PVOID InstanceTeardownStart;
	PVOID InstanceTeardownComplete;
	PVOID SupportedContextsListHead;
	PVOID SupportedContexts[7];
	PVOID PreVolumeMount;
	PVOID PostVolumeMount;
	PVOID GenerateFileName;
	PVOID NormalizeNameComponent;
	PVOID NormalizeNameComponentEx;
	PVOID NormalizeContextCleanup;
	PVOID KtmNotification;
	PVOID SectionNotification;
	PFLT_OPERATION_REGISTRATION Operations;
	PVOID OldDriverUnload;
	FLT_MUTEX_LIST_HEAD ActiveOpens;
	FLT_MUTEX_LIST_HEAD ConnectionList;
	FLT_MUTEX_LIST_HEAD PortList;
	EX_PUSH_LOCK PortLock;
} FLT_FILTER, *PFLT_FILTER;