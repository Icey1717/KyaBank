#include "ed3D.h"

#define HASH_CODE_HASH 0x48534148
#define HASH_CODE_MBNK 0x4b4e424d

#define ED3D_LOG(...)

int ed3DG2DGetG2DNbMaterials(ed_Chunck* pChunck)
{
	int materialCount;
	int adjustedChunkSize;

	if ((pChunck->hash == HASH_CODE_HASH) || (materialCount = -1, pChunck->hash == HASH_CODE_MBNK /*MBNK*/)) {
		adjustedChunkSize = pChunck->size + -0x10;
		materialCount = adjustedChunkSize >> 4;
		if (adjustedChunkSize < 0) {
			materialCount = pChunck->size + -1 >> 4;
		}
	}

	return materialCount;
}

#define HASH_CODE_MAT 0x2e54414d
#define HASH_CODE_MATA 0x4154414d

ed_Chunck* edChunckGetFirst(void* pBuffStart, char* pBuffEnd)
{
	char* pStart = reinterpret_cast<char*>(pBuffStart);
	/* Checks that the end of the file is greater than the start of the file */
	if ((pBuffEnd != (char*)0x0) && (pBuffEnd <= pStart)) {
		pStart = (char*)0x0;
	}
	return (ed_Chunck*)pStart;
}

ed_Chunck* edChunckGetNext(ed_Chunck* pCurChunck, char* pBuffEnd)
{
	ed_Chunck* pNextChunck;

	if ((pBuffEnd == (char*)0x0) || (pNextChunck = (ed_Chunck*)0x0, ((char*)pCurChunck + pCurChunck->size) < pBuffEnd)) {
		assert(pCurChunck->size > 0);
		pNextChunck = (ed_Chunck*)((char*)pCurChunck + pCurChunck->size);
	}
	return pNextChunck;
}

ed_hash_code* ed3DG2DGetMaterialFromIndex(ed_g2d_manager* pManager, int index)
{
	ed_Chunck* pMATA_HASH;
	ed_Chunck* pNextChunk;
	uint materialIndex;
	ed_hash_code* pHashCode;
	char* pChunkEnd;

	ED3D_LOG(LogLevel::Verbose, "ed3DG2DGetMaterialFromIndex Looking for index: {}", index);

	pMATA_HASH = pManager->pMATA_HASH;
	ed_Chunck* pMATA = pManager->pMATA_HASH - 1;

	/* Ensure we are trying to read something from the 'MATA' section */
	if (pMATA->hash == HASH_CODE_MATA) {
		/* Work out the section end address */
		pChunkEnd = (char*)pMATA + pMATA->size;
		for (pNextChunk = edChunckGetFirst(pMATA_HASH, pChunkEnd); pNextChunk != (ed_Chunck*)0x0; pNextChunk = edChunckGetNext(pNextChunk, pChunkEnd)) {
			ED3D_LOG(LogLevel::Verbose, "ed3DG2DGetMaterialFromIndex Chunk: {}", pNextChunk->GetHeaderString());

			/* Check the first value in the buffer is *MAT.* */
			if (pNextChunk->hash == HASH_CODE_MAT) {
				ED3D_LOG(LogLevel::Verbose, "ed3DG2DGetMaterialFromIndex Found MAT. chunk - size: 0x{:x}", pNextChunk->size);

				if ((index == 0) && (pMATA_HASH->hash == HASH_CODE_HASH)) {
					ED3D_LOG(LogLevel::Verbose, "ed3DG2DGetMaterialFromIndex Found HASH chunk - size: 0x{:x}", pNextChunk->size);

					/* Check the first value in the buffer is *HASH* */
					pHashCode = (ed_hash_code*)(pMATA_HASH + 1);

					const int materialCount = ed3DG2DGetG2DNbMaterials(pMATA_HASH);

					ED3D_LOG(LogLevel::Verbose, "ed3DG2DGetMaterialFromIndex Searching through {} materials for hash: {}", materialCount, pHashCode->hash.ToString());

					for (materialIndex = materialCount; materialIndex != 0; materialIndex = materialIndex - 1) {

						ed_Chunck* pMAT = LOAD_SECTION_CAST(ed_Chunck*, pHashCode->pData);

						assert(pMAT->hash == HASH_CODE_MAT);

						if (pMAT == pNextChunk) {
							ED3D_LOG(LogLevel::Verbose, "ed3DG2DGetMaterialFromIndex Found material at index {}", materialCount - materialIndex);
							return pHashCode;
						}

						pHashCode = pHashCode + 1;
					}
				}

				index = index + -1;
			}
			/* Jump to the next section */
		}
	}

	return (ed_hash_code*)0x0;
}

