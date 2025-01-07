#pragma once

#include "Types.h"

#include <string>
#include <cstring>

struct GXD_FileHeader
{
	undefined4 field_0x0;
	uint flags;
	undefined4 field_0x8;
	uint hash;
};

struct alignas(16) ed_Chunck {
	uint hash;
	short field_0x4;
	short field_0x6;
	int size;
	int nextChunckOffset;

	// Debug
	inline std::string GetHeaderString() const {
		// convert hash into chars
		char hashStr[5];
		std::memcpy(hashStr, &hash, 4);
		hashStr[4] = 0;
		return std::string(hashStr);
	}
};

struct ed_g2d_manager {
	GXD_FileHeader* pFileBuffer;
	int textureFileLengthA;
	ed_Chunck* pTextureChunk;
	ed_Chunck* pMATA_HASH;
	ed_Chunck* pT2DA;
	ed_Chunck* pPALL;
	byte field_0x18;
	byte field_0x19;
	byte field_0x1a;
	byte field_0x1b;
	int textureFileLengthB;
	ed_Chunck* pANMA;
	undefined field_0x24;
	undefined field_0x25;
	undefined field_0x26;
	undefined field_0x27;
	undefined field_0x28;
	undefined field_0x29;
	undefined field_0x2a;
	undefined field_0x2b;
	undefined field_0x2c;
	undefined field_0x2d;
	undefined field_0x2e;
	undefined field_0x2f;
};

struct ed_g2d_bitmap {
	ushort width;
	ushort height;
	ushort psm;
	ushort maxMipLevel;
	int pPSX2; //edpkt_data*
};

union Hash_8
{
	char name[8];
	ulong number;

	Hash_8() { number = 0; }
	Hash_8(ulong inNumber) { number = inNumber; }

	// Debug
	inline std::string ToString() const {
		char buff[24];

		// Null-terminate directly in `name`
		char tempName[9];
		memcpy(tempName, name, 8);
		tempName[8] = '\0';

		// Replace newlines with spaces if any exist in `name`
		for (int i = 0; i < 8; i++) {
			if (tempName[i] == '\n') {
				tempName[i] = ' ';
			}
		}

		snprintf(buff, sizeof(buff), "%s(0x%llx)", tempName, number);
		return std::string(buff);
	}
};

struct ed_hash_code {
	Hash_8 hash;
	int pData; // char*
	char _pad[4];
};

struct ed_g2d_layer {
	uint flags_0x0;
	uint flags_0x4;
	undefined field_0x8;
	undefined field_0x9;
	undefined field_0xa;
	undefined field_0xb;
	undefined field_0xc;
	undefined field_0xd;
	undefined field_0xe;
	undefined field_0xf;
	undefined field_0x10;
	undefined field_0x11;
	undefined field_0x12;
	undefined field_0x13;
	undefined field_0x14;
	undefined field_0x15;
	undefined field_0x16;
	undefined field_0x17;
	undefined field_0x18;
	undefined field_0x19;
	undefined field_0x1a;
	byte field_0x1b;
	short bHasTexture;
	ushort paletteId;
	int pTex; // ed_Chunck*
};

struct ed_g2d_material {
	byte nbLayers;
	undefined field_0x1;
	ushort flags;
	int pDMA_Material; // ed_dma_material*
	int pCommandBufferTexture; // RenderCommand*
	int commandBufferTextureSize;
	int aLayers[];
};

struct ed_g2d_texture {
	ed_hash_code hashCode;
	int bHasPalette;
	int pAnimSpeedNormalExtruder; //edF32VECTOR4*
	float field_0x18;
	int pAnimChunck; //ed_Chunck*
};

struct edPSX2Header
{
	int pPkt;
	int size;
};

struct ed_dma_material {
	ed_g2d_material* pMaterial;
	ed_g2d_bitmap* pBitmap;
};

struct TextureLibrary
{

};

int ed3DG2DGetG2DNbMaterials(ed_Chunck* pChunck);
ed_g2d_material* ed3DG2DGetG2DMaterialFromIndex(ed_g2d_manager* pManager, int index);
ed_g2d_manager* ed3DInstallG2D(char* pFileBuffer, int fileLength, int* outInt, ed_g2d_manager* pManager, int param_5);