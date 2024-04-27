#include "edSystem.h"
#include <stdio.h>
#include "edMem.h"

#include <stdlib.h>
#include <string.h>
#include <cstdarg>
#include <assert.h>

#ifdef PLATFORM_WIN
#include <string.h>
#endif

edSysHandlersNodeTable g_SysHandlersNodeTable_00489170;

namespace edCSysHandlerPool
{
	edSysHandlersPoolEntry* create_node(edSysHandlersNodeTable* pNodeTable)
	{
		edSysHandlersPoolEntry* peVar1;

		if (pNodeTable->maxPoolEntries - pNodeTable->currentPoolEntries < 1) {
			/* edCSysHandlerPool::create_node: Not enough room in node table */
			//SystemMessage(0, 0, s_edCSysHandlerPool::create_node:_N_00432d00);
			//Break();
			peVar1 = (edSysHandlersPoolEntry*)0x0;
		}
		else {
			peVar1 = pNodeTable->pNextFreePoolEntry;
			pNodeTable->pNextFreePoolEntry = peVar1->pNextEntry;
			pNodeTable->currentPoolEntries = pNodeTable->currentPoolEntries + 1;
		}
		return peVar1;
	}

	bool initialize(edSysHandlersNodeTable* pNode, int count)
	{
		bool bVar1;
		edSysHandlersPoolEntry* peVar2;
		int iVar3;

		if (pNode->pPoolStart == (edSysHandlersPoolEntry*)0x0) {
			pNode->maxPoolEntries = count;
			peVar2 = new edSysHandlersPoolEntry[count];
			pNode->pPoolStart = peVar2;
			if (pNode->pPoolStart == (edSysHandlersPoolEntry*)0x0) {
				/* edCSysHandlerPool::initialize: Unable to allocate resource pool */
				//SystemMessage(0, 0, s_edCSysHandlerPool::initialize:_U_00432c80);
				//Break();
				//terminate((int*)pNode);
				bVar1 = false;
			}
			else {
				pNode->pNextFreePoolEntry = pNode->pPoolStart;
				for (iVar3 = 0; iVar3 < count + -1; iVar3 = iVar3 + 1) {
					pNode->pNextFreePoolEntry[iVar3].pNextEntry = pNode->pNextFreePoolEntry + iVar3 + 1;
				}
				bVar1 = true;
				pNode->pNextFreePoolEntry[iVar3].pNextEntry = (edSysHandlersPoolEntry*)0x0;
			}
		}
		else {
			/* edCSysHandlerPool::initialize: Initialization already done */
			//SystemMessage(0, 0, s_edCSysHandlerPool::initialize:_I_00432c40);
			//Break();
			bVar1 = false;
		}
		return bVar1;
	}

}

bool edSysHandlersAdd(edSysHandlersNodeTable* pNode, edSysHandlersPoolEntry** param_2, int param_3, edSysHandlerType type, EdSysFunc* pHandlerFunc, long param_6, long param_7)
{
	edSysHandlersPoolEntry* peVar1;
	bool success;
	edSysHandlersPoolEntry* peVar2;
	edSysHandlersPoolEntry** ppeVar3;
	edSysHandlersPoolEntry* peVar4;

	if ((((pNode == (edSysHandlersNodeTable*)0x0) || ((long)(char)type < 0)) || ((long)param_3 <= (long)(char)type)) || ((param_6 < 1 || (pHandlerFunc == (EdSysFunc*)0x0)))) {
		/* edSysHandlersAdd: Bad parameters */
		//SystemMessage(0, 0, s_edSysHandlersAdd:_Bad_parameters_00432d70);
		//Break();
		success = false;
	}
	else {
		peVar2 = edCSysHandlerPool::create_node(pNode);
		if (peVar2 == (edSysHandlersPoolEntry*)0x0) {
			/* edSysHandlersAdd: Unable to allocate memory */
			//SystemMessage(0, 0, s_edSysHandlersAdd:_Unable_to_allo_00432da0);
			//Break();
			success = false;
		}
		else {
			ppeVar3 = param_2 + (char)type;
			peVar1 = *ppeVar3;
			peVar2->pFunc = pHandlerFunc;
			peVar2->field_0x8 = (int)param_6 + -1;
			peVar2->field_0xc = 0;
			if (peVar1 == (edSysHandlersPoolEntry*)0x0) {
				*ppeVar3 = peVar2;
				peVar2->pNextEntry = (edSysHandlersPoolEntry*)0x0;
			}
			else {
				if (param_7 == 0) {
					peVar2->pNextEntry = peVar1;
					*ppeVar3 = peVar2;
				}
				else {
					do {
						peVar4 = peVar1;
						peVar1 = peVar4->pNextEntry;
					} while (peVar1 != (edSysHandlersPoolEntry*)0x0);
					peVar4->pNextEntry = peVar2;
					peVar2->pNextEntry = (edSysHandlersPoolEntry*)0x0;
				}
			}
			success = true;
		}
	}
	return success;
}

bool edSysHandlersCall(int typeA, edSysHandlersPoolEntry** pPool, int eventMax, int eventID, void* param_5)
{
	bool bVar1;
	edSysHandlersPoolEntry* pEntry;

	if ((eventID < 0) || (eventMax <= eventID)) {
		IMPLEMENTATION_GUARD();
		/* edSysHandlersCall: Invalid event index */
		//SystemMessage(0, 0, s_edSysHandlersCall:_Invalid_event_00432e00);
		//Break();
		bVar1 = false;
	}
	else {
		for (pEntry = pPool[eventID]; bVar1 = true, pEntry != (edSysHandlersPoolEntry*)0x0; pEntry = pEntry->pNextEntry) {
			if (pEntry->field_0xc == 0) {
				(*pEntry->pFunc)(typeA, eventID, (char*)param_5);
				pEntry->field_0xc = pEntry->field_0x8;
			}
			else {
				pEntry->field_0xc = pEntry->field_0xc + -1;
			}
		}
	}
	return bVar1;
}
