#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include "../DemoMinifilter/FltDef.h"
#include "PebLdr.h"

class FltManager
{
public:
	FltManager();
	~FltManager();
	PVOID lpFltMgrBase = { 0 };
	PVOID lpFltGlobals = { 0 };
	PVOID lpFltFrame0 = { 0 };
	PFLT_FILTER GetFilterByName(const wchar_t* strFilterName);
	BOOL GetFilterOperationByMajorFn(PFLT_FILTER lpFilter, DWORD MajorFunction);
	DWORD GetFrameCount();

private:

	PVOID ResolveFltmgrBase();
	PVOID ResolveFltmgrGlobals(LPVOID lpkFltMgrBase);

};



