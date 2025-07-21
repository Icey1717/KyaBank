#include "ConvertMesh.h"

#include <fstream>
#include <iostream>

#include "ed3D.h"
#include "renderer.h"

#include "ConvertTexture.h"
#include "Mesh.h"

#include "MeshBuffer.h"

#include "tinygltf/tiny_gltf.h"

class PrimitiveList
{
public:
	tinygltf::Primitive AddPrimitive(Renderer::SimpleMesh* pSimpleMesh, tinygltf::Model& model)
	{
		assert(pSimpleMesh->GetVertexBufferData().index.tail % 3 == 0);

		MeshBuffer& meshBuffer = meshBuffers.emplace_back(pSimpleMesh);
		meshBuffer.AttachToModel(model);

		tinygltf::Primitive primitive;
		primitive.attributes["POSITION"] = meshBuffer.GetPositionAccessorIndex();
		primitive.attributes["NORMAL"] = meshBuffer.GetNormalAccessorIndex();
		primitive.attributes["TEXCOORD_0"] = meshBuffer.GetUvAccessorIndex();
		primitive.attributes["COLOR_0"] = meshBuffer.GetColorAccessorIndex();
		primitive.indices = meshBuffer.GetIndexAccessorIndex();
		primitive.mode = TINYGLTF_MODE_TRIANGLES;

		return primitive;
	}

private:
	std::vector<MeshBuffer> meshBuffers;
};

class Model
{
public:
	Model()
	{
		node.mesh = 0;
		model.nodes.push_back(node);
		scene.nodes.push_back(0);
		model.scenes.push_back(scene);

		model.defaultScene = 0;
	}

	void AddMaterial(std::string filename, const TextureRegisters& textureRegisters)
	{
		tinygltf::Image image;
		image.uri = filename;
		image.name = filename;
		model.images.push_back(image);
		int imageIndex = static_cast<int>(model.images.size() - 1);

		tinygltf::Texture texture;
		texture.source = imageIndex; // reference the image
		model.textures.push_back(texture);
		int textureIndex = static_cast<int>(model.textures.size() - 1);

		tinygltf::Material material;
		material.name = "TexturedMaterial";

		tinygltf::Parameter param;
		param.json_double_value = { {"index", static_cast<double>(textureIndex)} };

		material.pbrMetallicRoughness.baseColorTexture.index = textureIndex;
		material.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
		material.doubleSided = true;

		// Should probably add some support for alpha here.
		material.alphaMode = "OPAQUE";

		model.materials.push_back(material);
	}

	void AddPrimitive(Renderer::SimpleMesh* pSimpleMesh, const int materialIndex)
	{
		tinygltf::Primitive primitive = primitiveList.AddPrimitive(pSimpleMesh, model);
		primitive.material = materialIndex;
		mesh.primitives.push_back(primitive);
	}

	bool ConvertToGltf(std::filesystem::path dstPath)
	{
		// Push back our mesh to the model.
		model.meshes.push_back(mesh);

		// Write it out.
		tinygltf::TinyGLTF gltf;
		return gltf.WriteGltfSceneToFile(&model, dstPath.string(), false, true, true, false);
	}

private:
	tinygltf::Model model;
	tinygltf::Scene scene;
	tinygltf::Node node;
	tinygltf::Mesh mesh;

	PrimitiveList primitiveList;
};

static bool InstallTexture(ed_g2d_manager& manager, char** pTextureFileBuffer, const std::filesystem::path& srcPath)
{
	// Create a copy of the srcPath for the texture.
	std::filesystem::path srcTexturePath = srcPath;
	srcTexturePath.replace_extension(".g2d");

	if (!Texture::Install(srcTexturePath, manager, pTextureFileBuffer)) {
		return false;
	}

	return true;
}

static bool ConvertTexture(const std::filesystem::path& srcPath, std::filesystem::path dstPath)
{
	// Create a copy of the srcPath for the texture.
	std::filesystem::path srcTexturePath = srcPath;
	srcTexturePath.replace_extension(".g2d");

	return Texture::Convert(srcTexturePath, dstPath);
}

static uint64_t GetTextureHashCode(ed_hash_code* pMaterialBank, int index)
{
	if (pMaterialBank == nullptr || index < 0) {
		return 0;
	}

	// Get the hash code for the material.
	ed_hash_code* pHashCode = pMaterialBank + index;
	if (pHashCode->pData == 0) {
		return 0;
	}

	printf("Material hash: %s\n", pHashCode->hash.ToString().c_str());

	ed_hash_code* pOtherHashCode = LOAD_SECTION_CAST(ed_hash_code*, pHashCode->pData);

	ed_Chunck* pMAT = LOAD_SECTION_CAST(ed_Chunck*, pOtherHashCode->pData);

	ed_g2d_material* pMaterial = reinterpret_cast<ed_g2d_material*>(pMAT + 1);

	ed_Chunck* pLAY = LOAD_SECTION_CAST(ed_Chunck*, pMaterial->aLayers[0]);

	ed_g2d_layer* pLayer = reinterpret_cast<ed_g2d_layer*>(pLAY + 1);

	if (pLayer->bHasTexture == 0) {
		return 0;
	}

	// Get the hash code for the texture.
	ed_Chunck* pTEX = LOAD_SECTION_CAST(ed_Chunck*, pLayer->pTex);

	ed_g2d_texture* pTexture = reinterpret_cast<ed_g2d_texture*>(pTEX + 1);

	return pTexture->hashCode.hash.number;
}

