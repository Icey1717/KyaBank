#include "ConvertTexture.h"

#include <fstream>
#include <iostream>

#include "ed3D.h"
#include "KyaTexture/src/Texture.h"
#include "renderer.h"
#include "TextureUpload/src/TextureUpload.h"

#ifdef USE_LODE_PNG
#include "lodepng/lodepng.h"
#endif

#ifdef USE_PIZZA_PNG
#include "pizza-png/src/Image.h"
#endif

static Multidelegate<const std::string&, Renderer::SimpleTexture*> gOnTextureConverted;

static void FixAlpha(std::vector<unsigned char>& imageData)
{
	for (int i = 0; i < imageData.size(); i += 4)
	{
		float fPixelColor = static_cast<float>(imageData[i + 3]);
		fPixelColor = fPixelColor / (128.0f / 255.0f);

		if (fPixelColor > 255.0f) {
			fPixelColor = 255.0f;
		}
		else if (fPixelColor < 0.0f) {
			fPixelColor = 0.0f;
		}

		imageData[i + 3] = static_cast<unsigned char>(fPixelColor);
	}
}

static bool WriteBitmapFile(std::string srcFileName, std::filesystem::path dstPath, Renderer::SimpleTexture* pSimpleTexture)
{
	const std::string filename = srcFileName + "_M" + std::to_string(pSimpleTexture->GetMaterialIndex()) + "_L" + std::to_string(pSimpleTexture->GetLayerIndex()) + ".png";
	const std::string filePath = dstPath.concat(filename).string();
  
	printf("Writing: %s\n", filename.c_str());

	std::vector<unsigned char> imageData = pSimpleTexture->GetUploadedImageData();
	FixAlpha(imageData);

#ifdef USE_LODE_PNG
	std::vector<unsigned char> png;
	unsigned error = lodepng::encode(png, imageData, pSimpleTexture->GetWidth(), pSimpleTexture->GetHeight());

	if (!error)
	{
		lodepng::save_file(png, filePath);
		gOnTextureConverted(filename, pSimpleTexture);
	}

	if (error)
	{
		std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		return false;
	}
#endif

#ifdef USE_PIZZA_PNG
	const std::filesystem::path filePath = dstPath.concat(filename);

	printf("Writing: %s\n", filename.c_str());

	std::ofstream file(dstPath, std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}
	MaratTanalin::PizzaPNG::Image outImage(pSimpleTexture->GetWidth(), pSimpleTexture->GetHeight());

	for (int i = 0; i < pSimpleTexture->GetUploadedImageData().size(); i += 4)
	{
		std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		return false;
	}

	const std::string pngData = outImage;
	file.write(pngData.c_str(), pngData.size());
	file.close();
#endif
	return true;
}

static bool ConvertFile(std::filesystem::path rootPath, std::filesystem::path srcPath, std::filesystem::path dstPath)
{
	ed_g2d_manager manager;
	char* pFileBuffer = nullptr;

	printf("Converting: %s\n", srcPath.string().c_str());

	Texture::Install(srcPath, manager, &pFileBuffer);

	const std::string srcFileName = srcPath.filename().string();
	const auto srcFileNameNoExt = srcFileName.substr(0, srcFileName.length() - srcPath.extension().string().length());

	// Get the delta between the root and src path
	std::filesystem::path relativePath = srcPath.lexically_relative(rootPath);

	// Remove the filename from srcPath
	relativePath = relativePath.replace_filename("");

	// combine it with the dstPath
	dstPath = dstPath / relativePath;

	if (!std::filesystem::exists(dstPath)) {
		std::filesystem::create_directories(dstPath);
	}

	Renderer::Kya::G2D texture(&manager, srcFileNameNoExt);

	printf("Found %d materials\n", static_cast<int>(texture.GetMaterials().size()));

	for (auto& material : texture.GetMaterials()) {
		printf("Processing material, layers: %d\n", static_cast<int>(material.layers.size()));

		for (auto& layer : material.layers) {
			printf("Processing layer, textures: %d\n", static_cast<int>(layer.textures.size()));

			for (auto& texture : layer.textures) {
				WriteBitmapFile(srcFileNameNoExt, dstPath, texture.pSimpleTexture.get());
			}
		}
	}

	delete[] pFileBuffer;

	return true;
}

static bool ConvertDirectory(std::filesystem::path rootPath, std::filesystem::path srcPath, std::filesystem::path dstPath)
{
	bool bSuccess = true;

	for (const auto& entry : std::filesystem::directory_iterator(srcPath)) {
		if (entry.is_directory()) {
			bSuccess |= ConvertDirectory(rootPath, entry.path(), dstPath);
		}
		else if (entry.path().extension() == ".g2d" || entry.path().extension() == ".G2D")
		{
			bSuccess |= ConvertFile(rootPath, entry.path(), dstPath);
		}
	}

	return bSuccess;
}

bool Texture::Convert(std::filesystem::path srcPath, std::filesystem::path dstPath)
{
	if (dstPath.has_filename()) {
		printf("Output path must be a directory\n");
		return false;
	}

	if (!std::filesystem::exists(srcPath)) {
		printf("Source path does not exist\n");
		return false;
	}

	if (!std::filesystem::exists(dstPath)) {
		std::filesystem::create_directories(dstPath);
	}

	if (srcPath.has_filename()) {
		return ConvertFile(srcPath, srcPath, dstPath);
	}
	else {
		return ConvertDirectory(srcPath, srcPath, dstPath);
	}	

	return true;
}

bool Texture::Install(std::filesystem::path srcPath, ed_g2d_manager& manager, char** ppFileBuffer)
{
	// Try open the file.
	std::ifstream file(srcPath, std::ios::binary);
	if (!file.is_open())
	{
		printf("Failed to open file: %s\n", srcPath.string().c_str());
		return false;
	}

	// Seek to the end to get the file size.
	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	printf("File size: %d\n", static_cast<int>(fileSize));

	// Read the file into a buffer.
	*ppFileBuffer = new char[fileSize];
	file.read(*ppFileBuffer, fileSize);
	file.close();

	// Process the read g2d file into a struct that lays out all the data for us (same as the engine would).
	int outInt;
	ed3DInstallG2D(*ppFileBuffer, fileSize, &outInt, &manager, 0);

	return true;
}

Multidelegate<const std::string&, Renderer::SimpleTexture*>& Texture::GetTextureConvertedDelegate()
{
	return gOnTextureConverted;
}

void Renderer::SimpleTexture::CreateRenderer(const CombinedImageData& imageData)
{
	const ImageData& bitmap = imageData.bitmaps.front();
	const ImageData& palette = imageData.palette;

	if (palette.pImage && palette.canvasWidth) {
		TextureUpload::UploadPalette(reinterpret_cast<uint8_t*>(palette.pImage), palette.bitBltBuf.CMD, palette.trxPos.CMD, palette.trxReg.CMD, imageData.registers.tex.CMD);
	}

	auto* pOut = TextureUpload::UploadTexture(reinterpret_cast<uint8_t*>(bitmap.pImage), bitmap.bitBltBuf.CMD, bitmap.trxPos.CMD, bitmap.trxReg.CMD, imageData.registers.tex.CMD);

	const size_t bufferSize = bitmap.canvasWidth * bitmap.canvasHeight * 4;
	uploadedImageData.resize(bufferSize);
	memcpy(uploadedImageData.data(), pOut, bufferSize);

	width = bitmap.canvasWidth;
	height = bitmap.canvasHeight;
}
