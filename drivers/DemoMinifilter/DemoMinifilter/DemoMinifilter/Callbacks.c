#include "Callbacks.h"

_Use_decl_annotations_
FLT_PREOP_CALLBACK_STATUS PreCreateCallback(PFLT_CALLBACK_DATA lpFltCallbackData, PCFLT_RELATED_OBJECTS lpFltRelatedObj, PVOID* lpCompletionContext)
{

    UNREFERENCED_PARAMETER(lpFltCallbackData);
    *lpCompletionContext = NULL;
    PFILE_OBJECT lpFileObject = lpFltRelatedObj->FileObject;
    PUNICODE_STRING lpFileName = &lpFileObject->FileName;

    // desktop\test.txt
    if (RtlCompareUnicodeString(&g_TargetFileName, lpFileName, TRUE) == 0) {
        HANDLE hPid = PsGetCurrentProcessId();
        DbgPrint("[DEMOFLT] PID %p - Create - %wZ\n", hPid, lpFileName);
        //DbgBreakPoint();
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

_Use_decl_annotations_
FLT_PREOP_CALLBACK_STATUS PreCreateCallback2(PFLT_CALLBACK_DATA lpFltCallbackData, PCFLT_RELATED_OBJECTS lpFltRelatedObj, PVOID* lpCompletionContext)
{

    UNREFERENCED_PARAMETER(lpFltCallbackData);
    *lpCompletionContext = NULL;
    PFILE_OBJECT lpFileObject = lpFltRelatedObj->FileObject;
    PUNICODE_STRING lpFileName = &lpFileObject->FileName;

    if (RtlCompareUnicodeString(&g_TargetFileName, lpFileName, TRUE) == 0) {
        //sHANDLE hPid = PsGetCurrentProcessId();
        DbgPrint("[DEMOFLT] WEEWOOWEEWOO!!! SECOND CALLBACK WAS CALLED\n");
        //DbgBreakPoint();
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
