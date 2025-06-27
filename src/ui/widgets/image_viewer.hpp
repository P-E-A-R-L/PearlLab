//
// Created by xabdomo on 6/23/25.
//

#ifndef IMAGE_VIEWER_HPP
#define IMAGE_VIEWER_HPP

#include "imgui.h"
#include <algorithm>



class ImageViewer {
public:
    enum class ResizeMode {
        NONE,
        FIT_WIDTH,
        FIT_HEIGHT,
        CENTER,
        FILL
    };

    ImTextureID textureId;
    ImVec2 imageSize;
    ResizeMode resizeMode = ResizeMode::FIT_HEIGHT;
    float zoom = 1.0f;

    ImageViewer(ImTextureID texId, const ImVec2& imgSize);

    void Render(const char* label);

private:
    ImVec2 ComputeDrawSize(const ImVec2& avail) const;

    ImVec2 ComputeDrawPosition(const ImVec2& avail, const ImVec2& drawSize) const;

    void HandleContextMenu();
};




#endif //IMAGE_VIEWER_HPP
