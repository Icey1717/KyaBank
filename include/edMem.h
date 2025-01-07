#ifndef _EDMEM_H
#define _EDMEM_H

#include "Types.h"
#include <string.h>
#include <cstdlib>

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <stdlib.h>
#endif

enum EHeapType
{
	H_INVALID,
	H_MAIN,
	H_MEMKIT,
	H_SCRATCHPAD,
	H_VIDEO
};

typedef void* EHeap;

#ifdef _MSC_VER
inline void* edMemFree(void* pAlloc) { _aligned_free(pAlloc); return nullptr; }
inline void* edMemShrink(void* pAlloc, int size) { return nullptr; }
inline void* edMemAlloc(EHeap heapID, size_t size) { return _aligned_malloc(size, 1); }
inline void* edMemAllocAlign(EHeap heapID, size_t size, int align) { return _aligned_malloc(size, align); }
inline void* edMemAllocAlignBoundary(EHeap heap, size_t size, int align, int offset) { return _aligned_malloc(size, align); }
#else
inline void* edMemFree(void* pAlloc) { free(pAlloc); return nullptr; }
inline void* edMemShrink(void* pAlloc, int size) { return nullptr; }
inline void* edMemAlloc(EHeap heapID, size_t size) { return aligned_alloc(1, size); }
inline void* edMemAllocAlign(EHeap heapID, size_t size, int align) { return aligned_alloc(align, size); }
inline void* edMemAllocAlignBoundary(EHeap heap, size_t size, int align, int offset) { return aligned_alloc(align, size); }
#endif

#define TO_HEAP(a) (void*)a

#endif //_EDMEM_H