#pragma once

#include <utility>
#include <vector>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <span>

constexpr bool isPowerOf2(size_t n)
{
	return (n > 0 && ((n & (n - 1)) == 0));
}

constexpr size_t align(size_t x, size_t a)
{
	assert(isPowerOf2(a));
	return (x + (a - 1)) & ~(a - 1);
}

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

template<size_t alignement = 1>
class VectorAny
{
public:

	template<typename T>
	void push_back(T&& e = T{})
	{
		auto const i = arr.size();
		arr.resize(arr.size() + alignedElementSize<T>());
		new (&arr[i]) T(std::forward<T>(e));
	}

	template<typename T>
	T& get(size_t i)
	{
		return *reinterpret_cast<T*>(arr.data() + alignedElementSize<T>());
	}

	template<typename T>
	T const& get(size_t i) const
	{
		return *reinterpret_cast<T const*>(arr.data() + alignedElementSize<T>());
	}

	void resizeRaw(size_t n)
	{
		arr.resize(n);
	}

	template<typename T>
	void resize(size_t n)
	{
		arr.resize(alignedElementSize<T>() * n);
	}
	
	size_t sizeRaw() const noexcept
	{
		return arr.size();
	}

	template<typename T>
	size_t size() const noexcept
	{
		return arr.size() / alignedElementSize<T>();
	}
	
	std::byte const* data() const noexcept
	{
		return arr.data();
	}
	
private:
	
	template<typename T>
	constexpr static size_t alignedElementSize() noexcept
	{
		return align(sizeof(T), alignement);
	}
	
	std::vector<std::byte> arr;
};