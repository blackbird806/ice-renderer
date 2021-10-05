#pragma once

#include <vector>
#include <filesystem>
#include <span>

std::vector<char> readBinFile(std::filesystem::path const& filePath);

template<typename To, typename From>
std::span<To> toSpan(std::vector<From> const& vec)
{
	return std::span((To*)vec.data(), vec.size() * sizeof(vec[0]));
}
