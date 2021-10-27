#pragma once

#include <variant>
#include <optional>
#include <string_view>
#include <imgui/imgui.h>

// should not be shown in inspector
struct IgnoreAttribute
{
	
};

// treat vector as color
struct ColorAttribute
{
	ImGuiColorEditFlags imguiFlags;
};

using Attribute = std::variant<IgnoreAttribute, ColorAttribute>;
using Attributes = std::vector<Attribute>;

template<typename T>
std::optional<T> getAttribute(Attributes const& attributes)
{
	auto const it = std::find_if(attributes.begin(), attributes.end(), [](auto const& e)
		{
			return std::holds_alternative<T>(e);
		});
	
	if (it != attributes.end())
	{
		return { std::get<T>(*it) };
	}
	return {};
}

inline Attributes getAttributesFromName(std::string_view name)
{
	Attributes attr;

	if (name.find("padding") != std::string::npos) {
		attr.push_back(IgnoreAttribute{});
		return attr;
	}
	
	if (name.find("color") != std::string::npos) {
		attr.push_back(ColorAttribute{});
	}
	return attr;
}