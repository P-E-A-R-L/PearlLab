//
// Created by xabdomo on 6/11/25.
//

#include "font_manager.hpp"
#include <unordered_map>
#include <string>

#include "imgui.h"

static std::unordered_map<std::string, ImFont*> fontMap;

void FontManager::init() {
    ImGuiIO &io = ImGui::GetIO();

    // Load fonts and store them with names
    fontMap["Regular"] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 18.0f);
    fontMap["Bold"] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Bold.ttf", 18.0f);
    fontMap["SemiBold"] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-SemiBold.ttf", 14.0f);
    fontMap["Medium"] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Medium.ttf", 14.0f);
    fontMap["Light"] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Light.ttf", 14.0f);

    // Set default font
    io.FontDefault = fontMap["Regular"];

    // Build the font atlas
    io.Fonts->Build();
}

void FontManager::pushFont(const std::string &name) {
    if (fontMap.contains(name))
        ImGui::PushFont(fontMap[name]);
}

void FontManager::popFont() {
    ImGui::PopFont();
}