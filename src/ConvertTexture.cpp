#include "ConvertTexture.h"

#include <fstream>

#include "ed3D.h"
#include "KyaTexture/src/Texture.h"
#include "renderer.h"
#include "TextureUpload/src/TextureUpload.h"
#include "pizza-png/src/Image.h"

static bool WriteBitmapFile(std::string srcFileName, std::filesystem::path dstPath, Renderer::SimpleTexture* pSimpleTexture, int textureIndex)
{
	const std::string filename = srcFileName + "_" + std::to_string(pSimpleTexture->GetMaterialIndex()) + "_" + std::to_string(pSimpleTexture->GetLayerIndex()) + "_" + std::to_string(textureIndex) + ".png";
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
		outImage.addPixel(pSimpleTexture->GetUploadedImageData()[i + 0], pSimpleTexture->GetUploadedImageData()[i + 1], pSimpleTexture->GetUploadedImageData()[i + 2], pSimpleTexture->GetUploadedImageData()[i + 3]);
	}

	const std::string pngData = outImage;
	file.write(pngData.c_str(), pngData.size());
	file.close();

	return true;
}

static bool ConvertFile(std::filesystem::path rootPath, std::filesystem::path srcPath, std::filesystem::path dstPath)
{
	ed_g2d_manager manager;

	printf("Converting: %s\n", srcPath.string().c_str());

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
	char* pFileBuffer = new char[fileSize];
	file.read(pFileBuffer, fileSize);
	file.close();

	// Process the read g2d file into a struct that lays out all the data for us (same as the engine would).
	int outInt;
	ed3DInstallG2D(pFileBuffer, fileSize, &outInt, &manager, 0);


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

	int textureIndex = 0;

	for (auto& material : texture.GetMaterials()) {
		printf("Processing material, layers: %d\n", static_cast<int>(material.layers.size()));

		for (auto& layer : material.layers) {
			printf("Processing layer, textures: %d\n", static_cast<int>(layer.textures.size()));

			for (auto& texture : layer.textures) {
				WriteBitmapFile(srcFileNameNoExt, dstPath, texture.pSimpleTexture, textureIndex);
				textureIndex++;
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
