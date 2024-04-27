#ifndef _EDMEM_H
#define _EDMEM_H

#include "Types.h"
#include <string.h>
#include <cstdlib>

#include <malloc.h>


enum EHeapType
{
	H_INVALID,
	H_MAIN,
	H_MEMKIT,
	H_SCRATCHPAD,
	H_VIDEO
};

typedef void* EHeap;

inline void* edMemFree(void* pAlloc) { _aligned_free(pAlloc); return nullptr; }
inline void* edMemShrink(void* pAlloc, int size) { return nullptr; }
inline void* edMemAlloc(EHeap heapID, size_t size) { return _aligned_malloc(size, 1); }
inline void* edMemAllocAlign(EHeap heapID, size_t size, int align) { return _aligned_malloc(size, align); }
inline void* edMemAllocAlignBoundary(EHeap heap, size_t size, int align, int offset) { return _aligned_malloc(size, align); }

#define TO_HEAP(a) (void*)a

#endif //_EDMEM_H