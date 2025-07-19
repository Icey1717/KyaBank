#include "ConvertMesh.h"

#include <fstream>
#include <iostream>

#include "ed3D.h"
#include "renderer.h"

#include "ConvertTexture.h"
#include "Mesh.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

static size_t AlignTo(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

static void PadBuffer(std::vector<unsigned char>& bufferData, size_t alignment)
{
	size_t alignedSize = AlignTo(bufferData.size(), alignment);
	if (alignedSize > bufferData.size()) {
		bufferData.resize(alignedSize, 0); // pad with zero bytes
	}
}

struct OffsetData
{
	size_t positionOffset;
	size_t indexOffset;
	size_t normalOffset;
};

static void PopulateBuffer(Renderer::SimpleMesh* pSimpleMesh, std::vector<unsigned char>& bufferData, OffsetData& offsetData)
{
	auto& meshBuffer = pSimpleMesh->GetVertexBufferData();

	offsetData.positionOffset = bufferData.size();

	// Vertices.
	for (size_t i = 0; i < meshBuffer.vertex.tail; ++i)
	{
		const Renderer::GSVertexUnprocessedNormal& vertex = meshBuffer.vertex.buff[i];

		float xyz[3] = {
			vertex.XYZFlags.fXYZ[0],
			vertex.XYZFlags.fXYZ[1],
			vertex.XYZFlags.fXYZ[2]
		};

		bufferData.insert(bufferData.end(), reinterpret_cast<const unsigned char*>(xyz), reinterpret_cast<const unsigned char*>(xyz) + sizeof(float) * 3);
	}

	PadBuffer(bufferData, 4); // Align to 4 bytes
	offsetData.indexOffset = bufferData.size();

	// Indices.
	for (size_t i = 0; i + 2 < meshBuffer.index.tail; i += 3)
	{
		uint16_t i0 = meshBuffer.index.buff[i];
		uint16_t i1 = meshBuffer.index.buff[i + 1];
		uint16_t i2 = meshBuffer.index.buff[i + 2];

		// Flip winding (CW to CCW)
		uint16_t flipped[] = { i0, i2, i1 };
		bufferData.insert(bufferData.end(), reinterpret_cast<unsigned char*>(flipped), reinterpret_cast<unsigned char*>(flipped) + sizeof(flipped));
	}

	PadBuffer(bufferData, 4); // Align to 2 bytes
	offsetData.normalOffset = bufferData.size();

	// Normals.
	for (size_t i = 0; i < meshBuffer.vertex.tail; ++i)
	{
		const Renderer::GSVertexUnprocessedNormal& vertex = meshBuffer.vertex.buff[i];
		float normal[3] = {
			vertex.normal.fNormal[0],
			vertex.normal.fNormal[1],
			vertex.normal.fNormal[2]
		};
		bufferData.insert(bufferData.end(), reinterpret_cast<const unsigned char*>(normal), reinterpret_cast<const unsigned char*>(normal) + sizeof(float) * 3);
	}
}

static void ValidateBufferView(tinygltf::BufferView& bufferView, const std::vector<unsigned char>& bufferData)
{
	if (bufferView.byteOffset + bufferView.byteLength > bufferData.size())
	{
		throw std::runtime_error("BufferView exceeds buffer data size.");
	}
}

static bool CalculateBoundingBox(Renderer::SimpleMesh* pSimpleMesh, float outMin[3], float outMax[3])
{
	const auto& vertexBuffer = pSimpleMesh->GetVertexBufferData().vertex;
	if (vertexBuffer.tail == 0) {
		outMin[0] = outMin[1] = outMin[2] = 0.0f;
		outMax[0] = outMax[1] = outMax[2] = 0.0f;
		return false;
	}

	outMin[0] = outMin[1] = outMin[2] = std::numeric_limits<float>::max();
	outMax[0] = outMax[1] = outMax[2] = std::numeric_limits<float>::lowest();

	for (size_t i = 0; i < vertexBuffer.tail; ++i)
	{
		const Renderer::GSVertexUnprocessedNormal& vertex = vertexBuffer.buff[i];
		for (int j = 0; j < 3; ++j) {
			outMin[j] = std::min(outMin[j], vertex.XYZFlags.fXYZ[j]);
			outMax[j] = std::max(outMax[j], vertex.XYZFlags.fXYZ[j]);
		}
	}

	return true;
}


static void BuildModel(Renderer::SimpleMesh* pSimpleMesh, tinygltf::Model& model)
{
	tinygltf::Scene scene;
	tinygltf::Node node;
	tinygltf::Mesh mesh;
	tinygltf::Primitive primitive;

	assert(pSimpleMesh->GetVertexBufferData().index.tail % 3 == 0);

	std::vector<unsigned char> bufferData;
	OffsetData offsetData;
	PopulateBuffer(pSimpleMesh, bufferData, offsetData);

	// Add Buffer
	tinygltf::Buffer buffer;
	buffer.data = bufferData;
	model.buffers.push_back(buffer);

	// Add BufferView for position
	tinygltf::BufferView positionView;
	positionView.buffer = 0;
	positionView.byteOffset = offsetData.positionOffset;
	positionView.byteLength = sizeof(float) * 3 * pSimpleMesh->GetVertexBufferData().vertex.tail; // 3 floats per vertex
	positionView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
	model.bufferViews.push_back(positionView);

	ValidateBufferView(positionView, bufferData);

	float bbMin[3], bbMax[3];
	CalculateBoundingBox(pSimpleMesh, bbMin, bbMax);
		
	// Add Accessor for position
	tinygltf::Accessor positionAccessor;
	positionAccessor.bufferView = 0;
	positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	positionAccessor.count = pSimpleMesh->GetVertexBufferData().vertex.tail;
	positionAccessor.type = TINYGLTF_TYPE_VEC3;
	positionAccessor.minValues = { bbMin[0], bbMin[1], bbMin[2] };
	positionAccessor.maxValues = { bbMax[0], bbMax[1], bbMax[2] };
	model.accessors.push_back(positionAccessor);

	// Add BufferView for indices
	tinygltf::BufferView indexView;
	indexView.buffer = 0;
	indexView.byteOffset = offsetData.indexOffset;
	indexView.byteLength = sizeof(uint16_t) * pSimpleMesh->GetVertexBufferData().index.tail; // 2 bytes per index
	indexView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	model.bufferViews.push_back(indexView);

	ValidateBufferView(indexView, bufferData);

	// Add Accessor for indices
	tinygltf::Accessor indexAccessor;
	indexAccessor.bufferView = 1;
	indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	indexAccessor.count = pSimpleMesh->GetVertexBufferData().index.tail - (pSimpleMesh->GetVertexBufferData().index.tail % 3);
	indexAccessor.type = TINYGLTF_TYPE_SCALAR;
	model.accessors.push_back(indexAccessor);

	// Add BufferView for normals
	tinygltf::BufferView normalView;
	normalView.buffer = 0;
	normalView.byteOffset = offsetData.normalOffset;
	normalView.byteLength = sizeof(float) * 3 * pSimpleMesh->GetVertexBufferData().vertex.tail; // 3 floats per normal
	normalView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
	model.bufferViews.push_back(normalView);

	// Add Accessor for normals
	tinygltf::Accessor normalAccessor;
	normalAccessor.bufferView = 2;
	normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	normalAccessor.count = pSimpleMesh->GetVertexBufferData().vertex.tail;
	normalAccessor.type = TINYGLTF_TYPE_VEC3;
	model.accessors.push_back(normalAccessor);

	ValidateBufferView(normalView, bufferData);

	primitive.attributes["POSITION"] = 0;
	primitive.attributes["NORMAL"] = 2;
	primitive.indices = 1;
	primitive.mode = TINYGLTF_MODE_TRIANGLES;

	mesh.primitives.push_back(primitive);
	model.meshes.push_back(mesh);

	node.mesh = 0;
	model.nodes.push_back(node);
	scene.nodes.push_back(0);
	model.scenes.push_back(scene);

	model.defaultScene = 0;
}

static bool ConvertToGltf(Renderer::SimpleMesh* pSimpleMesh, std::filesystem::path dstPath)
{
	tinygltf::TinyGLTF gltf;
	tinygltf::Model model;
	BuildModel(pSimpleMesh, model);
	return gltf.WriteGltfSceneToFile(&model, dstPath.string(), false, true, true, false);
}

static bool ConvertFile(std::filesystem::path rootPath, std::filesystem::path srcPath, std::filesystem::path dstPath)
{
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

	{
		// Create a copy of the srcPath for the texture.
		std::filesystem::path srcTexturePath = srcPath;
		srcTexturePath.replace_extension(".g2d");

		char* pTextureFileBuffer = nullptr;

		if (!Texture::Install(srcTexturePath, textureManager, &pTextureFileBuffer)) {
			return false;
		}
	}

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

	Renderer::Kya::GetMeshLibraryMutable().AddMesh(&manager, srcFileNameNoExt);

	Renderer::Kya::GetMeshLibrary().ForEach(
		[dstPath](const Renderer::Kya::G3D& g3d)
		{
			// To GLTF.

			int hierarchyIndex = 0;
			for (auto& hierarchy : g3d.GetHierarchies()) {
				int lodIndex = 0;

				for (auto& lod : hierarchy.lods) {
					int stripIndex = 0;

					for (auto& strip : lod.object.strips) {
						if (strip.pSimpleMesh) {
							// Bulid the output name.
							std::string outputName = g3d.GetName() + "_H" + std::to_string(hierarchyIndex) + "_L" + std::to_string(lodIndex) + "_S" + std::to_string(stripIndex) + ".gltf";

							// Create the full path for the output.
							std::filesystem::path outputPath = dstPath / outputName;

							if (!ConvertToGltf(strip.pSimpleMesh.get(), outputPath)) {
								printf("Failed to convert mesh: %s\n", outputName.c_str());
							}
							else {
								printf("Converted mesh: %s\n", outputName.c_str());
							}
						}

						stripIndex++;
					}

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