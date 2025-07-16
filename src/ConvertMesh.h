#pragma once

#include <filesystem>

namespace Mesh
{
	bool Convert(std::filesystem::path srcPath, std::filesystem::path dstPath);
}