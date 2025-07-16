#pragma once

#include <filesystem>

struct ed_g2d_manager;

namespace Texture
{
	bool Convert(std::filesystem::path srcPath, std::filesystem::path dstPath);
	bool Install(std::filesystem::path srcPath, ed_g2d_manager& manager, char** ppFileBuffer);
}