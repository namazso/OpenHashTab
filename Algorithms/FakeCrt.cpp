#include <Windows.h>

#include <cstdio>
#include <new>

#include "blake2sp.h"

_Success_(return != EOF)
_Check_return_opt_

int __cdecl fputc(
    _In_ int,
    _Inout_ FILE *
) { return 0; }

_Check_return_opt_
_CRT_STDIO_INLINE int __CRTDECL fprintf(
    _Inout_ FILE *const,
    _In_z_ _Printf_format_string_ char const *const,
    ...
) { return 0; }

extern "C" void *malloc(size_t size) {
    return HeapAlloc(GetProcessHeap(), 0, size);
}

extern "C" void free(void *ptr) {
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}
