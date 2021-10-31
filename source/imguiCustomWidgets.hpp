#pragma once

#include <string_view>
#include <glm/vec3.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

inline bool guiVec3(std::string_view label, glm::vec3& value, glm::vec3 resetValue = glm::vec3{} )
{
	bool used = false;
	
	ImGui::PushID(label.data());
	
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 100.0f);
	ImGui::Text(label.data());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 2.0f});
	float const lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 const buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
	
	if (ImGui::Button("X", buttonSize))
	{
		value.x = resetValue.x;
		used = true;
	}
	
	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	used |= ImGui::DragFloat("##X", &value.x, 0.1f);
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	
	if (ImGui::Button("Y", buttonSize))
	{
		value.y = resetValue.y;
		used = true;
	}
	
	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	used |= ImGui::DragFloat("##Y", &value.y, 0.1f);
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
	
	if (ImGui::Button("Z", buttonSize))
	{
		value.z = resetValue.z;
		used = true;
	}
	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	used |= ImGui::DragFloat("##Z", &value.z, 0.1f);
	ImGui::PopItemWidth();

	ImGui::Columns(1);
	ImGui::PopStyleVar();
	ImGui::PopID();

	return used;
}
