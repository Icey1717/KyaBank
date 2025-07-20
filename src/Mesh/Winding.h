#pragma once

namespace Renderer
{
	struct CompatibilityMeshBuffer;
}

namespace Winding
{
	// Winding on the PS2 can be inconsistent, this function will fix the winding of triangles in a mesh buffer.
	void FixInconsistentWinding(Renderer::CompatibilityMeshBuffer& mesh);
}