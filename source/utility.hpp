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

template<typename T>
void mergeVectors(std::vector<T>& a, std::vector<T> const& b)
{
	a.insert(a.end(), b.begin(), b.end());
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;