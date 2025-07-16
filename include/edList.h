#pragma once

#include "Types.h"

typedef int(edListFunc)(void*, void*);

struct ShortField
{
	short type;
	ushort flags;
};

union NodeHeaderUnion
{
	byte byteFlags[4];
	short field_0x0[2];
	int count;
	void* randoPtr;
	ShortField typeField;
	int mode;
	edListFunc* pListFunc;
};

struct edNODE
{
	NodeHeaderUnion header;
	edNODE* pPrev;
	edNODE* pNext;
	void* pData;
};