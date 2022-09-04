#include "Callbacks.h"

_Use_decl_annotations_
FLT_PREOP_CALLBACK_STATUS PreCreateCallback(PFLT_CALLBACK_DATA lpFltCallbackData, PCFLT_RELATED_OBJECTS lpFltRelatedObj, PVOID* lpCompletionContext)
{
    UNREFERENCED_PARAMETER(lpFltCallbackData);
    *lpCompletionContext = NULL;
    PFILE_OBJECT lpFileObject = lpFltRelatedObj->FileObject;
    HANDLE hPid = PsGetCurrentProcessId();
    PUNICODE_STRING lpFileName = &lpFileObject->FileName;

    DbgPrint("[DEMOFLT] PID %p - Create - %wZ\n", hPid, lpFileName);

    return FLT_PREOP_COMPLETE;
}
