#include "FltUtil.h"

FltManager::FltManager() : lpFltMgrBase(ResolveFltmgrBase()), lpFltGlobals(ResolveFltmgrGlobals(lpFltMgrBase)), lpFltFrame0(0) {}


FltManager::~FltManager()
{
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

inline PVOID FltManager::ResolveFltmgrGlobals(LPVOID lpkFltMgrBase)
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
