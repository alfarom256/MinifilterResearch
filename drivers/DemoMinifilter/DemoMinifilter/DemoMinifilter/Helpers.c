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
				return pBeginIter + szIdx - (i + 1);
			}
		}
	}
    return NULL;
}

PVOID FindFltGlobals() {
	PVOID lpFltGlobals = NULL;
	PUCHAR lpFltGlobalsData = NULL;
	PVOID lpFltEnumerateFilters = FltGetRoutineAddress("FltEnumerateFilters");
	DWORD32 dwOffset = 0;

	UCHAR ucharLoadFltGlobals[3] = {
		0x48, 0x8d, 0x0d, // lea rcx, cs:FltGlobals+0x58
	};

	UCHAR ucharLoadFltGlobalsMask[3] = {
		1, 1, 1,
	};

	if (!lpFltEnumerateFilters) {
		return NULL;
	}

	DbgPrint("Found FltEnumerateFilters at %p\nStarting pattern search in FltEnumerateFilters\n", lpFltEnumerateFilters);

	lpFltGlobalsData = PatternSearch(
		lpFltEnumerateFilters, 
		0x200, 
		(PUCHAR)&ucharLoadFltGlobals, 
		(PUCHAR)&ucharLoadFltGlobalsMask,
		sizeof(ucharLoadFltGlobals)
	);

	if (!lpFltGlobalsData) {
		DbgPrint("Failed to find byte pattern...\n");
		return NULL;
	}

	dwOffset = *(DWORD32*)(lpFltGlobalsData + 3);
	lpFltGlobals = (PVOID)(lpFltGlobalsData + sizeof(ucharLoadFltGlobals) + dwOffset);
	DbgPrint("FLTMGR!FltGlobals is at %p\n", lpFltGlobals);
	return lpFltGlobals;
}