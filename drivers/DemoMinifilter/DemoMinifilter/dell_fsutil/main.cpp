#include <Windows.h>
#include "memory.h"
#include "FltUtil.h"

int main() {
	FltManager oFlt = FltManager();
	printf("Flt globals is at %p\n", oFlt.lpFltGlobals);
	getchar();
	return 0;
}