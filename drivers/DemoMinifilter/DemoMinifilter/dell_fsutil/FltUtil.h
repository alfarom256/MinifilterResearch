#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <vector>
#include "../DemoMinifilter/FltDef.h"
#include "PebLdr.h"
#include "MemHandler.h"

#define FLTGLB_OFFSET_FLT_RESOURCE_LISTHEAD 0x58
#define FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_LIST 0x68
#define FLT_RESOURCE_LISTHEAD_OFFSET_FRAME_COUNT 0x78

#define FLT_FRAME_OFFSET_FILTER_RESOUCE_LISTHEAD 0x48
#define FILTER_RESOUCE_LISTHEAD_OFFSET_COUNT 0x78
#define FILTER_RESOUCE_LISTHEAD_OFFSET_FILTER_LISTHEAD 0x68

#define FILTER_OFFSET_NAME 0x38
#define FILTER_OFFSET_OPERATIONS 0x1a8

#define UNISTR_OFFSET_LEN 0
#define UNISTR_OFFSET_BUF 8


class FltManager
{
public:
	FltManager(MemHandler* objMemHandler);
	~FltManager();
	PVOID lpFltMgrBase = { 0 };
	PVOID lpFltGlobals = { 0 };
	PVOID lpFltFrameList = { 0 };
	PVOID GetFilterByName(const wchar_t* strFilterName);
	BOOL GetFilterOperationByMajorFn(PVOID lpFilter, DWORD MajorFunction);
	std::vector<FLT_OPERATION_REGISTRATION> GetOperationsForFilter(PFLT_FILTER lpFilter);
	DWORD GetFrameCount();

private:
	ULONG ulNumFrames;
	PVOID ResolveFltmgrBase();
	PVOID ResolveFltmgrGlobals(LPVOID lpkFltMgrBase);
	MemHandler* objMemHandler;

};