ed_g2d_material* ed3DG2DGetG2DMaterialFromIndex(ed_g2d_manager* pManager, int index)
{
	ed_hash_code* pMAT_HASH;
	ed_g2d_material* pMAT_Internal;

	ED3D_LOG(LogLevel::Verbose, "ed3DG2DGetG2DMaterialFromIndex Looking for index: {}", index);

	pMAT_HASH = ed3DG2DGetMaterialFromIndex(pManager, index);

	if (pMAT_HASH == (ed_hash_code*)0x0) {
		pMAT_Internal = (ed_g2d_material*)0x0;
	}
	else {
		ed_Chunck* pMAT = LOAD_SECTION_CAST(ed_Chunck*, pMAT_HASH->pData);

		assert(pMAT->hash == HASH_CODE_MAT);

		pMAT_Internal = reinterpret_cast<ed_g2d_material*>(pMAT + 1);
	}

	return pMAT_Internal;
}

ed_g2d_manager gpG2D[0x10];
int gNbG2D;

#define HASH_CODE_ANMA 0x414d4e41
#define HASH_CODE_PALL 0x4c4c4150
#define HASH_CODE_T2DA 0x41443254
#define HASH_CODE_2D 0x2a44322a

void ed3DPrepareG2DManageStruct(ed_g2d_manager* pManager, char* pFileBuffer, int fileLength)
{
	ed_Chunck* pChunck;
	ed_Chunck* pSubChunck;
	int chunkHeader;

	/* seek through the buffer */
	for (pChunck = edChunckGetFirst(pFileBuffer, pFileBuffer + fileLength); pChunck != (ed_Chunck*)0x0; pChunck = edChunckGetNext(pChunck, pFileBuffer + fileLength)) {
		ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Chunk: {}", pChunck->GetHeaderString());

		chunkHeader = pChunck->hash;

		/* Check the first value in the buffer is *2D* */
		if (chunkHeader == HASH_CODE_2D) {
			/* Set the readValue to be the last section of the texture header */
			const int chunkSize = pChunck->size;

			ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found *2D* chunk - size: 0x{:x}", chunkSize);


			pManager->pTextureChunk = pChunck;

			for (pSubChunck = edChunckGetFirst(pChunck + 1, ((char*)pChunck + chunkSize)); pSubChunck != (ed_Chunck*)0x0;
				pSubChunck = edChunckGetNext(pSubChunck, ((char*)pChunck + chunkSize))) {
				ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct SubChunk: {}", pSubChunck->GetHeaderString());

				/* Check if the value in the buffer is 'MATA' */
				if (pSubChunck->hash == HASH_CODE_MATA) {
					ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found MATA chunk - size: 0x{:x}", pSubChunck->size);
					pManager->pMATA_HASH = (ed_Chunck*)(pSubChunck + 1);
				}
			}
		}
		else {
			/* Check if the value in the buffer is 'ANMA' */
			if (chunkHeader == HASH_CODE_ANMA) {
				ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found ANMA chunk - size: 0x{:x}", pChunck->size);
				pManager->pANMA = pChunck;
			}
			else {
				/* Check if the value in the buffer is 'PALL' */
				if (chunkHeader == HASH_CODE_PALL) {
					ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found PALL chunk - size: 0x{:x}", pChunck->size);
					pManager->pPALL = pChunck;
				}
				else {
					/* Check if the value in the buffer is 'T2DA' */
					if (chunkHeader == HASH_CODE_T2DA) {
						ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found T2DA chunk - size: 0x{:x}", pChunck->size);
						pManager->pT2DA = pChunck;
					}
				}
			}
		}
	}
	return;
}

