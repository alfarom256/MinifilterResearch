#pragma once
#include <fltKernel.h>
#include <ntddk.h>
#include "FltDef.h"

PVOID FindFltGlobals();
PVOID DbgPrintAllFilters();
PVOID PatternSearch(PVOID pBegin, SIZE_T szMaxSearch, PUCHAR searchBytes, PUCHAR searchMask, SIZE_T szSearchBytes);