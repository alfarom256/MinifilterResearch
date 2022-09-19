#include <Windows.h>
#include "memory.h"
#include "FltUtil.h"

int main() {
	Memory m = Memory();
	FltManager oFlt = FltManager(&m);
	DWORD dwX = oFlt.GetFrameCount();
	printf("Flt globals is at %p\n", oFlt.lpFltGlobals);
	printf("%d frames available\n", dwX);
	oFlt.GetFilterByName(L"");
	return 0;
}