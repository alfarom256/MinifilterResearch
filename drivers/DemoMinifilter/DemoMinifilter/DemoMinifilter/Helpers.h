#pragma once
#include <fltKernel.h>
#include <ntddk.h>

PVOID FindFltGlobals();
PVOID PatternSearch(PVOID pBegin, SIZE_T szMaxSearch, PUCHAR searchBytes, PUCHAR searchMask, SIZE_T szSearchBytes);