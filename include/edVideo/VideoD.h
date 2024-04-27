// Dummy file for includes

#ifndef _EDVIDEO_VIDEO_D_H_
#define _EDVIDEO_VIDEO_D_H_

#include "Types.h"

struct edSysHandlerVideo {
	edSysHandlersNodeTable* nodeParent;
	edSysHandlersPoolEntry* entries[11];
	int maxEventID;
	int mainIdentifier;
};

edSysHandlerVideo edSysHandlerVideo_0048cee0;

#endif 