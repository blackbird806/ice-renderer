#pragma once

#include <vector>
#include <filesystem>

std::vector<char> readBinFile(std::filesystem::path const& filePath);
