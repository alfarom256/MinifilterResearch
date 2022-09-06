#pragma once
#include <fltKernel.h>
#include <ntddk.h>

static const UNICODE_STRING g_TargetFileName = RTL_CONSTANT_STRING(L"\\Users\\Student\\Desktop\\test.txt");
FLT_PREOP_CALLBACK_STATUS PreCreateCallback(_Inout_ PFLT_CALLBACK_DATA lpFltCallbackData, _In_ PCFLT_RELATED_OBJECTS lpFltRelatedObj, _Out_ PVOID* lpCompletionContext);
