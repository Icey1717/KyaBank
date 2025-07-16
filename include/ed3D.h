#pragma once

#include "Types.h"

#include <string>
#include <cstring>

#define HASH_CODE_HASH 0x48534148
#define HASH_CODE_MBNK 0x4b4e424d

#define HASH_CODE_CDQA 0x41514443
#define HASH_CODE_CDQU 0x55514443
#define HASH_CODE_CDOA 0x414f4443
#define HASH_CODE_CDOC 0x434f4443

#define HASH_CODE_HIER 0x52454948

struct GXD_FileHeader
{
	undefined4 field_0x0;
	uint flags;
	undefined4 field_0x8;
	uint hash;
};

struct alignas(16) ed_Chunck
{
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

struct ed_g2d_manager
{
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

struct ed_g2d_bitmap
{
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

struct ed_hash_code
{
	Hash_8 hash;
	int pData; // char*
	char _pad[4];
};

struct ed_g2d_layer
{
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

struct ed_g2d_material
{
	byte nbLayers;
	undefined field_0x1;
	ushort flags;
	int pDMA_Material; // ed_dma_material*
	int pCommandBufferTexture; // RenderCommand*
	int commandBufferTextureSize;
	int aLayers[];
};

struct ed_g2d_texture
{
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

struct ed_dma_material
{
	ed_g2d_material* pMaterial;
	ed_g2d_bitmap* pBitmap;
};

union DMA_Matrix
{
	int pDMA_Matrix;

	struct
	{
		ushort flagsA;
		ushort flagsB;
	};
};

struct ed_3d_strip
{
	uint flags;
	short materialIndex;
	short cachedIncPacket;
	int vifListOffset;
	int pNext; // ed_3d_strip*
	edF32VECTOR4 boundingSphere;
	int pSTBuf; // char*
	int pColorBuf; // _rgba*
	int pVertexBuf; // edVertex*
	int pNormalBuf; // char*
	short shadowCastFlags;
	short shadowReceiveFlags;
	DMA_Matrix pDMA_Matrix; // ed_dma_matrix*
	byte field_0x38;
	byte primListIndex;
	short meshCount;
	int pBoundSpherePkt; // ed_Bound_Sphere_packet*
};

static_assert(sizeof(ed_3d_strip) == 0x40, "Invalid ed_3d_strip size");

struct ed3DLod
{
	int pObj; // char*
	short renderType;
	short sizeBias;
};

static_assert(sizeof(ed3DLod) == 0x8, "Invalid ed3DLod size");

struct ed_g3d_hierarchy
{
	edF32MATRIX4 transformA;
	edF32MATRIX4 transformB;
	Hash_8 hash;
	byte field_0x88;
	byte field_0x89;
	ushort bRenderShadow;
	uint pShadowAnimMatrix; // edF32MATRIX4*
	int pLinkTransformData; // ed_3d_hierarchy*
	int field_0x94; // undefined*
	int pTextureInfo; // undefined*
	ushort lodCount;
	ushort flags_0x9e;
	int pHierarchySetup; // ed_3d_hierarchy_setup*
	int pMatrixPkt; // edpkt_data*
	int pAnimMatrix; // edF32MATRIX4*
	short subMeshParentCount_0xac;
	byte desiredLod;
	char GlobalAlhaON;
	ed3DLod aLods[];
};

static_assert(sizeof(ed_g3d_hierarchy) == 0xb0, "Invalid ed_g3d_hierarchy size");

struct TextureLibrary
{

};

struct ed_g3d_object
{
	undefined field_0x10;
	undefined field_0x11;
	undefined field_0x12;
	undefined field_0x13;
	int stripCount;
	edF32VECTOR4 boundingSphere;
	undefined field_0x28;
	undefined field_0x29;
	undefined field_0x2a;
	undefined field_0x2b;
	int p3DData; //ed_3d_strip* or ed_3d_sprite* 
};

static_assert(sizeof(ed_g3d_object) == 0x20, "Invalid ed_g3d_object size");

struct MeshData_CSTA
{
	edF32VECTOR3 field_0x20;
	undefined field_0x2c; // pad
	undefined field_0x2d; // pad
	undefined field_0x2e; // pad
	undefined field_0x2f; // pad
	edF32VECTOR4 worldLocation;
};

static_assert(sizeof(MeshData_CSTA) == 0x20, "MeshData_CSTA size is incorrect");

struct ClusterDetails
{
	int field_0x20; // int*
	int field_0x24; // int*
	ushort count_0x28; // int*
	ushort clusterHierCount;
	undefined field_0x2c;
	undefined field_0x2d;
	ushort spriteCount;
};

static_assert(sizeof(ClusterDetails) == 0x10);

struct ed_g3d_cluster
{
	ushort aClusterStripCounts[5];
	ushort field_0x1a;
	ushort flags_0x1c;
	undefined2 field_0x1e;
	ClusterDetails clusterDetails;
	int field_0x30; // int*
	int field_0x34; // int*
	int field_0x38; // uint*
	int field_0x3c; // uint*
	int pMBNK; // char*
	undefined field_0x44;
	undefined field_0x45;
	undefined field_0x46;
	undefined field_0x47;
	int p3DStrip; // ed_3d_strip*
	int p3DSprite; // ed_3d_sprite*
};

static_assert(sizeof(ed_g3d_cluster) == 0x40);

struct ed_g3d_manager
{
	GXD_FileHeader* fileBufferStart;
	char* field_0x4;
	int fileLengthA;
	undefined4 field_0xc;
	ed_Chunck* OBJA;
	ed_Chunck* LIA;
	ed_Chunck* CAMA;
	ed_Chunck* SPRA;
	ed_Chunck* HALL;
	ed_Chunck* CSTA;
	ed_Chunck* GEOM;
	ed_Chunck* MBNA;
	ed_Chunck* INFA;
	int fileLengthB;
	ed_Chunck* CDZA;
	ed_Chunck* ANMA;
};

struct edVertexNormal
{
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t pad;
};

int ed3DG2DGetG2DNbMaterials(ed_Chunck* pChunck);
ed_g2d_material* ed3DG2DGetG2DMaterialFromIndex(ed_g2d_manager* pManager, int index);
ed_g2d_manager* ed3DInstallG2D(char* pFileBuffer, int fileLength, int* outInt, ed_g2d_manager* pManager, int param_5);
ed_Chunck* edChunckGetFirst(void* pBuffStart, char* pBuffEnd);
uint edChunckGetNb(void* pStart, char* pEnd);
ed_g3d_hierarchy* ed3DG3DHierarchyGetFromIndex(ed_g3d_manager* pMeshInfo, int count);