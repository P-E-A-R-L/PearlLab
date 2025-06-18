//
// Created by xabdomo on 6/18/25.
//

#include "preview.hpp"

#include <imgui.h>

#include "pipeline.hpp"
#include "../utility/gl_texture.hpp"

void Preview::init() {

}

static int active_preview            = 0; // 0 -> Env, 1 -> Methods
static int active_preview_method     = 0; // the index of the active method in the preview
static int agents_per_row = 3;

static std::vector<GLTexture*> preview_textures;

static void _render_agent(const Pipeline::ActiveAgent& agent, float width, int index) {
    ImGui::BeginChild(agent.name, ImVec2(width, 0), true);
    ImGui::Text("Agent: %s", agent.name);
    ImGui::Text("Reward: %.2f", agent.reward);
    if (active_preview == 0) { // draw env
        if (agent.env) {
            auto obs = agent.env->render("rbg_array");
            if (obs != std::nullopt) {
                py::array_t<float> arr = obs->cast<py::array>();
                if (arr.ndim() == 3) { // assuming RGB image
                    int width = arr.shape(1);
                    int height = arr.shape(0);
                    int channels = arr.shape(2);
                    if (channels == 3 || channels == 4) {
                        GLTexture* texture = preview_textures[index];
                        std::vector<unsigned char> rgbData(width * height * channels);
                        auto data_ptr = arr.data();
                        for (int i = 0;i < width * height * channels; i++) {
                            rgbData[i] = static_cast<unsigned char>(data_ptr[i] * 255.0f);
                        }
                        texture->set(rgbData, width, height, channels);
                        ImGui::Image(texture->id(), ImVec2(width, height));
                    } else {
                        ImGui::Text("Unsupported number of channels: %d", channels);
                    }
                } else {
                    ImGui::Text("Unsupported observation shape: %dD", arr.ndim());
                }
            } else {
                ImGui::Text("No observations available.");
            }
        } else {
            ImGui::Text("No environment available.");
        }
    } else { // draw methods
        if (index < agent.methods.size()) {
            auto method = agent.methods[index];
            if (method) {
                // auto vis = method->getVisualization(Pipeline::VisualizationMethod::IMAGE, py::dict());
                // if (vis.has_value()) {
                //     GLTexture* texture = new GLTexture();
                //     texture->set(vis.value().cast<py::array>(), 320, 240, 3); // assuming RGB image
                //     preview_textures.push_back(texture);
                //     texture->bind();
                //     ImGui::Image((void*)(intptr_t)texture->id(), ImVec2(320, 240));
                //     texture->unbind();
                // } else {
                //     ImGui::Text("No visualization available for this method.");
                // }
            } else {
                ImGui::Text("No method available.");
            }
        } else {
            ImGui::Text("Invalid method index.");
        }
    }
    ImGui::EndChild();
}

static void _render_preview() {
    auto child_width = ( ImGui::GetContentRegionAvail().x - (agents_per_row + 1) * 10 ) / agents_per_row;
    child_width = std::max(child_width, 320.0f);

    float y = 60;
    float max_height = 0;
    for (int i = 0;i < Pipeline::PipelineState::activeAgents.size();i++) {
        auto& agent = Pipeline::PipelineState::activeAgents[i];
        ImGui::PushID(i);
        ImGui::SetCursorPos({(i % agents_per_row) * ( child_width + 10 ) + 10, y});
        ImGui::BeginGroup();
        _render_agent(agent, child_width, i);
        ImGui::EndGroup();
        ImGui::PopID();
        auto size = ImGui::GetItemRectSize();
        max_height = std::max(max_height, size.y);
        if (i % agents_per_row == 0 && i != 0) {
            y += max_height + 10; // add some spacing
            max_height = 0; // reset for next row
        }
    }
}

void Preview::render() {
    ImGui::Begin("Preview");
    if (!Pipeline::isExperimenting()) {

        auto size = ImGui::GetContentRegionAvail();
        auto text_size  = ImGui::CalcTextSize("No active experiment.\nStart an experiment in the pipeline tab.");

        auto x = (size.x - text_size.x) / 2;
        auto y = (size.y - text_size.y) / 2;

        ImGui::SetCursorPos({ x, y });
        ImGui::Text("No active experiment.\nStart an experiment in the pipeline tab.");

    } else {

        _render_preview();
    }
    ImGui::End();
}

void Preview::onStart() {
    for (auto& agent: Pipeline::PipelineState::activeAgents) {
        // Create a texture for each agent
        GLTexture* texture = new GLTexture();
        preview_textures.push_back(texture);
    }
}

void Preview::onStop() {
    for (auto& texture : preview_textures) {
        delete texture;
    }
    preview_textures.clear();
}

void Preview::destroy() {
    for (auto& texture : preview_textures) {
        delete texture;
    }
}
