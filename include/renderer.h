#pragma once

#include "Types.h"
#include "GifReg.h"

#include <vector>
#include <string>
#include "MultiDelegate.h"

struct ed_g2d_manager;
struct ed_g3d_manager;

struct ImageData
{
	void* pImage = nullptr;
	uint32_t canvasWidth = 0;
	uint32_t canvasHeight = 0;
	uint32_t bpp = 0;
	uint32_t maxMipLevel = 0;

	// Full commands for uploading the image.
	GIFReg::GSBitBltBuf bitBltBuf;
	GIFReg::GSTrxPos trxPos;
	GIFReg::GSTrxReg trxReg;
};

struct TextureRegisters
{
	GIFReg::GSClamp clamp;
	GIFReg::GSTex tex;
	GIFReg::GSTest test;
	GIFReg::GSColClamp colClamp;
	GIFReg::GSAlpha alpha;
};

struct CombinedImageData
{
	// One bitmap per mip level.
	std::vector<ImageData> bitmaps;
	ImageData palette;
	TextureRegisters registers;
};

namespace Renderer
{
	inline void BindTexture(struct SimpleTexture* pTexture) {}

	using InUseTextureList = std::vector<SimpleTexture*>;
	inline const InUseTextureList& GetInUseTextures() { static InUseTextureList textures; return textures; }

	struct RendererObject {
		RendererObject(std::string inName) : name(inName) {}
		const std::string& GetName() const { return name; }

	private:
		std::string name;
	};

	struct SimpleTexture : public RendererObject
	{
		struct Details {
			int layerIndex = 0;
			int materialIndex = 0;

			size_t layerCount = 0;
			size_t materialCount = 0;

			uint64_t hash;
		};

		SimpleTexture(const std::string inName, const Details& inDetails, const TextureRegisters& inTextureRegisters)
			: RendererObject(inName)
			, details(inDetails)
			, registers(inTextureRegisters)
			, width(0)
			, height(0)
		{
		}

		void CreateRenderer(const CombinedImageData& inImageData);

		int GetLayerIndex() const { return details.layerIndex; }
		int GetMaterialIndex() const { return details.materialIndex; }

		size_t GetLayerCount() const { return details.layerCount; }
		size_t GetMaterialCount() const { return details.materialCount; }

		uint64_t GetHash() const { return details.hash; }

		const TextureRegisters& GetTextureRegisters() const { return registers; }

		const std::vector<unsigned char>& GetUploadedImageData() const { return uploadedImageData; }

		int GetWidth() const { return width; }
		int GetHeight() const { return height; }
	private:

		//PS2::GSSimpleTexture* pRenderer;
		Details details;
		TextureRegisters registers;

		std::vector<unsigned char> uploadedImageData;

		int width;
		int height;
	};

	struct alignas(32) GSVertexUnprocessed
	{
		struct
		{
			int32_t ST[2];
			float Q;
			float _pad;
		} STQ;

		uint32_t RGBA[4];

		struct Vertex
		{
			union {
				float fXYZ[3];
				int32_t iXYZ[3];
			};
			uint32_t flags;
		} XYZFlags;
	};

	struct alignas(32) GSVertexUnprocessedNormal : public GSVertexUnprocessed
	{
		union Normal {
			float fNormal[4];
			int32_t iNormal[4];
		} normal;
	};

	// Just struct compatible with the Mesh library code that can hold vertex and index data.
	struct CompatibilityMeshBuffer
	{
		void Init(const int vertexCount, const int indexCount);

		struct Index
		{
			uint16_t* buff{};
			size_t tail{};
			size_t maxcount{};
		};

		struct Vertex
		{
			GSVertexUnprocessedNormal* buff{};
			size_t head{}, tail{}, next{}, maxcount{}; // head: first vertex, tail: last vertex + 1, next: last indexed + 1
			size_t xy_tail{};
			uint64_t xy[4]{};
			float fxyz[4][3]{};
		};

		Vertex vertex;
		Index index;

	private:
		// Not time critical, vector is fine.
		std::vector<GSVertexUnprocessedNormal> vertices;
		std::vector<uint16_t> indices;
	};

	struct SimpleMesh : public RendererObject
	{
		SimpleMesh(const std::string inName, const GIFReg::GSPrim& inPrim)
			: RendererObject(inName)
			, prim(inPrim)
		{
		}
		const GIFReg::GSPrim& GetPrim() const { return prim; }
		CompatibilityMeshBuffer& GetVertexBufferData() { return vertexBufferData; }

		GIFReg::GSPrim prim;
		CompatibilityMeshBuffer vertexBufferData;
	};

	// Stub for compatibility with existing code.
	inline void RenderMesh(SimpleMesh*, ushort) {}

	void KickVertex(GSVertexUnprocessedNormal& vtx, GIFReg::GSPrim primReg, uint32_t skip, CompatibilityMeshBuffer& drawBuffer);
}

inline Multidelegate<ed_g2d_manager*, std::string>& ed3DGetTextureLoadedDelegate()
{
	static Multidelegate<ed_g2d_manager*, std::string> delegate;
	return delegate;
}

inline Multidelegate<ed_g3d_manager*, std::string>& ed3DGetMeshLoadedDelegate()
{
	static Multidelegate<ed_g3d_manager*, std::string> delegate;
	return delegate;
}