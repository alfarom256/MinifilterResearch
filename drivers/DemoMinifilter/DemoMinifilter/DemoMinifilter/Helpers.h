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