#define HASH_CODE_REAA 0x41414552
#define HASH_CODE_REAL 0x4c414552
#define HASH_CODE_RSTR 0x52545352
#define HASH_CODE_ROBJ 0x4a424f52
#define HASH_CODE_RGEO 0x4f454752

int* ed3DPreparePointer(char* pFileBufferA, int lengthA, char* pFileBufferB, int lengthB)
{
	int reaaChunkSize;
	ed_Chunck* pChunk;
	ed_Chunck* pREAA;
	int* nextSubSection;
	uint uVar3;
	uint uVar4;
	char* actualFileStart;
	char* pcVar5;
	int chunkHeader;

	pcVar5 = pFileBufferA + (-lengthB - (ulong)pFileBufferB);
	actualFileStart = pFileBufferB + -0x10;

	if (pFileBufferA == pFileBufferB) {
		pcVar5 = (char*)0x0;
	}

	pREAA = edChunckGetFirst(pFileBufferA, pFileBufferA + lengthA);

	ED3D_LOG(LogLevel::Info, "ed3DPreparePointer First chunk: {}", pREAA->GetHeaderString());

	while (true) {
		if (pREAA == (ed_Chunck*)0x0) {
			return (int*)0x0;
		}

		// Check if the value in the buffer is 'REAA'
		if ((pREAA->hash == HASH_CODE_REAA) && (pREAA->field_0x4 != 0x7b6)) break;

		pREAA = edChunckGetNext(pREAA, pFileBufferA + lengthA);
		ED3D_LOG(LogLevel::Info, "ed3DPreparePointer Searching for REAA - current: {}", pREAA->GetHeaderString());
	}

	reaaChunkSize = pREAA->size;
	pREAA->field_0x4 = 0x7b6;

	ED3D_LOG(LogLevel::Info, "ed3DPreparePointer REAA chunk size: {}", reaaChunkSize);

	pChunk = edChunckGetFirst(pREAA + 1, ((char*)pREAA + reaaChunkSize));

	ED3D_LOG(LogLevel::Info, "ed3DPreparePointer Post REAA chunk: {}", pChunk->GetHeaderString());

	pcVar5 = actualFileStart + (int)pcVar5;

	for (; pChunk != (ed_Chunck*)0x0; pChunk = edChunckGetNext(pChunk, ((char*)pREAA + reaaChunkSize))) {
		ED3D_LOG(LogLevel::Info, "ed3DPreparePointer edChunckGetNext: {}", pChunk->GetHeaderString());

		chunkHeader = pChunk->hash;

		nextSubSection = (int*)(pChunk + 1);
		uVar3 = pChunk->size - 0x10U >> 2;
		/* Check if the value in the buffer is 'REAL' */
		if (chunkHeader == HASH_CODE_REAL) {
			ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found REAL chunk - size: 0x{:x}", pChunk->size);

			for (uVar4 = 0; uVar4 < (uVar3 & 0xffff); uVar4 = uVar4 + 1 & 0xffff) {
				/* Jump forward one section */
				if (*nextSubSection != 0) {
					/* Store a pointer to the next sub section */
					int offset = *(int*)(actualFileStart + *nextSubSection);
					*(int*)(actualFileStart + *nextSubSection) = STORE_SECTION(actualFileStart + offset);
				}

				nextSubSection = nextSubSection + 1;
			}
		}
		else {
			/* Is current value RSTR */
			if (chunkHeader == HASH_CODE_RSTR) {
				ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found RSTR chunk - size: 0x{:x}", pChunk->size);

				for (uVar4 = 0; uVar4 < (uVar3 & 0xffff); uVar4 = uVar4 + 1 & 0xffff) {
					if (*nextSubSection != 0) {
						/* Store a pointer to the next sub section */
						*(int*)(pcVar5 + *nextSubSection) = (int)STORE_SECTION(actualFileStart + (int)*(char**)(pcVar5 + *nextSubSection));
					}

					nextSubSection = nextSubSection + 1;
				}
			}
			else {
				/* Is current value ROBJ */
				if (chunkHeader == HASH_CODE_ROBJ) {
					ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found ROBJ chunk - size: 0x{:x}", pChunk->size);

					for (uVar4 = 0; uVar4 < (uVar3 & 0xffff); uVar4 = uVar4 + 1 & 0xffff) {
						if (*nextSubSection != 0) {
							/* Store a pointer to the next sub section */
							*(int*)(actualFileStart + *nextSubSection) = (int)STORE_SECTION(pcVar5 + (int)*(char**)(actualFileStart + *nextSubSection));
						}

						nextSubSection = nextSubSection + 1;
					}
				}
				else {
					/* Is current value RGEO */
					if (chunkHeader == HASH_CODE_RGEO) {
						ED3D_LOG(LogLevel::Info, "ed3DPrepareG2DManageStruct Found RGEO chunk - size: 0x{:x}", pChunk->size);

						for (uVar4 = 0; uVar4 < (uVar3 & 0xffff); uVar4 = uVar4 + 1 & 0xffff) {
							if (*nextSubSection != 0) {
								/* Store a pointer to the next sub section */
								*(int*)(pcVar5 + *nextSubSection) = (int)STORE_SECTION(pcVar5 + (int)*(char**)(pcVar5 + *nextSubSection));
							}

							nextSubSection = nextSubSection + 1;
						}
					}
				}
			}
		}
	}

	ED3D_LOG(LogLevel::Info, "ed3DPreparePointer Complete");

	return (int*)pREAA;
}

