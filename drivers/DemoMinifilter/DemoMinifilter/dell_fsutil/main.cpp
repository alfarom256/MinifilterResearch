#include <Windows.h>
#include "memory.h"
#include "FltUtil.h"

int main() {
	PVOID lpFltMgrBase = ResolveFltmgrBase();
	PVOID lpFltGlobals = ResolveFltmgrGlobals(lpFltMgrBase);
	printf("Flt globals is at %p\n", lpFltGlobals);
	getchar();
	return 0;
}