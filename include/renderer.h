#pragma once

#include "Types.h"
#include "GifReg.h"

struct ImageData {
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

struct TextureRegisters {
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
}

template<typename... Args>
class Multidelegate {
public:
	using Delegate = std::function<void(Args...)>;

	void operator+=(const Delegate& delegate) {
		delegates.push_back(delegate);
	}

	void operator-=(const Delegate& delegate) {
		delegates.erase(std::remove(delegates.begin(), delegates.end(), delegate), delegates.end());
	}

	void operator()(Args... args) const {
		for (const auto& delegate : delegates) {
			delegate(args...);
		}
	}

	void RemoveAll() {
		delegates.clear();
	}

private:
	std::vector<Delegate> delegates;
};

inline Multidelegate<ed_g2d_manager*, std::string>& ed3DGetTextureLoadedDelegate()
{
	static Multidelegate<ed_g2d_manager*, std::string> delegate;
	return delegate;
}