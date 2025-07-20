#include "Winding.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm> // for std::swap
#include "renderer.h"

void Winding::FixInconsistentWinding(Renderer::CompatibilityMeshBuffer& mesh)
{
	// Use the precomputed normal data to determine the winding of triangles.

	auto* indices = mesh.index.buff;
	size_t indexCount = mesh.index.tail;

	auto* vertices = mesh.vertex.buff;

	for (size_t i = 0; i + 2 < indexCount; i += 3)
	{
		uint16_t i0 = indices[i];
		uint16_t i1 = indices[i + 1];
		uint16_t i2 = indices[i + 2];

		const auto& v0 = vertices[i0];
		const auto& v1 = vertices[i1];
		const auto& v2 = vertices[i2];

		// Positions
		glm::vec3 p0(v0.XYZFlags.fXYZ[0], v0.XYZFlags.fXYZ[1], v0.XYZFlags.fXYZ[2]);
		glm::vec3 p1(v1.XYZFlags.fXYZ[0], v1.XYZFlags.fXYZ[1], v1.XYZFlags.fXYZ[2]);
		glm::vec3 p2(v2.XYZFlags.fXYZ[0], v2.XYZFlags.fXYZ[1], v2.XYZFlags.fXYZ[2]);

		// Compute face normal
		glm::vec3 edge1 = p1 - p0;
		glm::vec3 edge2 = p2 - p0;
		glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

		// Normals
		glm::vec3 n0(v0.normal.fNormal[0], v0.normal.fNormal[1], v0.normal.fNormal[2]);
		glm::vec3 n1(v1.normal.fNormal[0], v1.normal.fNormal[1], v1.normal.fNormal[2]);
		glm::vec3 n2(v2.normal.fNormal[0], v2.normal.fNormal[1], v2.normal.fNormal[2]);

		glm::vec3 avgNormal = glm::normalize(n0 + n1 + n2);

		float dot = glm::dot(faceNormal, avgNormal);

		if (dot < 0.0f)
		{
			// Flip triangle winding
			std::swap(indices[i + 1], indices[i + 2]);
		}
	}
}
