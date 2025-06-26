//
// Created by xabdomo on 6/26/25.
//

#include "scrollable_plot.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>

#include "imgui.h"

void Widgets::PlotLinesScrollable(const char *label, const std::vector<float> &data, int n_visible, int scroll_speed)
{
    if (data.empty()) {
        ImGui::Text("%s: No data", label);
        return;
    }

    static std::unordered_map<std::string, int> scroll_offsets;
    int& offset = scroll_offsets[label]; // persists per plot

    const int total_points = static_cast<int>(data.size());
    n_visible = std::min(n_visible, total_points);

    // Clamp offset to valid range
    offset = std::clamp(offset, 0, std::max(0, total_points - n_visible));

    // Optional slider
    std::string slider_label = std::string("Scroll##") + label;
    ImGui::SliderInt(slider_label.c_str(), &offset, 0, std::max(0, total_points - n_visible));

    // Plot label must be hidden to avoid label text space
    std::string plot_label = std::string("##Plot_") + label;

    // Record cursor position and plot bounding box
    ImVec2 plot_pos = ImGui::GetCursorScreenPos();
    ImVec2 plot_size = ImVec2(ImGui::GetContentRegionAvail().x, 80);

    // Handle scrolling with mouse wheel
    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                   ImGui::IsMouseHoveringRect(plot_pos, ImVec2(plot_pos.x + plot_size.x, plot_pos.y + plot_size.y));

    if (hovered && ImGui::IsWindowFocused()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            offset -= static_cast<int>(wheel * scroll_speed);
            offset = std::clamp(offset, 0, std::max(0, total_points - n_visible));
        }
    }

    // Plot data
    ImGui::PlotLines(
        plot_label.c_str(),
        data.data() + offset,
        n_visible,
        0,
        nullptr,
        0.0f,
        FLT_MAX,
        plot_size
    );

    ImGui::Text("Showing points [%d - %d] of %d", offset, offset + n_visible - 1, total_points);
}
