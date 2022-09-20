#include <Windows.h>
#include "memory.h"
#include "FltUtil.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		puts("Useage: dell_fsutil.exe <FILTER_NAME>");
		return -1;
	}
	char* strFilterName = argv[1];
	wchar_t* wstrFilterName = new wchar_t[strlen(strFilterName) + 2];
	size_t numConv = 0;
	mbstowcs_s(&numConv, wstrFilterName, strlen(strFilterName) + 2,strFilterName, strlen(strFilterName));
	printf("Enumerating for filter %S\n", wstrFilterName);

	Memory m = Memory();
	FltManager oFlt = FltManager(&m);
	DWORD dwX = oFlt.GetFrameCount();
	printf("Flt globals is at %p\n", oFlt.lpFltGlobals);
	printf("%d frames available\n", dwX);
	PVOID lpFilter = oFlt.GetFilterByName(wstrFilterName);
	auto x = oFlt.GetOperationsForFilter(lpFilter);
	for (auto a : x) {
		const char* strOperation = g_IrpMjMap.count((BYTE)a.MajorFunction) ?  g_IrpMjMap[(BYTE)a.MajorFunction] : "IRP_MJ_UNDEFINED";
		printf("MajorFn: %s\nPre: %p\nPost %p\n", strOperation, a.PreOperation, a.PostOperation);
	}
	return 0;
}