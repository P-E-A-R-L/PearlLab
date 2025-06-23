#include "preview.hpp"

#include <imgui.h>

#include "logger.hpp"
#include "pipeline.hpp"
#include "../font_manager.hpp"
#include "../../backend/py_safe_wrapper.hpp"
#include "../utility/gl_texture.hpp"
#include "../utility/image_store.hpp"

void Preview::VisualizedObject::init(PyVisualizable *obj)
{
    py::gil_scoped_acquire acquire{};

    this->visualizable = obj;

    if (visualizable->supports(VisualizationMethod::RGB_ARRAY))
    {
        _init_rgb_array();
    }

    if (visualizable->supports(VisualizationMethod::GRAY_SCALE))
    {
        _init_gray();
    }

    if (visualizable->supports(VisualizationMethod::HEAT_MAP))
    {
        _init_heat_map();
    }

    if (visualizable->supports(VisualizationMethod::FEATURES))
    {
        _init_features();
    }

    if (visualizable->supports(VisualizationMethod::BAR_CHART))
    {
        _init_bar_chart();
    }
}

bool Preview::VisualizedObject::supports(VisualizationMethod method) const
{
    switch (method)
    {
    case VisualizationMethod::RGB_ARRAY:
        return rgb_array != nullptr;
    case VisualizationMethod::GRAY_SCALE:
        return gray != nullptr;
    case VisualizationMethod::HEAT_MAP:
        return heat_map != nullptr;
    case VisualizationMethod::FEATURES:
        return features_params != nullptr;
    case VisualizationMethod::BAR_CHART:
        return bar_chart_params != nullptr;
    default:
        return false;
    }
}

void Preview::VisualizedObject::update() {
    if (rgb_array)
        _update_rgb_array();
    if (gray)
        _update_gray();
    if (heat_map)
        _update_heat_map();
    if (features_params)
        _update_features();
    if (bar_chart_params)
        _update_bar_chart();

    if (rgb_array == nullptr && visualizable->supports(VisualizationMethod::RGB_ARRAY))
        _init_rgb_array();
    if (gray == nullptr && visualizable->supports(VisualizationMethod::GRAY_SCALE))
        _init_gray();
    if (heat_map == nullptr && visualizable->supports(VisualizationMethod::HEAT_MAP))
        _init_heat_map();
    if (features_params == nullptr && visualizable->supports(VisualizationMethod::FEATURES))
        _init_features();
    if (bar_chart_params == nullptr && visualizable->supports(VisualizationMethod::BAR_CHART))
        _init_bar_chart();
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
        } }))
    {
        Logger::error("Failed to initialize RGB Array visualization (params) for object: " + std::string(visualizable->moduleName));
        delete rgb_array_params;
        rgb_array_params = nullptr;
        return;
    }

    if (!SafeWrapper::execute([&]
                              {
        rgb_array = new GLTexture();

        auto data = visualizable->getVisualization(VisualizationMethod::RGB_ARRAY, rgb_array_params->object);
        if (data.has_value()) {
            py::array_t<float> arr = data->cast<py::array>();
            if (arr.ndim() == 3 || arr.ndim() == 4) { // assuming RGB image / RGBA image
                int width = arr.shape(1);
                int height = arr.shape(0);
                int channels = arr.shape(2);
                //Logger::info(std::format("RGB Array Size: <{}, {}>", width, height));
                Logger::info("RGB Array Size: <" + std::to_string(width) + ", " + std::to_string(height) + ">");
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
        } }))
    {
        Logger::error("Failed to initialize RGB Array visualization (buffer) for object: " + std::string(visualizable->moduleName));
        delete rgb_array_params;
        rgb_array_params = nullptr;
        delete rgb_array;
        rgb_array = nullptr;
        return;
    }
}

void Preview::VisualizedObject::_update_rgb_array() const
{
    SafeWrapper::execute([&]
                         {
        auto data = visualizable->getVisualization(VisualizationMethod::RGB_ARRAY, rgb_array_params->object);
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
        } });
}

