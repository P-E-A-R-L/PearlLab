//
// Created by xabdomo on 6/25/25.
//

#include "image_text_button.hpp"

#include <algorithm>

bool Widgets::ImageTextButton(
    ImTextureID texture, const char *label, ImVec2 imageSize, float spacing, ImVec4 bgColor,
ImVec2 padding) {
    ImGui::PushID(label);

    // Calculate sizes
    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 contentSize = ImVec2(imageSize.x + spacing + textSize.x, std::max(imageSize.y, textSize.y));
    ImVec2 buttonSize = ImVec2(contentSize.x + padding.x * 2, contentSize.y + padding.y * 2);

    // Reserve space and invisible button
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##image_text_btn", buttonSize);
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background highlight (optional, here only on hover)
    if (hovered) {
        drawList->AddRectFilled(pos, ImVec2(pos.x + buttonSize.x, pos.y + buttonSize.y),
                                IM_COL32(255, 255, 255, 80), 4.0f);
    }

    // Draw image
    ImVec2 imagePos = ImVec2(pos.x + padding.x, pos.y + padding.y + (contentSize.y - imageSize.y) * 0.5f);
    drawList->AddImage(texture, imagePos, ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));

    // Draw text to the right of the image
    ImVec2 textPos = ImVec2(imagePos.x + imageSize.x + spacing,
                            pos.y + padding.y + (contentSize.y - textSize.y) * 0.5f);
    drawList->AddText(textPos, IM_COL32_WHITE, label);

    ImGui::PopID();
    return clicked;
}
