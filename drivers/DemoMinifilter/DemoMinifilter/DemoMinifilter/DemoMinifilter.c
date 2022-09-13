#include <fltKernel.h>
#include <ntddk.h>
#include "Callbacks.h"
#include "Helpers.h"
#include "FltDef.h"

EXTERN_C NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT pDrvObj, _In_ PUNICODE_STRING pRegPath);
NTSTATUS FsFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

PFLT_FILTER g_FilterHandle;

const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{
		IRP_MJ_CREATE,
		0,
		PreCreateCallback,
		NULL,
	},
	{ IRP_MJ_OPERATION_END }
};

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/ns-fltkernel-_flt_registration
const FLT_REGISTRATION FltRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	NULL,
	Callbacks,
	FsFilterUnload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

NTSTATUS FsFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags) {
	UNREFERENCED_PARAMETER(Flags);
	FltUnregisterFilter(g_FilterHandle);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT pDrvObj, _In_ PUNICODE_STRING pRegPath) {
	UNREFERENCED_PARAMETER(pDrvObj);
	UNREFERENCED_PARAMETER(pRegPath);
	NTSTATUS status = 0;

	status = FltRegisterFilter(pDrvObj, &FltRegistration, &g_FilterHandle);
	if (!NT_SUCCESS(status)) {
		FltUnregisterFilter(g_FilterHandle);
		return status;
	}

	FltStartFiltering(g_FilterHandle);
	
	/*status = PsCreateSystemThread(&hSystemThread, 0, NULL, (HANDLE)-1, &createdClientId, DbgPrintAllFilters, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[DriverEntry] Failed to create system thread");
		return status;
	}*/

	DbgPrintAllFilters();
	return status;
}