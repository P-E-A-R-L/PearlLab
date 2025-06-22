#include "layout.hpp"

#include <imgui.h>

#include <unordered_map>

// fixme it doesn't work
void Layout::centered(const std::function<void()> &f)
{
    // Use a static cache to remember the last measured size per callsite (optional: use unique key)
    static std::unordered_map<ImGuiID, ImVec2> cached_sizes;

    ImGuiID id = ImGui::GetID(f.target_type().name()); // or pass a custom label/key if needed

    auto window_size = ImGui::GetWindowSize();

    if (cached_sizes.find(id) != cached_sizes.end())
    {
        // Get group size (after EndGroup)
        ImVec2 size = ImGui::GetItemRectSize();
        cached_sizes[id] = size;

        // Stage 2: render centered
        float cursor_x = (window_size.x - size.x) * 0.5f;
        ImGui::SetCursorPosX(cursor_x);
    }

    ImGui::BeginGroup();
    f(); // re-draw content (needed due to immediate mode)
    ImGui::EndGroup();
}
