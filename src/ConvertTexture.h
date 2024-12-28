#pragma once

#include <filesystem>

namespace Texture
{
	bool Convert(std::filesystem::path srcPath, std::filesystem::path dstPath);
}