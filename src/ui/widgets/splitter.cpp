//
// Created by xabdomo on 6/24/25.
//

#include "splitter.hpp"

#include <numeric>
#include <string>

#include "imgui.h"

void Splitter::Vertical(std::vector<float> &sizes, std::function<void(int)> renderFn, float splitterHeight) {
    const int count = static_cast<int>(sizes.size());
    if (count == 0) return;

    // Normalize sizes
    float total = std::accumulate(sizes.begin(), sizes.end(), 0.0f);
    total = (total <= 0.0f) ? 1.0f : total;
    for (float& s : sizes) s /= total;

    const float width = ImGui::GetContentRegionAvail().x;
    const float totalHeight = ImGui::GetContentRegionAvail().y - (count - 1) * splitterHeight;;
    const float minSize = 0.05f;

    // Compute pixel heights
    std::vector<float> heights(count);
    for (int i = 0; i < count; ++i)
        heights[i] = sizes[i] * totalHeight;

    // Render panels and splitters
    for (int i = 0; i < count; ++i) {
        ImGui::BeginChild(("VPanel" + std::to_string(i)).c_str(), ImVec2(width, heights[i]), true);
        renderFn(i);
        ImGui::EndChild();

        // Draw splitter bar
        if (i < count - 1) {
            ImGui::PushID(i);
            ImVec2 splitterSize(width, splitterHeight);
            ImGui::InvisibleButton("VSplitter", splitterSize);

            // Feedback: highlight during drag
            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetItemRectMin(),
                    ImGui::GetItemRectMax(),
                    ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered)
                );

            // Resize logic
            if (ImGui::IsItemActive()) {
                float delta = ImGui::GetIO().MouseDelta.y / totalHeight;
                float top = sizes[i], bottom = sizes[i + 1];
                if (top + delta >= minSize && bottom - delta >= minSize) {
                    sizes[i] += delta;
                    sizes[i + 1] -= delta;
                }
            }
            ImGui::PopID();
        }
    }
}

void Splitter::Horizontal(std::vector<float> &sizes, std::function<void(int)> renderFn, float splitterWidth) {
    const int count = static_cast<int>(sizes.size());
    if (count == 0) return;

    // Normalize sizes
    float total = std::accumulate(sizes.begin(), sizes.end(), 0.0f);
    total = (total <= 0.0f) ? 1.0f : total;
    for (float& s : sizes) s /= total;

    const float totalWidth = ImGui::GetContentRegionAvail().x - (count - 1) * splitterWidth;
    const float height = ImGui::GetContentRegionAvail().y;
    const float minSize = 0.05f;

    // Compute pixel widths
    std::vector<float> widths(count);
    for (int i = 0; i < count; ++i)
        widths[i] = sizes[i] * totalWidth;

    // Render panels and splitters
    for (int i = 0; i < count; ++i) {
        if (i > 0) ImGui::SameLine();

        ImGui::BeginChild(("Panel" + std::to_string(i)).c_str(), ImVec2(widths[i], height), true);
        renderFn(i);
        ImGui::EndChild();

        // Draw splitter bar
        if (i < count - 1) {
            ImGui::SameLine();
            ImGui::PushID(i);
            ImVec2 splitterSize(splitterWidth, height);
            ImGui::InvisibleButton("Splitter", splitterSize);

            // Feedback: highlight during drag
            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetItemRectMin(),
                    ImGui::GetItemRectMax(),
                    ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered)
                );

            // Resize logic
            if (ImGui::IsItemActive()) {
                float delta = ImGui::GetIO().MouseDelta.x / totalWidth;
                float left = sizes[i], right = sizes[i + 1];
                if (left + delta >= minSize && right - delta >= minSize) {
                    sizes[i] += delta;
                    sizes[i + 1] -= delta;
                }
            }
            ImGui::PopID();
        }
    }
}
