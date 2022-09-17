#pragma once
#include <fltKernel.h>
#include <ntddk.h>
#include "FltDef.h"

PVOID FindFltGlobals();
PFLTP_FRAME GetFrameFromGlobals(PVOID lpFltGlobals);
VOID DbgPrintAllFilters();
VOID WalkLinkedList(PFLTP_FRAME lpFltFrame);
VOID PrintOperationsForFilter(PFLT_FILTER lpFilter);
PVOID PatternSearch(PVOID pBegin, SIZE_T szMaxSearch, PUCHAR searchBytes, PUCHAR searchMask, SIZE_T szSearchBytes);
PVOID FindRet1();
PFLT_OPERATION_REGISTRATION QueryMinifilterMajorOperation(PUNICODE_STRING lpFilterName, ULONG MajorFunction);
PFLT_FILTER QueryMinifilter(PUNICODE_STRING lpFilterName);
PLIST_ENTRY GetFilterInstanceList(PUNICODE_STRING lpFilterName);

VOID BorkMinifilter(PUNICODE_STRING lpFilterName);