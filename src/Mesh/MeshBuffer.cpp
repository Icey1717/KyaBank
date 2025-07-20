#include "MeshBuffer.h"

#include "renderer.h"

#include "Winding.h"

#include <stdexcept>

#define int12_to_float(x)	(float)((float)x * 0.000244140625f)

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

void MeshBuffer::PopulateBuffer(Renderer::SimpleMesh* pSimpleMesh)
{
	auto& meshBuffer = pSimpleMesh->GetVertexBufferData();

	Winding::FixInconsistentWinding(meshBuffer);

	offsetData.positionOffset = bufferData.size();

	// Vertices.
	for (size_t i = 0; i < meshBuffer.vertex.tail; ++i)
	{
		const Renderer::GSVertexUnprocessedNormal& vertex = meshBuffer.vertex.buff[i];

		float xyz[3] = {
			vertex.XYZFlags.fXYZ[0],
			vertex.XYZFlags.fXYZ[1],
			vertex.XYZFlags.fXYZ[2],
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
		uint16_t flipped[] = { i0, i1, i2 };
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
			vertex.normal.fNormal[2],
		};
		bufferData.insert(bufferData.end(), reinterpret_cast<const unsigned char*>(normal), reinterpret_cast<const unsigned char*>(normal) + sizeof(float) * 3);
	}

	// UVs.
	
	PadBuffer(bufferData, 4); // Align to 4 bytes
	offsetData.uvOffset = bufferData.size();

	for (size_t i = 0; i < meshBuffer.vertex.tail; ++i)
	{
		const Renderer::GSVertexUnprocessedNormal& vertex = meshBuffer.vertex.buff[i];
		float uv[2] = {
			int12_to_float(vertex.STQ.ST[0]) / vertex.STQ.Q,
			int12_to_float(vertex.STQ.ST[1]) / vertex.STQ.Q,
		};
		bufferData.insert(bufferData.end(), reinterpret_cast<const unsigned char*>(uv), reinterpret_cast<const unsigned char*>(uv) + sizeof(float) * 2);
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

MeshBuffer::MeshBuffer(Renderer::SimpleMesh* pSimpleMesh)
{
	PopulateBuffer(pSimpleMesh);

	// Add Buffer
	buffer.data = bufferData;

	// Add BufferView for position
	positionView.byteOffset = offsetData.positionOffset;
	positionView.byteLength = sizeof(float) * 3 * pSimpleMesh->GetVertexBufferData().vertex.tail; // 3 floats per vertex
	positionView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

	ValidateBufferView(positionView, bufferData);

	float bbMin[3], bbMax[3];
	CalculateBoundingBox(pSimpleMesh, bbMin, bbMax);

	// Add Accessor for position
	positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	positionAccessor.count = pSimpleMesh->GetVertexBufferData().vertex.tail;
	positionAccessor.type = TINYGLTF_TYPE_VEC3;
	positionAccessor.minValues = { bbMin[0], bbMin[1], bbMin[2] };
	positionAccessor.maxValues = { bbMax[0], bbMax[1], bbMax[2] };

	// Add BufferView for indices
	indexView.byteOffset = offsetData.indexOffset;
	indexView.byteLength = sizeof(uint16_t) * pSimpleMesh->GetVertexBufferData().index.tail; // 2 bytes per index
	indexView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

	ValidateBufferView(indexView, bufferData);

	// Add Accessor for indices
	indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	indexAccessor.count = pSimpleMesh->GetVertexBufferData().index.tail - (pSimpleMesh->GetVertexBufferData().index.tail % 3);
	indexAccessor.type = TINYGLTF_TYPE_SCALAR;

	// Add BufferView for normals
	normalView.byteOffset = offsetData.normalOffset;
	normalView.byteLength = sizeof(float) * 3 * pSimpleMesh->GetVertexBufferData().vertex.tail; // 3 floats per normal
	normalView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

	// Add Accessor for normals
	normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	normalAccessor.count = pSimpleMesh->GetVertexBufferData().vertex.tail;
	normalAccessor.type = TINYGLTF_TYPE_VEC3;

	ValidateBufferView(normalView, bufferData);

	// Add BufferView for UVs
	uvView.byteOffset = offsetData.uvOffset;
	uvView.byteLength = sizeof(float) * 2 * pSimpleMesh->GetVertexBufferData().vertex.tail; // 2 floats per UV
	uvView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

	// Add Accessor for UVs
	uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	uvAccessor.count = pSimpleMesh->GetVertexBufferData().vertex.tail;
	uvAccessor.type = TINYGLTF_TYPE_VEC2;

	ValidateBufferView(uvView, bufferData);
}

void MeshBuffer::AttachToModel(tinygltf::Model& model)
{
	const int bufferIndex = static_cast<int>(model.buffers.size());

	positionView.buffer = bufferIndex;
	indexView.buffer = bufferIndex;
	normalView.buffer = bufferIndex;
	uvView.buffer = bufferIndex;

	model.buffers.push_back(buffer);

	positionAccessorIndex = static_cast<int>(model.accessors.size());
	positionAccessor.bufferView = static_cast<int>(model.bufferViews.size());
	model.bufferViews.push_back(positionView);
	model.accessors.push_back(positionAccessor);

	indexAccessorIndex = static_cast<int>(model.accessors.size());
	indexAccessor.bufferView = static_cast<int>(model.bufferViews.size());
	model.bufferViews.push_back(indexView);
	model.accessors.push_back(indexAccessor);

	normalAccessorIndex = static_cast<int>(model.accessors.size());
	normalAccessor.bufferView = static_cast<int>(model.bufferViews.size());
	model.bufferViews.push_back(normalView);
	model.accessors.push_back(normalAccessor);

	uvAccessorIndex = static_cast<int>(model.accessors.size());
	uvAccessor.bufferView = static_cast<int>(model.bufferViews.size());
	model.bufferViews.push_back(uvView);
	model.accessors.push_back(uvAccessor);
}
