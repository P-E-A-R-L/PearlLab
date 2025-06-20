//
// Created by xabdomo on 6/18/25.
//

#include "preview.hpp"

#include <imgui.h>

#include "logger.hpp"
#include "pipeline.hpp"
#include "../../backend/py_safe_wrapper.hpp"
#include "../utility/gl_texture.hpp"
#include "../utility/image_store.hpp"

void Preview::VisualizedObject::init(PyVisualizable * obj) {
    this->visualizable = obj;

    if (visualizable->supports(VisualizationMethod::RGB_ARRAY)) {
        _init_rgb_array();
    }
}

bool Preview::VisualizedObject::supports(VisualizationMethod method) const {
    switch (method) {
        case VisualizationMethod::RGB_ARRAY:
            return rgb_array != nullptr;
        case VisualizationMethod::GRAY_SCALE:
            return gray != nullptr;
        case VisualizationMethod::HEAT_MAP:
            return heat_map != nullptr;
        case VisualizationMethod::FEATURES:
            return features_params != nullptr;
        default:
            return false;
    }
}

void Preview::VisualizedObject::update() const {
    if (rgb_array) _update_rgb_array();
}

Preview::VisualizedObject::~VisualizedObject() {
    visualizable = nullptr;

    delete rgb_array;
    delete rgb_array_params;

    delete gray;
    delete gray_params;

    delete heat_map;
    delete heat_map_params;

    delete features_params;

    rgb_array = nullptr;
    rgb_array_params = nullptr;

    gray = nullptr;
    gray_params = nullptr;

    heat_map = nullptr;
    heat_map_params = nullptr;

    features_params = nullptr;
}

void Preview::VisualizedObject::_init_rgb_array() {
    if (!SafeWrapper::execute([&]{
        auto type = visualizable->getVisualizationParamsType(VisualizationMethod::RGB_ARRAY);
        rgb_array_params = new PyLiveObject();
        if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
            py::tuple args(0);
            rgb_array_params->object = (*type)(*args);
            PyScope::parseLoadedModule(py::getattr(rgb_array_params->object, "__class__"), *rgb_array_params);
        } else {
            rgb_array_params->object = py::none();
        }
    })) {
        Logger::error("Failed to initialize RGB Array visualization (params) for object: " + std::string(visualizable->moduleName));
        delete rgb_array_params;
        rgb_array_params = nullptr;
        return;
    }

    if (!SafeWrapper::execute([&] {
        rgb_array = new GLTexture();

        auto data = visualizable->getVisualization(VisualizationMethod::RGB_ARRAY);
        if (data.has_value()) {
            py::array_t<float> arr = data->cast<py::array>();
            if (arr.ndim() == 3 || arr.ndim() == 4) { // assuming RGB image / RGBA image
                int width = arr.shape(1);
                int height = arr.shape(0);
                int channels = arr.shape(2);
                if (channels == 3 || channels == 4) {
                    std::vector<unsigned char> rgbData(width * height * channels);
                    auto data_ptr = arr.data();
                    for (int i = 0; i < width * height; i++) {
                        rgbData[i * channels + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                        rgbData[i * channels + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 1] * 255.0f);
                        rgbData[i * channels + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 2] * 255.0f);

                        if (channels == 4)
                            rgbData[i * channels + 3] = static_cast<unsigned char>(data_ptr[i * channels + 3] * 255.0f);
                    }
                    rgb_array->set(rgbData, width, height, channels);
                } else {
                    Logger::error("Unsupported number of channels: " + std::to_string(channels));
                }
            } else {
                Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
            }
        } else {
            Logger::warning("No RGB Array visualization available for object: " + std::string(visualizable->moduleName));
        }
    })) {
        Logger::error("Failed to initialize RGB Array visualization (buffer) for object: " + std::string(visualizable->moduleName));
        delete rgb_array_params;
        rgb_array_params = nullptr;
        delete rgb_array;
        rgb_array = nullptr;
        return;
    }
}

void Preview::VisualizedObject::_update_rgb_array() const {
    auto data = visualizable->getVisualization(VisualizationMethod::RGB_ARRAY);
    if (data.has_value()) {
        py::array_t<float> arr = data->cast<py::array>();
        if (arr.ndim() == 3 || arr.ndim() == 4) { // assuming RGB image / RGBA image
            int width = arr.shape(1);
            int height = arr.shape(0);
            int channels = arr.shape(2);
            if (channels == 3 || channels == 4) {
                std::vector<unsigned char> rgbData(width * height * channels);
                auto data_ptr = arr.data();
                for (int i = 0; i < width * height; i++) {
                    rgbData[i * channels + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                    rgbData[i * channels + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 1] * 255.0f);
                    rgbData[i * channels + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 2] * 255.0f);

                    if (channels == 4)
                        rgbData[i * channels + 3] = static_cast<unsigned char>(data_ptr[i * channels + 3] * 255.0f);
                }
                rgb_array->update(rgbData);
            } else {
                Logger::error("Unsupported number of channels: " + std::to_string(channels));
            }
        } else {
            Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
        }
    } else {
        Logger::warning("No RGB Array visualization available for object: " + std::string(visualizable->moduleName));
    }
}

void Preview::VisualizedAgent::init(Pipeline::ActiveAgent *agent) {

    this->agent = agent;
    if (agent->env) {
        env_visualization = new VisualizedObject();
        env_visualization->init(agent->env);
    }

    for (auto& method : agent->methods) {
        if (method) {
            auto vis_obj = new VisualizedObject();
            vis_obj->init(method);
            method_visualizations.push_back(vis_obj);
        }
    }
}

void Preview::VisualizedAgent::update() {
    if (env_visualization) {
        env_visualization->update();
    }

    for (auto& method_vis : method_visualizations) {
        method_vis->update();
    }
}

Preview::VisualizedAgent::~VisualizedAgent() {
    delete env_visualization;
    for (auto& method_vis : method_visualizations) {
        delete method_vis;
    }

    method_visualizations.clear();
    env_visualization = nullptr;
}

void Preview::init() {

}

static int active_preview            = 0; // 0 -> Env, 1 -> Methods
static int active_preview_method     = 0; // the index of the active method in the preview
static int agents_per_row = 3;

static void _render_agent(const Pipeline::ActiveAgent& agent, float width, int index) {
    ImGui::BeginChild(agent.name, ImVec2(width, 0), true);
    ImGui::Text("Agent: %s"   , agent.name);
    ImGui::Text("Reward: %.2f", agent.reward);

    Preview::previews[index]->update(); // update the visualizations (textures, features, etc ..)

    if (active_preview == 0) { // draw env
        if (agent.env) {
            auto obs = Preview::previews[index]->env_visualization->rgb_array;
            if (obs) {
                ImGui::Image(obs->id(), ImVec2(obs->width(), obs->height()));
            } else {
                ImGui::Text("No observations available.");
            }
        } else {
            ImGui::Text("No environment available.");
        }
    } else { // draw methods

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
    ImGui::Image(ImageStore::idOf("./assets/icons/play.png"), {64, 64});
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

std::vector<Preview::VisualizedAgent*> Preview::previews;

void Preview::onStart() {
    for (auto& agent: Pipeline::PipelineState::activeAgents) {
        previews.push_back(new Preview::VisualizedAgent());
        previews.back()->init(&agent);
    }
}

void Preview::onStop() {
    for (auto prev : previews) {
        delete prev;
    }

    previews.clear();
}

void Preview::destroy() {
    for (auto prev : previews) {
        delete prev;
    }

    previews.clear();
}
