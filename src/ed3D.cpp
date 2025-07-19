#include "ed3D.h"
#include "edMem.h"

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

#define HASH_CODE_CDZA 0x415a4443
#define HASH_CODE_INFA 0x41464e49
#define HASH_CODE_MBNA 0x414e424d
#define HASH_CODE_SPRA 0x41525053
#define HASH_CODE_CAMA 0x414d4143
#define HASH_CODE_LIA_ 0x2e41494c
#define HASH_CODE_CSTA 0x41545343
#define HASH_CODE_ANMA 0x414d4e41
#define HASH_CODE_HALL 0x4c4c4148
#define HASH_CODE_OBJA 0x414a424f
#define HASH_CODE_GEOM 0x4d4f4547

inline void ProcessG3DChunck(ed_g3d_manager* pManager, ed_Chunck* pChunck)
{
	int chunkHeader = pChunck->hash;

	/* Is current value CDZA */
	if (chunkHeader == HASH_CODE_CDZA) {
		ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
		pManager->CDZA = pChunck;
	}
	else {
		/* Is current value INFA */
		if (chunkHeader == HASH_CODE_INFA) {
			ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
			pManager->INFA = pChunck;
		}
		else {
			/* Is current value MBNA */
			if (chunkHeader == HASH_CODE_MBNA) {
				ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
				pManager->MBNA = pChunck;
			}
			else {
				/* Is current value SPRA */
				if (chunkHeader == HASH_CODE_SPRA) {
					ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
					pManager->SPRA = pChunck;
				}
				else {
					/* Is current value CAMA */
					if (chunkHeader == HASH_CODE_CAMA) {
						ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
						pManager->CAMA = pChunck;
					}
					else {
						/* Is current value LIA. */
						if (chunkHeader == HASH_CODE_LIA_) {
							ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
							pManager->LIA = pChunck;
						}
						else {
							/* Is current value CSTA */
							if (chunkHeader == HASH_CODE_CSTA) {
								ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
								pManager->CSTA = pChunck;
							}
							else {
								/* Is current value ANMA */
								if (chunkHeader == HASH_CODE_ANMA) {
									ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
									pManager->ANMA = pChunck;
								}
								else {
									/* Is current value HALL */
									if (chunkHeader == HASH_CODE_HALL) {
										ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
										pManager->HALL = pChunck;
									}
									else {
										/* Is current value OBJA */
										if (chunkHeader == HASH_CODE_OBJA) {
											ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
											pManager->OBJA = pChunck;
										}
										else {
											/* Is current value GEOM */
											if (chunkHeader == HASH_CODE_GEOM) {
												ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct Found {} chunk - size: 0x{:x}", pChunck->GetHeaderString(), pChunck->size);
												pManager->GEOM = pChunck;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

#define HASH_CODE_G3D 0x4433472e

void ed3DPrepareG3DManageStruct(ed_g3d_manager* pManager, char* pFileBufferA, int fileLengthA, char* pFileBufferB, int fileLengthB)
{
	ed_Chunck* pChunck;

	for (pChunck = edChunckGetFirst(pFileBufferB, pFileBufferB + fileLengthB); pChunck != (ed_Chunck*)0x0; pChunck = edChunckGetNext(pChunck, pFileBufferB + fileLengthB)) {
		ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct B Chunk: {}", pChunck->GetHeaderString());
		ProcessG3DChunck(pManager, pChunck);
	}

	if (pFileBufferA != pFileBufferB) {
		for (pChunck = edChunckGetFirst(pFileBufferA, pFileBufferA + fileLengthA); pChunck != (ed_Chunck*)0x0; pChunck = edChunckGetNext(pChunck, pFileBufferA + fileLengthA)) {
			ED3D_LOG(LogLevel::Info, "ed3DPrepareG3DManageStruct A Chunk: {}", pChunck->GetHeaderString());
			ProcessG3DChunck(pManager, pChunck);
		}
	}

	return;
}

char* edHashCodeGet(Hash_8 hash, int* texHashPtr, int hashCount, uint triedHashes, uint totalHashes)
{
	uint uVar1;
	ulong* puVar2;

	puVar2 = (ulong*)(texHashPtr + hashCount * 4);
	if (*puVar2 < hash.number) {
		uVar1 = (uint)(totalHashes - hashCount) >> 1;
		if (uVar1 == 0) {
			uVar1 = 1;
		}
		if (hashCount == totalHashes) {
			puVar2 = (ulong*)0x0;
		}
		else {
			puVar2 = (ulong*)edHashCodeGet(hash, texHashPtr, hashCount + uVar1, hashCount + 1, totalHashes);
		}
	}
	else {
		if (hash.number < *puVar2) {
			uVar1 = (uint)(hashCount - triedHashes) >> 1;
			if (uVar1 == 0) {
				uVar1 = 1;
			}
			if (hashCount == triedHashes) {
				puVar2 = (ulong*)0x0;
			}
			else {
				puVar2 = (ulong*)edHashCodeGet(hash, texHashPtr, hashCount - uVar1, triedHashes, hashCount + -1);
			}
		}
	}
	return (char*)puVar2;
}

ed_hash_code* edHashcodeGet(Hash_8 hash, ed_Chunck* pChunck)
{
	ed_hash_code* pcVar1;

	/* If the first value in the texture obj buffer is HASH or MBNK */
	if ((pChunck->hash == HASH_CODE_HASH) || (pChunck->hash == HASH_CODE_MBNK)) {
		pcVar1 = (ed_hash_code*)edHashCodeGet(hash, (int*)(pChunck + 1), pChunck->nextChunckOffset - 0x10U >> 5, 0,
			pChunck->nextChunckOffset - 0x10U >> 4);
	}
	else {
		pcVar1 = (ed_hash_code*)0x0;
	}
	return pcVar1;
}

void ed3DPrepareMaterialBank(ed_Chunck* pMBNA, ed_g2d_manager* pTextureInfo)
{
	int mbnaSize;
	ed_Chunck* pChunk;
	ushort materialCount;
	ed_hash_code* pHashCode;
	ed_Chunck* pMBNK;

	mbnaSize = pMBNA->size;
	/* Checks all the hashes in the mesh to make sure they match what is in the texture Get the end of the current section */
	for (pChunk = edChunckGetFirst(pMBNA + 1, (char*)pMBNA + mbnaSize); pChunk != (ed_Chunck*)0x0; pChunk = edChunckGetNext(pChunk, (char*)pMBNA + mbnaSize)) {
		ED3D_LOG(LogLevel::Info, "ed3DPrepareMaterialBank Chunk: {}", pChunk->GetHeaderString());

		/* Check if read value is MBNK */
		if (pChunk->hash == HASH_CODE_MBNK) {
			ED3D_LOG(LogLevel::Info, "ed3DPrepareMaterialBank Found MBNK chunk - size: 0x{:x}", pChunk->size);

			materialCount = (pChunk->size - 0x10U) >> 4;
			ED3D_LOG(LogLevel::Info, "ed3DPrepareMaterialBank Material count: {}", materialCount);

			pMBNK = pChunk;
			while (true) {
				if (materialCount == 0) break;

				ed_hash_code* pMaterialHash = (ed_hash_code*)(pMBNK + 1);

				ED3D_LOG(LogLevel::Info, "ed3DPrepareMaterialBank Trying to find texture {}: {}", ((pChunk->size - 0x10U) >> 4) - materialCount, pMaterialHash->hash.ToString());

				pHashCode = edHashcodeGet(pMaterialHash->hash, pTextureInfo->pMATA_HASH);
				if (pHashCode != (ed_hash_code*)0x0) {
					ED3D_LOG(LogLevel::Info, "ed3DPrepareMaterialBank Found texture: {}", pHashCode->hash.ToString());
					pMaterialHash->pData = STORE_SECTION(pHashCode);
				}

				materialCount = materialCount - 1;
				pMBNK = pMBNK + 1;
			}
		}
	}

	return;
}

int ed3DPrepareHierarchy(ed_g3d_hierarchy* pHierarchy, ed_g2d_manager* pTextureInfo)
{
	short sVar1;
	ushort uVar2;
	ed_Chunck* piVar3;
	int iVar4;
	char* pBuffStart;
	ed_g3d_hierarchy* pTVar5;
	ed_Chunck* pHIER;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchy {}", pHierarchy->hash.ToString());

	if ((pHierarchy->flags_0x9e & 4) == 0) {
		pTVar5 = pHierarchy + 1;

		pHIER = reinterpret_cast<ed_Chunck*>(reinterpret_cast<char*>(pHierarchy) - sizeof(ed_Chunck));

		if (pHIER->nextChunckOffset != pHIER->size) {
			// Tidy
			IMPLEMENTATION_GUARD();
			pBuffStart = ((char*)pHierarchy) + (pHIER->size - 0x10);
			iVar4 = *(int*)(pBuffStart + 8);
			for (piVar3 = edChunckGetFirst(pBuffStart, pBuffStart + iVar4); piVar3 != (ed_Chunck*)0x0;
				piVar3 = edChunckGetNext(piVar3, pBuffStart + iVar4)) {
				if (piVar3->hash == 0x494e4f42) {
					IMPLEMENTATION_GUARD(pHierarchy->field_0xa8 = piVar3 + 4);
				}
			}
		}

		pHierarchy->flags_0x9e = pHierarchy->flags_0x9e | 4;
		IMPLEMENTATION_GUARD(
			for (uVar2 = 0; uVar2 < pHierarchy->field_0x9c; uVar2 = uVar2 + 1) {
				sVar1 = *(short*)&pTVar5->field_0x4;
				piVar3 = *(int**)(*(int*)pTVar5 + 8);
				if (sVar1 == 3) {
					if (*piVar3 == 0x2e4a424f) {
						//ed3DPrepareObjectSprite((int)pHIER, (uint*)(piVar3 + 4), pTextureInfo);
					}
				}
				else {
					if ((((sVar1 != 2) && (sVar1 != 1)) && (sVar1 == 0)) && (*piVar3 == 0x2e4a424f)) {
						//ed3DPrepareObject((int)pHIER, (uint*)(piVar3 + 4), pTextureInfo);
					}
				}
				pTVar5 = (ed_g3d_hierarchy*)&pTVar5->field_0x8;
			}

		if (pHierarchy->field_0x90 != 0) {
			ed3DPrepareHierarchy((ed_g3d_hierarchy*)(pHIER->field_0x90 + 0x10), pTextureInfo);
		}
		iVar4 = 1;)
	}
	else {
		ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchy No additional data");
		iVar4 = 0;
	}

	return iVar4;
}

void ed3DPrepareHierarchyALL(ed_g3d_manager* pMeshInfo, ed_g2d_manager* pTextureInfo)
{
	ed_Chunck* pHALL;
	uint chunkNb;
	ed_hash_code* pHashCode;
	uint curIndex;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchyALL");

	pHALL = pMeshInfo->HALL;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchyALL Loaded HALL chunck hash: {} size: 0x{:x}", pHALL->GetHeaderString(), pHALL->size);

	ed_Chunck* pHASH = pHALL + 1;
	assert(pHASH->hash == HASH_CODE_HASH);
	ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchyALL Loaded HASH chunck hash: {} size: 0x{:x}", pHASH->GetHeaderString(), pHASH->size);

	pHashCode = reinterpret_cast<ed_hash_code*>(pHASH + 1);
	chunkNb = edChunckGetNb(pHASH, reinterpret_cast<char*>(pHALL) + pHALL->size);
	ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchyALL Hash count: {}", chunkNb - 1);

	for (curIndex = 0; curIndex < chunkNb - 1; curIndex = curIndex + 1) {
		ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchyALL Processing hash {}/{}: {}", curIndex, chunkNb - 1, pHashCode->hash.ToString());

		ed_Chunck* pHIER = LOAD_SECTION_CAST(ed_Chunck*, pHashCode->pData);
		assert(pHIER->hash == HASH_CODE_HIER);
		ED3D_LOG(LogLevel::Info, "ed3DPrepareHierarchyALL Loaded HIER chunck hash: {} size: 0x{:x}", pHIER->GetHeaderString(), pHIER->size);

		ed3DPrepareHierarchy(reinterpret_cast<ed_g3d_hierarchy*>(pHIER + 1), pTextureInfo);
		pHashCode = pHashCode + 1;
	}
	return;
}

struct astruct_14
{
	int* field_0x0;
	int* field_0x4;
	int* field_0x8;
	char field_0xc;
	undefined field_0xd;
	undefined field_0xe;
	undefined field_0xf;
	ClusterDetails clusterDetails;
	short field_0x24;
};

int INT_0044935c = 0;
bool BOOL_00449370 = false;

void ed3DPrepareCluster(ed_g3d_cluster* pCluster, bool param_2, ed_g3d_manager* pMeshInfo, ed_g2d_manager* pTextureInfo, int param_5, bool bHasFlag)
{
	ushort uVar1;
	ed_Chunck* piVar2;
	int chunkSize;
	int offset;
	ed_Chunck* piVar4;
	uint* puVar5;
	uint uVar6;
	uint uVar7;
	uint uVar8;
	astruct_14 aStack96;
	astruct_14 local_30;
	bool bHasInternalFlag;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterTree");

	bHasInternalFlag = (pCluster->flags_0x1c & 1) == 0;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterTree Internal flag: {}", bHasInternalFlag);

	if (bHasFlag == false) {
		if (bHasInternalFlag) {
			uVar8 = 0;
			pCluster->flags_0x1c = pCluster->flags_0x1c | 1;

			uVar7 = 0;
			do {
				uVar6 = uVar7 + 1 & 0xffff;
				uVar8 = uVar8 + pCluster->aClusterStripCounts[uVar7 - 8];
				uVar7 = uVar6;
			} while (uVar6 < 0xd);

			INT_0044935c = 0x60;
			local_30.field_0xc = '\0';
			local_30.clusterDetails = pCluster->clusterDetails;
			piVar4 = (ed_Chunck*)pCluster->field_0x30;
			piVar2 = (ed_Chunck*)pCluster->field_0x34;
			local_30.field_0x4 = (int*)pTextureInfo;
			local_30.field_0x8 = (int*)param_5;
			for (uVar7 = 0; uVar7 < uVar8; uVar7 = uVar7 + 1 & 0xffff) {
				IMPLEMENTATION_GUARD(
					local_30.field_0x0 = piVar2 + 4;
				puVar5 = ed3DPrepareStrip(&local_30);
				if ((puVar5 != (uint*)0x0) &&
					(g_pStrippBufLastPos =
						(edpkt_data*)
						ed3DStripPreparePacket(puVar5, (ulong*)g_pStrippBufLastPos, (int)(piVar4 + 4), 4), uVar7 == 0
						)) {
					pCluster->field_0x38 = puVar5;
				}
				piVar2 = edChunckGetNext(piVar2, (int*)0x0);)
			}
		}
	}
	else {
		if (BOOL_00449370 == true) {
			IMPLEMENTATION_GUARD(
				piVar4 = (int*)pCluster->field_0x30;
			puVar5 = (uint*)pCluster->field_0x38;
			uVar7 = 0;
			if ((piVar4 != (int*)0x0) && (puVar5 != (uint*)0x0)) {
				uVar8 = 0;
				do {
					uVar6 = uVar8 + 1 & 0xffff;
					uVar7 = uVar7 + pCluster->field_0x10[uVar8 - 8];
					uVar8 = uVar6;
				} while (uVar6 < 0xd);
				INT_0044935c = 0x60;
				for (uVar8 = 0; uVar8 < uVar7; uVar8 = uVar8 + 1 & 0xffff) {
					if ((long)(int)puVar5[0xf] != 0) {
						ed3DStripPrepareSpherePacket((int)puVar5, (long)(int)puVar5[0xf], (int)(piVar4 + 4));
					}
					puVar5 = (uint*)puVar5[3];
				}
				chunkSize = *(int*)&pCluster[-1].field_0x44;
				goto LAB_002a40c4;
			})
		}
	}

	ed_Chunck* pChunk = reinterpret_cast<ed_Chunck*>(reinterpret_cast<char*>(pCluster) - sizeof(ed_Chunck));

	chunkSize = pChunk->size;
LAB_002a40c4:
	uVar1 = pCluster->field_0x1a;
	offset = (uint)uVar1 << 3;

	if ((uVar1 & 1) != 0) {
		offset = (uVar1 + 1) * 8;
	}

	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterTree Header chunk: {} size: 0x{:x} flags: 0x{:x} offset: 0x{:x}", pChunk->GetHeaderString(), chunkSize, uVar1, offset);

	ed_Chunck* pPostClusterChunk;
	for (pPostClusterChunk = edChunckGetFirst(reinterpret_cast<char*>(pCluster) + offset + sizeof(ed_g3d_cluster), reinterpret_cast<char*>(pCluster) + chunkSize + -sizeof(ed_Chunck));
		pPostClusterChunk != (ed_Chunck*)0x0; pPostClusterChunk = edChunckGetNext(pPostClusterChunk, reinterpret_cast<char*>(pCluster) + chunkSize + -sizeof(ed_Chunck))) {
		if (pPostClusterChunk->hash == HASH_CODE_SPRA) {
			IMPLEMENTATION_GUARD(
				if (bHasInternalFlag) {
					aStack96.field_0xc = '\0';
					aStack96.clusterDetails = pCluster->clusterDetails;
					aStack96.field_0x4 = (int*)pTextureInfo;
					aStack96.field_0x8 = (int*)param_5;
					IMPLEMENTATION_GUARD(puVar5 = ed3DPrepareAllSprite((int)pPostClusterChunk, &aStack96, (int)(pCluster->field_0x30 + 4), 4));
					pCluster->field_0x3c = (int)STORE_SECTION(puVar5);
					pCluster->field_0x1e = aStack96.field_0x24;
				})
		}
		else {
			if ((pPostClusterChunk->hash == HASH_CODE_CDQU) || (pPostClusterChunk->hash == HASH_CODE_CDOC)) {
				ed3DPrepareCluster((ed_g3d_cluster*)(pPostClusterChunk + 4), param_2, pMeshInfo, pTextureInfo, param_5, bHasFlag);
			}
		}
	}

	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterTree Complete");

	return;
}

edpkt_data* g_pStrippBufLastPos;

void ed3DPrepareClusterALL(bool bUnused, ed_g3d_manager* meshInfoObj, ed_g2d_manager* textureInfo, int param_4)
{
	int cstaSize;
	int chunkSize;
	bool bHasFlag;
	ed_Chunck* pCSTA;
	edpkt_data* pCommandBuffer;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL");

	pCommandBuffer = g_pStrippBufLastPos;

	pCSTA = meshInfoObj->CSTA;
	cstaSize = pCSTA->size;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Loaded CSTA chunck hash: {} size: 0x{:x}", pCSTA->GetHeaderString(), pCSTA->size);

	bHasFlag = (meshInfoObj->fileBufferStart->flags & 1) != 0;
	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Mesh has flag: {}", bHasFlag);

	ed_Chunck* pNextChunk = pCSTA + 1;
	ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Loaded next chunck hash: {} size: 0x{:x}", pNextChunk->GetHeaderString(), pNextChunk->size);

	char* const pCSTA_End = reinterpret_cast<char*>(pCSTA) + cstaSize;

	for (ed_Chunck* pChunk = edChunckGetFirst(pNextChunk, pCSTA_End); pChunk != (ed_Chunck*)0x0; pChunk = edChunckGetNext(pChunk, pCSTA_End)) {
		ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Processing chunck hash: {} size: 0x{:x}", pChunk->GetHeaderString(), pChunk->size);

		/* If read value is CDQA -> Quad Tree */
		if (pChunk->hash == HASH_CODE_CDQA) {
			ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Found CDQA chunck hash: {} size: 0x{:x}", pChunk->GetHeaderString(), pChunk->size);

			/* Search for CDQU */
			chunkSize = pChunk->size;
			char* const pChunkEnd = reinterpret_cast<char*>(pChunk) + chunkSize;

			for (ed_Chunck* pSubChunk = edChunckGetFirst(pChunk + 3, pChunkEnd); pSubChunk != (ed_Chunck*)0x0; pSubChunk = edChunckGetNext(pSubChunk, pChunkEnd))
			{
				ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Processing chunck hash: {} size: 0x{:x}", pSubChunk->GetHeaderString(), pSubChunk->size);

				/* If read value is CDQU */
				if (pSubChunk->hash == HASH_CODE_CDQU) {
					ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Found CDQU chunck hash: {} size: 0x{:x}", pSubChunk->GetHeaderString(), pSubChunk->size);
					ed3DPrepareCluster((ed_g3d_cluster*)(pSubChunk + 1), true, meshInfoObj, textureInfo, param_4, bHasFlag);
				}
			}
		}
		else {
			/* If read value is CDOA -> Octree */
			if (pChunk->hash == HASH_CODE_CDOA) {
				ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Found CDOA chunck hash: {} size: 0x{:x}", pChunk->GetHeaderString(), pChunk->size);

				/* Search for CDOC */
				chunkSize = pChunk->size;
				char* const pChunkEnd = reinterpret_cast<char*>(pChunk) + chunkSize;

				for (ed_Chunck* pSubChunk = edChunckGetFirst(pChunk + 3, pChunkEnd); pSubChunk != (ed_Chunck*)0x0; pSubChunk = edChunckGetNext(pSubChunk, pChunkEnd)) {
					ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Processing chunck hash: {} size: 0x{:x}", pSubChunk->GetHeaderString(), pSubChunk->size);

					/* If read value is CDOC */
					if (pSubChunk->hash == HASH_CODE_CDOC) {
						ED3D_LOG(LogLevel::Info, "ed3DPrepareClusterALL Found CDOC chunck hash: {} size: 0x{:x}", pSubChunk->GetHeaderString(), pSubChunk->size);
						ed3DPrepareCluster((ed_g3d_cluster*)(pSubChunk + 1), true, meshInfoObj, textureInfo, param_4, bHasFlag);
					}
				}
			}
		}
	}

	meshInfoObj->fileLengthB = meshInfoObj->fileLengthB + (int)((ulong)g_pStrippBufLastPos - (ulong)pCommandBuffer);
	return;
}

void ed3DPrepareG3D(bool bUnused, ed_g3d_manager* pMeshManager, ed_g2d_manager* pTextureManager, int unknown)
{
	char* pAlloc;

	ED3D_LOG(LogLevel::Info, "ed3DPrepareG3D");

	if (pTextureManager != (ed_g2d_manager*)0x0) {
		ed3DPrepareMaterialBank(pMeshManager->MBNA, pTextureManager);
	}

	if (pMeshManager->CSTA == (ed_Chunck*)0x0) {
		ed3DPrepareHierarchyALL(pMeshManager, pTextureManager);
		pAlloc = pMeshManager->field_0x4;
	}
	else {
		ed3DPrepareClusterALL(bUnused, pMeshManager, pTextureManager, unknown);
		pAlloc = pMeshManager->field_0x4;
	}

	if (pAlloc != (char*)0x0) {
		edMemFree(pAlloc);
		pMeshManager->fileLengthB = pMeshManager->fileLengthB + -pMeshManager->field_0xc;
		pMeshManager->field_0xc = 0;
		pMeshManager->field_0x4 = (char*)0x0;
		pMeshManager->GEOM = (ed_Chunck*)0x0;
	}

	return;
}

ed_g3d_manager* gpG3D = NULL;
int gNbG3D = 0;

ed_g3d_manager* ed3DInstallG3D(char* pFileBuffer, int fileLength, ulong flags, int* outInt, ed_g2d_manager* pTextureManager, int unknown, ed_g3d_manager* pManager)
{
	bool bIsNewManager;
	int size;
	ed_g3d_manager* pMeshPool;
	int* piVar3;
	char* pFileBodyB;
	ed_g3d_manager* pOutManager;
	char* pFileBodyA;
	int size_00;

	ED3D_LOG(LogLevel::Info, "ed3DInstallG3D");

	pOutManager = pManager;

	pMeshPool = gpG3D;
	if (pManager == (ed_g3d_manager*)0x0) {
		while (pOutManager = pMeshPool, pOutManager->fileBufferStart != (GXD_FileHeader*)0x0) {
			pMeshPool = pOutManager + 1;
		}
	}

	GXD_FileHeader* const pFileHeader = reinterpret_cast<GXD_FileHeader*>(pFileBuffer);

	/* Check if the offset value from the buffer is '.G3D' */
	if (((pOutManager == (ed_g3d_manager*)0x0) || (memset(pOutManager, 0, sizeof(ed_g3d_manager)), pFileBuffer == (char*)0x0)) || (pFileHeader->hash != HASH_CODE_G3D)) {
		bIsNewManager = pOutManager != pManager;
		pOutManager = (ed_g3d_manager*)0x0;

		if (bIsNewManager) {
			gNbG3D = gNbG3D + -1;
			pOutManager = (ed_g3d_manager*)0x0;
		}
	}
	else {
		pOutManager->fileBufferStart = reinterpret_cast<GXD_FileHeader*>(pFileBuffer);
		pOutManager->fileLengthA = fileLength;
		pOutManager->field_0x4 = (char*)0x0;

		if ((pFileHeader->flags & 1) == 0) {
			pFileBodyA = reinterpret_cast<char*>(pFileHeader + 1);

			if ((flags & 2) == 0) {
				// Unused currently.
			}
			else {
				// Unused currently.
			}

			ed3DPrepareG3D(true, pOutManager, pTextureManager, unknown);
		}
		else {
			pFileBodyA = reinterpret_cast<char*>(pFileHeader + 1);

			ed3DPrepareG3DManageStruct(pOutManager, pFileBodyA, fileLength + -0x10, pFileBodyA, fileLength + -0x10);
			ed3DPreparePointer(pFileBodyA, fileLength + -0x10, pFileBodyA, fileLength + -0x10);

			pOutManager->GEOM = (ed_Chunck*)0x0;
			pOutManager->field_0x4 = (char*)0x0;
			pOutManager->field_0xc = 0;
			pOutManager->fileLengthB = pOutManager->fileLengthA;

			ed3DPrepareG3D(true, pOutManager, pTextureManager, unknown);
		}
	}

#ifdef PLATFORM_WIN
	//onMeshLoadedDelegate(pOutManager, ObjectNaming::CopyObjectName());
#endif

	return pOutManager;
}

// Finds the number of chunks between two pointers, based off the size of the chunks.
uint edChunckGetNb(void* pStart, char* pEnd)
{
	uint nbChunks;

	ed_Chunck* pChunck = reinterpret_cast<ed_Chunck*>(pStart);

	nbChunks = 0;

	if ((pEnd != (char*)0x0) && (pEnd <= reinterpret_cast<char*>(pChunck))) {
		pChunck = (ed_Chunck*)0x0;
	}

	while (pChunck != (ed_Chunck*)0x0) {
		nbChunks = nbChunks + 1;

		if ((pEnd == (char*)0x0) || ((reinterpret_cast<char*>(pChunck) + pChunck->size) < pEnd)) {
			pChunck = reinterpret_cast<ed_Chunck*>(reinterpret_cast<char*>(pChunck) + pChunck->size);
		}
		else {
			pChunck = (ed_Chunck*)0x0;
		}
	}

	return nbChunks & 0xffff;
}

ed_g3d_hierarchy* ed3DG3DHierarchyGetFromIndex(ed_g3d_manager* pMeshInfo, int count)
{
	ed_hash_code* pMVar1;

	pMVar1 = (ed_hash_code*)(pMeshInfo->HALL + 2);

	for (; count != 0; count = count + -1) {
		pMVar1 = pMVar1 + 1;
	}

	char* pLoaded = (char*)LOAD_SECTION(pMVar1->pData);

	return (ed_g3d_hierarchy*)(pLoaded + 0x10);
}