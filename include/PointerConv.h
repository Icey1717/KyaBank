#pragma once

#include <optional>

namespace PointerConv
{
	int AddTextureSectionValue(void* value);
	std::optional<void*> ResolveTextureSectionKey(int key);
	void* ResolveTextureSectionKeyChecked(int key);
}

#define STORE_SECTION(a) PointerConv::AddTextureSectionValue(a)
#define LOAD_SECTION(a) PointerConv::ResolveTextureSectionKeyChecked(a)

#define LOAD_SECTION_CAST(type, a) reinterpret_cast<type>(PointerConv::ResolveTextureSectionKeyChecked(a))