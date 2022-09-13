#include "Helpers.h"

PVOID PatternSearch(PVOID pBegin, SIZE_T szMaxSearch, PUCHAR searchBytes, PUCHAR searchMask, SIZE_T szSearchBytes)
{
	PUCHAR pBeginIter = (PUCHAR)pBegin;
	for (SIZE_T szIdx = 0 ; *(PUSHORT)pBeginIter != 0xc3cc && szIdx < szMaxSearch; szIdx++) {
		for (SIZE_T i = 0; i < szSearchBytes; i++) {
			if (searchMask[i] == 0) {
				continue;
			} else if (pBeginIter[szIdx + i] != searchBytes[i]) {
				break;
			} else if (i + 1 == szSearchBytes) {
				return pBeginIter + szIdx;
			}
		}
	}
    return NULL;
}

PVOID FindFltGlobals() {
	PVOID lpFltGlobals = NULL;
	PUCHAR lpFltGlobalsData = NULL;
	PVOID lpFltEnumerateFilters = FltGetRoutineAddress("FltEnumerateFilters");
	INT32 dwOffset = 0;

	UCHAR ucharLoadFltGlobals[3] = {
		0x48, 0x8d, 0x0d, // lea rcx, cs:FltGlobals+0x58
	};

	UCHAR ucharLoadFltGlobalsMask[3] = {
		1, 1, 1,
	};

	if (!lpFltEnumerateFilters) {
		return NULL;
	}

	DbgPrint("[FindFltGlobals] Found FltEnumerateFilters at %p\nStarting pattern search in FltEnumerateFilters\n", lpFltEnumerateFilters);

	lpFltGlobalsData = PatternSearch(
		lpFltEnumerateFilters, 
		0x200, 
		(PUCHAR)&ucharLoadFltGlobals, 
		(PUCHAR)&ucharLoadFltGlobalsMask,
		sizeof(ucharLoadFltGlobals)
	);

	if (!lpFltGlobalsData) {
		DbgPrint("[FindFltGlobals] Failed to find byte pattern...\n");
		return NULL;
	}

	dwOffset = *(INT32*)(lpFltGlobalsData + 3);

	/*
		uf FLTMGR!FltEnumerateFilters


		...
		fffff80d`769e4e83 0f1f440000      nop     dword ptr [rax+rax]
		fffff80d`769e4e88 b201            mov     dl,1
		fffff80d`769e4e8a 488d0d0757fdff  lea     rcx,[FLTMGR!FltGlobals+0x58 (fffff80d`769ba598)] // <------- this
		fffff80d`769e4e91 48ff15a0c2fdff  call    qword ptr [FLTMGR!_imp_ExAcquireResourceSharedLite (fffff80d`769c1138)]
		fffff80d`769e4e98 0f1f440000      nop     dword ptr [rax+rax]
		...

		calculation is from rip relative lea
		we should be getting FltGlobals+0x58 which is why 0x58 is subtracted
	*/

	lpFltGlobals = (PVOID)(lpFltGlobalsData + 7 + dwOffset - 0x58);
	DbgPrint("[FindFltGlobals] FLTMGR!FltGlobals is at %p\n", lpFltGlobals);
	return lpFltGlobals;
}

PFLTP_FRAME GetFrameFromGlobals(PVOID lpFltGlobals)
{
	if (!lpFltGlobals) {
		return NULL;
	}
	return (PFLTP_FRAME)((SIZE_T)(*(PVOID*)((SIZE_T)lpFltGlobals + 0xc8)) - 8);
}

VOID DbgPrintAllFilters()
{
	PVOID lpFltGlobals = FindFltGlobals();
	if (!lpFltGlobals) {
		return;
	}

	PFLTP_FRAME lpFltFrame = GetFrameFromGlobals(lpFltGlobals);
	WalkLinkedList(lpFltFrame);
}

VOID WalkLinkedList(PFLTP_FRAME lpFltFrame)
{
	ULONG ulCount = lpFltFrame->RegisteredFilters.rCount;
	PLIST_ENTRY listHead = lpFltFrame->RegisteredFilters.rList.Flink;
	PLIST_ENTRY listIter = listHead;

	for (ULONG i = 0; i < ulCount; i++) {
		PFLT_FILTER lpFilter = (PFLT_FILTER)((SIZE_T)listIter - 0x10);
		DbgPrint("[WalkLinkedList] Found filter at - %p\n", lpFilter);
		DbgPrint("\tFilter Name - %wZ\n", &lpFilter->Name);
		DbgPrint("\tFilter Altitude - %wZ\n", &lpFilter->DefaultAltitude);
		PrintOperationsForFilter(lpFilter);
		listIter = listIter->Flink;
	}
}

VOID PrintOperationsForFilter(PFLT_FILTER lpFilter)
{
	UNREFERENCED_PARAMETER(lpFilter);
	FLT_OPERATION_REGISTRATION* Callbacks = lpFilter->Operations;
	
	if (Callbacks == NULL) {
		DbgPrint("\tCallbacks is NULL!\n");
		return;
	}

	while (Callbacks->MajorFunction != IRP_MJ_OPERATION_END) {
		DbgPrint("\tMajorFunction: 0x%x\n", Callbacks->MajorFunction);
		if(Callbacks->PreOperation)
			DbgPrint("\t\tPre Operation: %p\n", Callbacks->PreOperation);
		if (Callbacks->PreOperation)
			DbgPrint("\t\tPost Operation: %p\n", Callbacks->PostOperation);
		DbgPrint("\t\tFlags: 0x%x\n", Callbacks->Flags);
		Callbacks++;
	}
}
