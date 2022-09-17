#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include "../DemoMinifilter/FltDef.h"
#include "PebLdr.h"

PVOID ResolveFltmgrBase();
PVOID ResolveFltmgrGlobals(LPVOID lpkFltMgrBase);
PFLT_FILTER GetFilterByName(const wchar_t* strFilterName);
BOOL GetFilterOperationByMajorFn(PFLT_FILTER lpFilter, DWORD MajorFunction);
DWORD GetFrameCount();