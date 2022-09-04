#pragma once
#include <fltKernel.h>
#include <ntddk.h>

FLT_PREOP_CALLBACK_STATUS PreCreateCallback(_Inout_ PFLT_CALLBACK_DATA lpFltCallbackData, _In_ PCFLT_RELATED_OBJECTS lpFltRelatedObj, _Out_ PVOID* lpCompletionContext);
