//
// Created by xabdomo on 6/25/25.
//

#ifndef IMAGE_TEXT_BUTTON_HPP
#define IMAGE_TEXT_BUTTON_HPP
#include "imgui.h"


namespace Widgets {
    bool ImageTextButton(
        ImTextureID texture,
        const char* label,
        ImVec2 imageSize,
        float spacing = 8.0f,
        ImVec4 bgColor = ImVec4(0, 0, 0, 0),
        ImVec2 padding = ImGui::GetStyle().FramePadding);
}



#endif //IMAGE_TEXT_BUTTON_HPP