#define HASH_CODE_T2D 0x4432472e

ed_g2d_manager* ed3DInstallG2D(char* pFileBuffer, int fileLength, int* outInt, ed_g2d_manager* pManager, int param_5)
{
	bool bIsNewManager;
	ed_g2d_manager* pOutManager;

	char* pFileBody;
	ed_g2d_manager* pTexturePool;

	ED3D_LOG(LogLevel::Info, "ed3DInstallG2D");

	/* Find some free area in the buffer? */
	pOutManager = pManager;

	pTexturePool = gpG2D;
	if (pManager == (ed_g2d_manager*)0x0) {
		while (pOutManager = pTexturePool, pOutManager->pFileBuffer != (GXD_FileHeader*)0x0) {
			pTexturePool = pOutManager + 1;
		}
	}

	if ((pOutManager == (ed_g2d_manager*)0x0) || (memset(pOutManager, 0, sizeof(ed_g2d_manager)), pFileBuffer == (char*)0x0)) {
		bIsNewManager = pOutManager != pManager;
		pOutManager = (ed_g2d_manager*)0x0;

		if (bIsNewManager) {
			gNbG2D = gNbG2D + -1;
			pOutManager = (ed_g2d_manager*)0x0;
		}
	}
	else {
		GXD_FileHeader* const pFileHeader = reinterpret_cast<GXD_FileHeader*>(pFileBuffer);
		pFileBody = reinterpret_cast<char*>(pFileHeader + 1);

		assert(pFileHeader->hash == HASH_CODE_T2D);

		ed3DPrepareG2DManageStruct(pOutManager, pFileBody, fileLength + -0x10);
		ed3DPreparePointer(pFileBody, fileLength + -0x10, pFileBody, fileLength + -0x10);

		pOutManager->pFileBuffer = pFileHeader;
		pOutManager->textureFileLengthA = fileLength;
		pOutManager->textureFileLengthB = fileLength;
		*outInt = fileLength - pOutManager->textureFileLengthB;

		// Not used so far.
		//ed3DPrepareG2D(pOutManager, param_5);
	}

	return pOutManager;
}