void Preview::VisualizedObject::_init_gray()
{
    if (!SafeWrapper::execute([&]
                              {
        auto type = visualizable->getVisualizationParamsType(VisualizationMethod::GRAY_SCALE);
        gray_params = new PyLiveObject();
        if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
            py::tuple args(0);
            gray_params->object = (*type)(*args);
            PyScope::parseLoadedModule(py::getattr(gray_params->object, "__class__"), *gray_params);
        } else {
            gray_params->object = py::none();
        } }))
    {
        Logger::error("Failed to initialize Gray Scale visualization (params) for object: " + std::string(visualizable->moduleName));
        delete gray_params;
        gray_params = nullptr;
        return;
    }

    if (!SafeWrapper::execute([&]
                              {
        gray = new GLTexture();

        auto data = visualizable->getVisualization(VisualizationMethod::GRAY_SCALE, gray_params->object);
        if (data.has_value()) {
            py::array_t<float> arr = data->cast<py::array>();
            if (arr.ndim() == 2) { // assuming Gray image
                int width = arr.shape(1);
                int height = arr.shape(0);
                int channels = 1;
                //Logger::info(std::format("Gray Size: <{}, {}>", width, height));
                Logger::info("Gray Size: <" + std::to_string(width) + ", " + std::to_string(height) + ">");

                std::vector<unsigned char> rgbData(width * height * 3);
                auto data_ptr = arr.data();
                for (int i = 0; i < width * height; i++) { // convert to RGB
                    rgbData[i * 3 + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                    rgbData[i * 3 + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                    rgbData[i * 3 + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                }
                gray->set(rgbData, width, height, 3);
            } else {
                Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
            }
        } else {
            Logger::warning("No Gray Scale visualization available for object: " + std::string(visualizable->moduleName));
        } }))
    {
        Logger::error("Failed to initialize Gray Scale visualization (buffer) for object: " + std::string(visualizable->moduleName));
        delete gray_params;
        gray_params = nullptr;
        delete gray;
        gray = nullptr;
        return;
    }
}

void Preview::VisualizedObject::_update_gray() const
{
    SafeWrapper::execute([&]
                         {
        auto data = visualizable->getVisualization(VisualizationMethod::GRAY_SCALE, gray_params->object);
        if (data.has_value()) {
            py::array_t<float> arr = data->cast<py::array>();
            if (arr.ndim() == 2) { // assuming Gray image
                int width = arr.shape(1);
                int height = arr.shape(0);
                int channels = 1;

                std::vector<unsigned char> rgbData(width * height * 3);
                auto data_ptr = arr.data();
                for (int i = 0; i < width * height; i++) { // convert to RGB
                    rgbData[i * 3 + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                    rgbData[i * 3 + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                    rgbData[i * 3 + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                }
                gray->update(rgbData);
            } else {
                Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
            }
        } else {
            Logger::warning("No Gray Scale visualization available for object: " + std::string(visualizable->moduleName));
        } });
}

void Preview::VisualizedObject::_init_heat_map()
{
    if (!SafeWrapper::execute([&]
                              {
        auto type = visualizable->getVisualizationParamsType(VisualizationMethod::HEAT_MAP);
        heat_map_params = new PyLiveObject();
        if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
            py::tuple args(0);
            heat_map_params->object = (*type)(*args);
            PyScope::parseLoadedModule(py::getattr(heat_map_params->object, "__class__"), *heat_map_params);
        } else {
            heat_map_params->object = py::none();
        } }))
    {
        Logger::error("Failed to initialize Heat map visualization (params) for object: " + std::string(visualizable->moduleName));
        delete heat_map_params;
        heat_map_params = nullptr;
        return;
    }

    if (!SafeWrapper::execute([&]
                              {
        heat_map = new GLTexture();

        auto data = visualizable->getVisualization(VisualizationMethod::HEAT_MAP, heat_map_params->object);
        if (data.has_value()) {
            py::array_t<float> arr = data->cast<py::array>();
            if (arr.ndim() == 2) { // assuming Gray image
                int width = arr.shape(1);
                int height = arr.shape(0);
                int channels = 1;
                //Logger::info(std::format("Heat map Size: <{}, {}>", width, height));
                Logger::info("Heat map Size: <" + std::to_string(width) + ", " + std::to_string(height) + ">");

                std::vector<unsigned char> rgbData(width * height * 3);
                auto data_ptr = arr.data();
                auto max = *std::max_element(data_ptr, data_ptr + width * height);
                auto min = *std::min_element(data_ptr, data_ptr + width * height);

                for (int i = 0; i < width * height; i++) { // convert to RGB
                    auto weight = (data_ptr[i * channels + 0] - min) / (max - min);
                    rgbData[i * 3 + 0]     = static_cast<unsigned char>(weight       * 255.0f);
                    rgbData[i * 3 + 1]     = static_cast<unsigned char>(0);
                    rgbData[i * 3 + 2]     = static_cast<unsigned char>((1 - weight) * 255.0f);
                }
                heat_map->set(rgbData, width, height, 3);
            } else {
                Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
            }
        } else {
            Logger::warning("No Heat map visualization available for object: " + std::string(visualizable->moduleName));
        } }))
    {
        Logger::error("Failed to initialize Heat map visualization (buffer) for object: " + std::string(visualizable->moduleName));
        delete heat_map_params;
        heat_map_params = nullptr;
        delete heat_map;
        heat_map = nullptr;
        return;
    }
}

void Preview::VisualizedObject::_update_heat_map() const
{
    SafeWrapper::execute([&]
                         {
        auto data = visualizable->getVisualization(VisualizationMethod::HEAT_MAP, heat_map_params->object);
        if (data.has_value()) {
            py::array_t<float> arr = data->cast<py::array>();
            if (arr.ndim() == 2) { // assuming Gray image
                int width = arr.shape(1);
                int height = arr.shape(0);
                int channels = 1;

                std::vector<unsigned char> rgbData(width * height * 3);
                auto data_ptr = arr.data();
                auto max = *std::max_element(data_ptr, data_ptr + width * height);
                auto min = *std::min_element(data_ptr, data_ptr + width * height);

                for (int i = 0; i < width * height; i++) { // convert to RGB
                    auto weight = (data_ptr[i * channels + 0] - min) / (max - min);
                    rgbData[i * 3 + 0]     = static_cast<unsigned char>(weight       * 255.0f);
                    rgbData[i * 3 + 1]     = static_cast<unsigned char>(0);
                    rgbData[i * 3 + 2]     = static_cast<unsigned char>((1 - weight) * 255.0f);
                }
                heat_map->update(rgbData);
            } else {
                Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
            }
        } else {
            Logger::warning("No Heat map visualization available for object: " + std::string(visualizable->moduleName));
        } });
}

void Preview::VisualizedObject::_init_features()
{
    if (!SafeWrapper::execute([&]
                              {
        auto type = visualizable->getVisualizationParamsType(VisualizationMethod::FEATURES);
        features_params = new PyLiveObject();
        if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
            py::tuple args(0);
            features_params->object = (*type)(*args);
            PyScope::parseLoadedModule(py::getattr(features_params->object, "__class__"), *features_params);
        } else {
            features_params->object = py::none();
        } }))
    {
        Logger::error("Failed to initialize Features visualization (params) for object: " + std::string(visualizable->moduleName));
        delete features_params;
        features_params = nullptr;
        return;
    }

    _update_features();
}

void Preview::VisualizedObject::_update_features()
{
    SafeWrapper::execute([&]
                         {
        auto data = visualizable->getVisualization(VisualizationMethod::FEATURES, features_params->object);
        if (data.has_value()) {
            features.clear();

            py::dict dict = data->cast<py::dict>();
            for (const auto item : dict) {
                py::str key = py::str(item.first);
                py::str value = py::str(item.second);
                features[key] = value;
            }

        } else {
            Logger::warning("No Features visualization available for object: " + std::string(visualizable->moduleName));
        } });
}

void Preview::VisualizedObject::_init_bar_chart()
{
    if (!SafeWrapper::execute([&]
                              {
        auto type = visualizable->getVisualizationParamsType(VisualizationMethod::BAR_CHART);
        bar_chart_params = new PyLiveObject();
        if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
            py::tuple args(0);
            bar_chart_params->object = (*type)(*args);
            PyScope::parseLoadedModule(py::getattr(bar_chart_params->object, "__class__"), *bar_chart_params);
        } else {
            bar_chart_params->object = py::none();
        } }))
    {
        Logger::error("Failed to initialize Bar Chart visualization (params) for object: " + std::string(visualizable->moduleName));
        delete bar_chart_params;
        bar_chart_params = nullptr;
        return;
    }

    _update_bar_chart();
}

void Preview::VisualizedObject::_update_bar_chart()
{
    SafeWrapper::execute([&]
                         {
        auto data = visualizable->getVisualization(VisualizationMethod::BAR_CHART, bar_chart_params->object);
        if (data.has_value()) {
            bar_chart.clear();
            py::dict dict = data->cast<py::dict>();

            for (const auto item : dict) {
                py::str key = py::str(item.first);
                try {
                    float value = py::cast<float>(item.second);
                    bar_chart[key] = value;
                } catch (...) {
                    Logger::error("Failed to process bar chart entry: " + std::string(key));
                }
            }
        } else {
            Logger::warning("No Bar Chart visualization available");
        } });
}

void Preview::VisualizedAgent::init(Pipeline::ActiveAgent *agent)
{

    this->agent = agent;
    if (agent->env)
    {
        env_visualization = new VisualizedObject();
        env_visualization->init(agent->env);
    }

    for (auto &method : agent->methods)
    {
        if (method)
        {
            auto vis_obj = new VisualizedObject();
            vis_obj->init(method);
            method_visualizations.push_back(vis_obj);
        }
    }
}

void Preview::VisualizedAgent::update() const
{
    py::gil_scoped_acquire acquire{};

    if (env_visualization){
        env_visualization->update();
    }

    for (auto &method_vis : method_visualizations){
        method_vis->update();
    }
}

Preview::VisualizedAgent::~VisualizedAgent()
{
    delete env_visualization;
    for (auto &method_vis : method_visualizations)
    {
        delete method_vis;
    }

    method_visualizations.clear();
    env_visualization = nullptr;
}

void Preview::init() {}

static void _render_visualizable(Preview::VisualizedObject *obj, const std::string &message)
{
    if (ImGui::BeginTabBar("##vis"))
    {
        if (obj->supports(VisualizationMethod::RGB_ARRAY) && ImGui::BeginTabItem("RGB"))
        {
            auto obs = obj->rgb_array;
            ImGui::Image(obs->id(), ImVec2(obs->width(), obs->height()));
            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::GRAY_SCALE) && ImGui::BeginTabItem("Gray"))
        {
            auto obs = obj->gray;
            ImGui::Image(obs->id(), ImVec2(obs->width(), obs->height()));
            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::HEAT_MAP) && ImGui::BeginTabItem("Heat Map"))
        {
            auto obs = obj->heat_map;
            ImGui::Image(obs->id(), ImVec2(obs->width(), obs->height()));
            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::FEATURES) && ImGui::BeginTabItem("Features"))
        {
            auto obs = obj->features;

            if (ImGui::BeginTable("StringFloatTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                for (const auto &[name, value] : obs)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(value.c_str());
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::BAR_CHART) && ImGui::BeginTabItem("Bar Chart"))
        {
            // min/max values for scaling
            float min_value = 0.0f;
            float max_value = 0.0f;
            for (const auto &[key, value] : obj->bar_chart)
            {
                min_value = std::min(min_value, value);
                max_value = std::max(max_value, value);
            }

            float range_padding = std::max(std::abs(max_value), std::abs(min_value)) * 0.2f;
            min_value -= range_padding;
            max_value += range_padding;
            if (min_value == max_value)
            {
                min_value -= 1.0f;
                max_value += 1.0f;
            }

            // chart parameters
            const float bar_width = 50.0f;
            const float bar_spacing = 30.0f;
            const float graph_height = 300.0f;
            const float zero_line_y = graph_height * (max_value / (max_value - min_value));
            const float value_range = max_value - min_value;

            const float graph_width = (bar_width + bar_spacing) * obj->bar_chart.size() + bar_spacing;

            if (ImGui::BeginChild("BarChartView", ImVec2(0, graph_height + 50), false,
                                  ImGuiWindowFlags_HorizontalScrollbar))
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                const ImU32 positive_color = ImGui::GetColorU32(ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                const ImU32 negative_color = ImGui::GetColorU32(ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                const ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
                const ImU32 axis_color = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

                // zero line
                draw_list->AddLine(
                    ImVec2(cursor_pos.x, cursor_pos.y + zero_line_y),
                    ImVec2(cursor_pos.x + graph_width, cursor_pos.y + zero_line_y),
                    axis_color, 2.0f);

                float x_offset = bar_spacing;

                for (const auto &[key, value] : obj->bar_chart)
                {
                    const bool is_positive = value >= 0;
                    const float bar_height = (std::abs(value) / value_range) * graph_height;

                    // bar positions
                    ImVec2 bar_min, bar_max;
                    if (is_positive)
                    {
                        bar_min = ImVec2(cursor_pos.x + x_offset, cursor_pos.y + zero_line_y - bar_height);
                        bar_max = ImVec2(cursor_pos.x + x_offset + bar_width, cursor_pos.y + zero_line_y);
                    }
                    else
                    {
                        bar_min = ImVec2(cursor_pos.x + x_offset, cursor_pos.y + zero_line_y);
                        bar_max = ImVec2(cursor_pos.x + x_offset + bar_width, cursor_pos.y + zero_line_y + bar_height);
                    }

                    draw_list->AddRectFilled(bar_min, bar_max,
                                             is_positive ? positive_color : negative_color, 3.0f);

                    // value text
                    char value_text[32];
                    snprintf(value_text, sizeof(value_text), "%.2f", value);
                    const ImVec2 text_size = ImGui::CalcTextSize(value_text);

                    ImVec2 text_pos;
                    if (is_positive)
                    {
                        text_pos = ImVec2(
                            bar_min.x + (bar_width - text_size.x) * 0.5f,
                            bar_min.y - text_size.y - 2.0f);
                    }
                    else
                    {
                        text_pos = ImVec2(
                            bar_min.x + (bar_width - text_size.x) * 0.5f,
                            bar_max.y + 2.0f);
                    }
                    draw_list->AddText(text_pos, text_color, value_text);

                    // feature names
                    const ImVec2 name_pos = ImVec2(
                        bar_min.x + (bar_width - ImGui::CalcTextSize(key.c_str()).x) * 0.5f,
                        cursor_pos.y + graph_height + 5.0f);
                    draw_list->AddText(name_pos, text_color, key.c_str());

                    x_offset += bar_width + bar_spacing;
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

static void _render_agent_basic(const Pipeline::ActiveAgent* agent, float width, int index)
{
    ImGui::BeginChild(agent->name, ImVec2(width, 0), true);
    FontManager::pushFont("Bold");
    ImGui::Text("%s", agent->name);
    FontManager::popFont();

    if (agent->env)
    {
        _render_visualizable(Preview::previews[index]->env_visualization, "No observations available.");
    }
    else
    {
        ImGui::Text("No environment available.");
    }

    if (ImGui::BeginTable("AgentScoresTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Method");
        ImGui::TableSetupColumn("Episode Score");
        ImGui::TableSetupColumn("Total Score");
        ImGui::TableSetupColumn("Normalized");
        ImGui::TableHeadersRow();

        for (int i = 0; i < agent->methods.size(); ++i)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Pipeline::PipelineConfig::pipelineMethods[i].name);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", agent->scores_ep[i]);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", agent->scores_total[i]);

            ImGui::TableSetColumnIndex(3);
            float normalized = (agent->total_steps > 0) ? agent->scores_total[i] / agent->total_steps : 0.0f;
            ImGui::Text("%.4f", normalized);
        }

        ImGui::EndTable();
    }

    if (agent->env_terminated || agent->env_truncated)
    {
        ImGui::Text("<Episode completed>");
    }

    ImGui::EndChild();
}

typedef std::function<void(Pipeline::ActiveAgent*, float, int)> _agent_render_function;

static int agents_per_row = 3;
static void tab_wrapper(const _agent_render_function &_f)
{
    ImGui::BeginChild("AgentsScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    { // agents previews (env)
        auto available_width = ImGui::GetContentRegionAvail().x;
        auto child_width = (available_width - (agents_per_row + 1) * 10) / agents_per_row;
        child_width = std::max(child_width, 320.0f);

        int num_rows = (Pipeline::PipelineState::activeAgents.size() + agents_per_row - 1) / agents_per_row;
        float total_height = 0;
        
        for (int row = 0; row < num_rows; row++) {
            float max_height_in_row = 0;
            
            for (int col = 0; col < agents_per_row; col++) {
                int agent_index = row * agents_per_row + col;
                if (agent_index >= Pipeline::PipelineState::activeAgents.size())
                    break;
                
                auto &agent = Pipeline::PipelineState::activeAgents[agent_index];
                ImGui::PushID(agent_index);
                
                ImGui::SetCursorPos(ImVec2(col * (child_width + 10) + 10, total_height + 10));
                ImGui::BeginGroup();
                _f(agent, child_width, agent_index);
                ImGui::EndGroup();
                
                ImGui::PopID();
                auto size = ImGui::GetItemRectSize();
                max_height_in_row = std::max(max_height_in_row, size.y);
            }
            
            total_height += max_height_in_row + 10; // spacing after each row
        }
        
        // dummy item to ensure proper scrolling
        ImGui::SetCursorPos(ImVec2(0, total_height));
        ImGui::Dummy(ImVec2(1, 1));
    }
    
    ImGui::EndChild();
}

static void _render_preview()
{
    for (int i = 0; i < Pipeline::PipelineState::activeAgents.size(); i++) {
        Preview::previews[i]->update(); // update the visualizations (textures, features, etc ..)
    }

    { // control zone
        auto playing = Pipeline::isSimRunning();
        auto size = ImGui::GetContentRegionAvail();
        auto control_size = 16 * 2 + ImGui::GetStyle().ItemSpacing.x;
        auto x = (size.x - control_size) / 2;

        std::string Icon = "./assets/icons/play-grad.png";
        if (playing)
            Icon = "./assets/icons/pause-grad.png";

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::SetCursorPosX(x);
        if (ImGui::ImageButton("##play_btn", ImageStore::idOf(Icon), {20, 20}))
        {
            if (!playing)
                Pipeline::continueSim();
            else
                Pipeline::pauseSim();
        }

        ImGui::SameLine();
        if (ImGui::ImageButton("##step_btn", ImageStore::idOf("./assets/icons/step-grad.png"), {20, 20}))
        {
            Pipeline::stepSim();
        }
        ImGui::PopStyleVar();
    }

    if (ImGui::BeginTabBar("Tabs"))
    {
        if (ImGui::BeginTabItem("Basic"))
        {
            tab_wrapper(_render_agent_basic);
            ImGui::EndTabItem();
        }

        for (int i = 0; i < Pipeline::PipelineConfig::pipelineMethods.size(); i++)
        {
            ImGui::PushID(i);
            if (Pipeline::PipelineConfig::pipelineMethods[i].active && ImGui::BeginTabItem(Pipeline::PipelineConfig::pipelineMethods[i].name))
            {
                tab_wrapper([&](Pipeline::ActiveAgent *agent, float width, int index)
                            {
                    ImGui::BeginChild(agent->name, ImVec2(width, 0), true);
                    FontManager::pushFont("Bold");
                    ImGui::Text("%s"   , agent->name);
                    FontManager::popFont();

                    if (agent->methods[i]) {
                        _render_visualizable(Preview::previews[index]->method_visualizations[i], "No observations available.");
                    } else {
                        ImGui::Text("No method visualization available.");
                    }


                    if (agent->env_terminated || agent->env_truncated) {
                        ImGui::Text("<Episode completed>");
                    }

                    ImGui::EndChild(); });
                ImGui::EndTabItem();
            }
            ImGui::PopID();
        }

        ImGui::EndTabBar();
    }
}

void Preview::render()
{
    ImGui::Begin("Preview");

    if (!Pipeline::isExperimenting())
    {

        auto size = ImGui::GetContentRegionAvail();
        auto text_size = ImGui::CalcTextSize("No active experiment.\nStart an experiment in the pipeline tab.");

        auto x = (size.x - text_size.x) / 2;
        auto y = (size.y - text_size.y) / 2;

        ImGui::SetCursorPos({x, y});
        ImGui::Text("No active experiment.\nStart an experiment in the pipeline tab.");
    }
    else
    {

        _render_preview();
    }
    ImGui::End();
}

std::vector<Preview::VisualizedAgent *> Preview::previews;

void Preview::onStart()
{
    for (auto &agent : Pipeline::PipelineState::activeAgents)
    {
        previews.push_back(new Preview::VisualizedAgent());
        previews.back()->init(agent);
    }
}

void Preview::onStop()
{
    for (auto prev : previews)
    {
        delete prev;
    }

    previews.clear();
}

void Preview::destroy()
{
    for (auto prev : previews)
    {
        delete prev;
    }

    previews.clear();
}
