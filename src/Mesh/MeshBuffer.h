#pragma once

#include "tinygltf/tiny_gltf.h"

#include <assert.h>
#include <vector>

namespace Renderer
{
	struct SimpleMesh;
}

// Populates a tinygltf buffer object with data from a SimpleMesh object.
class MeshBuffer
{
public:
	MeshBuffer(Renderer::SimpleMesh* pSimpleMesh);

	void AttachToModel(tinygltf::Model& model);

	int GetPositionAccessorIndex() const { assert(positionAccessorIndex != -1); return positionAccessorIndex; }
	int GetIndexAccessorIndex() const { assert(indexAccessorIndex != -1); return indexAccessorIndex; }
	int GetNormalAccessorIndex() const { assert(normalAccessorIndex != -1); return normalAccessorIndex; }
	int GetUvAccessorIndex() const { assert(uvAccessorIndex != -1); return uvAccessorIndex; }

private:
	struct OffsetData
	{
		size_t positionOffset = 0;
		size_t indexOffset = 0;
		size_t normalOffset = 0;
		size_t uvOffset = 0;
	};

	void PopulateBuffer(Renderer::SimpleMesh* pSimpleMesh);

	std::vector<unsigned char> bufferData;

	OffsetData offsetData;

	// Tiny GLTF buffer and views.
	tinygltf::Buffer buffer;

	tinygltf::BufferView positionView;
	tinygltf::Accessor positionAccessor;

	tinygltf::BufferView indexView;
	tinygltf::Accessor indexAccessor;

	tinygltf::BufferView normalView;
	tinygltf::Accessor normalAccessor;

	tinygltf::BufferView uvView;
	tinygltf::Accessor uvAccessor;

	int positionAccessorIndex = -1;
	int indexAccessorIndex = -1;
	int normalAccessorIndex = -1;
	int uvAccessorIndex = -1;
};