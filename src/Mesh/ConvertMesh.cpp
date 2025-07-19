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

	void AddMaterial(std::string filename)
	{
		tinygltf::Image image;
		image.uri = filename;
		image.name = "MyDiffuse";
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

		model.materials.push_back(material);
	}

	void AddPrimitive(Renderer::SimpleMesh* pSimpleMesh)
	{
		tinygltf::Primitive primitive = primitiveList.AddPrimitive(pSimpleMesh, model);

		int materialIndex = static_cast<int>(model.materials.size() - 1);
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

static bool ConvertFile(std::filesystem::path rootPath, std::filesystem::path srcPath, std::filesystem::path dstPath)
{
	std::vector<std::string> convertedList;

	Texture::GetTextureConvertedDelegate() +=
		[&convertedList](const std::string& textureName)
		{
			convertedList.push_back(textureName);
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
	InstallTexture(textureManager, &pTextureFileBuffer, srcPath);

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

				for (auto& lod : hierarchy.lods) {
					// Bulid the output name.
					std::string outputName = g3d.GetName() + "_H" + std::to_string(hierarchyIndex) + "_L" + std::to_string(lodIndex) + ".gltf";

					Model model;

					model.AddMaterial(convertedList[0]);

					for (auto& strip : lod.object.strips) {
						if (strip.pSimpleMesh) {
							model.AddPrimitive(strip.pSimpleMesh.get());
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