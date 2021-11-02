#pragma once

#include <utility>
#include <vector>
#include <algorithm>
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

template<typename C, typename E>
void insertUnique(C& cont, E&& e)
{
	if (std::find(cont.begin(), cont.end(), e) == cont.end())
		cont.insert(cont.end(), std::forward<E>(e));
}


class VectorAny
{
public:

	template<typename T>
	void push_back(T&& e = T{})
	{
		auto const i = arr.size();
		arr.resize(arr.size() + sizeof(e));
		new (&arr[i]) T(std::forward<T>(e));
	}

	template<typename T>
	T& get(size_t i)
	{
		return *static_cast<T*>(arr.data())[i];
	}

	template<typename T>
	T const& get(size_t i) const
	{
		return *static_cast<T*>(arr.data())[i];
	}

	void resizeRaw(size_t n)
	{
		arr.resize(n);
	}

	template<typename T>
	void resize(size_t n)
	{
		arr.resize(sizeof(T) * n);
	}
	
	size_t sizeRaw() const
	{
		return arr.size();
	}
	
private:
	std::vector<std::byte> arr;
};