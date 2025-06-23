//
// Created by xabdomo on 6/23/25.
//

#include "image_viewer.hpp"

ImageViewer::ImageViewer(ImTextureID texId, const ImVec2 &imgSize) : textureId(texId), imageSize(imgSize) {}

void ImageViewer::Render(const char *label) {
    if (!textureId) return;

    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImVec2 drawSize = ComputeDrawSize(avail);
    ImVec2 drawPos = ComputeDrawPosition(avail, drawSize);

    ImGui::SetCursorPos(drawPos);
    ImGui::Image(textureId, drawSize);

    HandleContextMenu();
}

ImVec2 ImageViewer::ComputeDrawSize(const ImVec2 &avail) const  {
    float aspect = imageSize.x / imageSize.y;
    ImVec2 scaledSize = imageSize;

    switch (resizeMode) {
        case ResizeMode::FIT_WIDTH: {
            float width = std::min(std::max(imageSize.x, avail.x), avail.x);
            float height = width / aspect;
            scaledSize = ImVec2(width, height);
            break;
        }
        case ResizeMode::FIT_HEIGHT: {
            float height = std::min(std::max(imageSize.y, avail.y), avail.y);
            float width = height * aspect;
            scaledSize = ImVec2(width, height);
            break;
        }
        case ResizeMode::CENTER:
            scaledSize = imageSize;
            break;
        case ResizeMode::FILL:
            scaledSize = avail;
            break;
        case ResizeMode::NONE:
            scaledSize = imageSize;
    }

    // Apply zoom
    scaledSize.x *= zoom;
    scaledSize.y *= zoom;
    return scaledSize;
}

ImVec2 ImageViewer::ComputeDrawPosition(const ImVec2 &avail, const ImVec2 &drawSize) const  {
    ImVec2 cursorPos = ImGui::GetCursorPos();

    if (resizeMode == ResizeMode::CENTER) {
        ImVec2 offset = ImVec2(
            std::max(0.0f, (avail.x - drawSize.x) * 0.5f),
            std::max(0.0f, (avail.y - drawSize.y) * 0.5f)
        );
        return ImVec2(cursorPos.x + offset.x, cursorPos.y + offset.y);
    }

    return cursorPos;
}

void ImageViewer::HandleContextMenu() {
    if (ImGui::BeginPopupContextWindow("##ImageWidgetMenu", ImGuiPopupFlags_MouseButtonRight)) {
        ImGui::TextDisabled("Resize Mode");

        if (ImGui::MenuItem("None", nullptr, resizeMode == ResizeMode::NONE)) resizeMode = ResizeMode::NONE;
        if (ImGui::MenuItem("Fit Width", nullptr, resizeMode == ResizeMode::FIT_WIDTH)) resizeMode = ResizeMode::FIT_WIDTH;
        if (ImGui::MenuItem("Fit Height", nullptr, resizeMode == ResizeMode::FIT_HEIGHT)) resizeMode = ResizeMode::FIT_HEIGHT;
        if (ImGui::MenuItem("Center", nullptr, resizeMode == ResizeMode::CENTER)) resizeMode = ResizeMode::CENTER;
        if (ImGui::MenuItem("Fill", nullptr, resizeMode == ResizeMode::FILL)) resizeMode = ResizeMode::FILL;

        ImGui::Separator();
        ImGui::TextDisabled("Zoom");

        if (ImGui::MenuItem("0.5x", nullptr, zoom == 0.5f)) zoom = 0.5f;
        if (ImGui::MenuItem("1x", nullptr, zoom == 1.0f)) zoom = 1.0f;
        if (ImGui::MenuItem("1.5x", nullptr, zoom == 1.5f)) zoom = 1.5f;
        if (ImGui::MenuItem("2x", nullptr, zoom == 2.0f)) zoom = 2.0f;
        if (ImGui::MenuItem("4x", nullptr, zoom == 4.0f)) zoom = 4.0f;

        ImGui::EndPopup();
    }
}