static void ListMaterials(ed_g2d_manager& textureManager)
{
	ed_Chunck* pMATA = textureManager.pMATA_HASH;
	ed_hash_code* pHashCodes = reinterpret_cast<ed_hash_code*>(pMATA + 1);

	int nbMaterials = pMATA->size / sizeof(ed_hash_code) - 1; // -1 for the header

	printf("\nFound %d materials\n", nbMaterials);

	for (int i = 0; i < nbMaterials; ++i) {
		ed_hash_code* pHashCode = pHashCodes + i;
		printf("Material %d: %s\n", i, pHashCode->hash.ToString().c_str());
	}
}

static int GetMaterialIndex(const std::vector<std::uint64_t>& hashes, uint64_t materialHash)
{
	for (int i = 0; i < hashes.size(); ++i) {
		if (hashes[i] == materialHash) {
			return i; // Found the material index
		}
	}

	return -1; // Not found
}

static bool ConvertFile(std::filesystem::path rootPath, std::filesystem::path srcPath, std::filesystem::path dstPath)
{
	struct ConvertedTexture
	{
		std::string name;
		Renderer::SimpleTexture* pSimpleTexture;
	};

	std::unordered_map<uint64_t, ConvertedTexture> convertedList;

	Texture::GetTextureConvertedDelegate().RemoveAll();

	Texture::GetTextureConvertedDelegate() +=
		[&convertedList](const std::string& textureName, Renderer::SimpleTexture* pSimpleTexture)
		{
			ConvertedTexture convertedTexture = { textureName, pSimpleTexture };
			convertedList.emplace(pSimpleTexture->GetHash(), convertedTexture);
		};

	ed_g3d_manager manager;

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

	ed_g2d_manager textureManager;
	char* pTextureFileBuffer = nullptr;
	if (!InstallTexture(textureManager, &pTextureFileBuffer, srcPath)) {
		return false;
	}

	ListMaterials(textureManager);

	// Process the read g3d file into a struct that lays out all the data for us (same as the engine would).
	int outInt;
	ed3DInstallG3D(pFileBuffer, fileSize, 0x0, &outInt, &textureManager, 0xc, &manager);

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

	if (!ConvertTexture(srcPath, dstPath)) {
		printf("Failed to convert texture for %s\n", srcPath.string().c_str());
		return false;
	}

	Renderer::Kya::GetMeshLibraryMutable().AddMesh(&manager, srcFileNameNoExt);

	Renderer::Kya::GetMeshLibrary().ForEach(
		[dstPath, &convertedList](const Renderer::Kya::G3D& g3d)
		{
			// To GLTF.
			int hierarchyIndex = 0;
			for (auto& hierarchy : g3d.GetHierarchies()) {
				int lodIndex = 0;

				ed_Chunck* pMBNK = LOAD_SECTION_CAST(ed_Chunck*, hierarchy.pHierarchy->pTextureInfo);

				ed_hash_code* pHashCode = reinterpret_cast<ed_hash_code*>(pMBNK + 1);

				for (auto& lod : hierarchy.lods) {
					// Bulid the output name.
					std::string outputName = g3d.GetName() + "_H" + std::to_string(hierarchyIndex) + "_L" + std::to_string(lodIndex) + ".gltf";

					printf("\nBeginning conversion of %s\n", outputName.c_str());

					Model model;

					std::vector<uint64_t> hashes;
					for (auto& [hash, data] : convertedList) {
						// Add the material to the model.
						model.AddMaterial(data.name, data.pSimpleTexture->GetTextureRegisters());
						hashes.push_back(hash);
					}

					for (auto& strip : lod.object.strips) {
						if (strip.pSimpleMesh) {
							const int materialIndex = GetMaterialIndex(hashes, GetTextureHashCode(pHashCode, strip.pStrip->materialIndex));
							assert(materialIndex != -1 && "Material index not found in converted list");
							model.AddPrimitive(strip.pSimpleMesh.get(), materialIndex);
						}
					}

					// Create the full path for the output.
					std::filesystem::path outputPath = dstPath / outputName;
					model.ConvertToGltf(outputPath);

					lodIndex++;
				}

				hierarchyIndex++;
			}
		}
	);

	Renderer::Kya::GetMeshLibraryMutable().Clear();

	return true;
}

static bool ConvertDirectory(std::filesystem::path rootPath, std::filesystem::path srcPath, std::filesystem::path dstPath)
{
	bool bSuccess = true;

	for (const auto& entry : std::filesystem::directory_iterator(srcPath)) {
		if (entry.is_directory()) {
			bSuccess |= ConvertDirectory(rootPath, entry.path(), dstPath);
		}
		else if (entry.path().extension() == ".g3d" || entry.path().extension() == ".G3D")
		{
			bSuccess |= ConvertFile(rootPath, entry.path(), dstPath);
		}
	}

	return bSuccess;
}

bool Mesh::Convert(std::filesystem::path srcPath, std::filesystem::path dstPath)
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