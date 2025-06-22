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
    default:
        return false;
    }
}

void Preview::VisualizedObject::update()
{
    if (rgb_array)
        _update_rgb_array();
    if (gray)
        _update_gray();
    if (heat_map)
        _update_heat_map();
    if (features_params)
        _update_features();

    if (rgb_array == nullptr && visualizable->supports(VisualizationMethod::RGB_ARRAY))
        _init_rgb_array();
    if (gray == nullptr && visualizable->supports(VisualizationMethod::GRAY_SCALE))
        _init_gray();
    if (heat_map == nullptr && visualizable->supports(VisualizationMethod::HEAT_MAP))
        _init_heat_map();
    if (features_params == nullptr && visualizable->supports(VisualizationMethod::FEATURES))
        _init_features();
}

Preview::VisualizedObject::~VisualizedObject()
{
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

void Preview::VisualizedObject::_init_rgb_array()
{
    if (!SafeWrapper::execute([&]
                              {
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
    if (env_visualization)
    {
        env_visualization->update();
    }

    for (auto &method_vis : method_visualizations)
    {
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

        ImGui::EndTabBar();
    }
}

static void _render_agent_basic(const Pipeline::ActiveAgent &agent, float width, int index)
{
    ImGui::BeginChild(agent.name, ImVec2(width, 0), true);
    FontManager::pushFont("Bold");
    ImGui::Text("%s", agent.name);
    FontManager::popFont();

    if (agent.env)
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

        for (int i = 0; i < agent.methods.size(); ++i)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Pipeline::PipelineConfig::pipelineMethods[i].name);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", agent.scores_ep[i]);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", agent.scores_total[i]);

            ImGui::TableSetColumnIndex(3);
            float normalized = (agent.total_steps > 0) ? agent.scores_total[i] / agent.total_steps : 0.0f;
            ImGui::Text("%.4f", normalized);
        }

        ImGui::EndTable();
    }

    if (agent.env_terminated || agent.env_truncated)
    {
        ImGui::Text("<Episode completed>");
    }

    ImGui::EndChild();
}

typedef std::function<void(Pipeline::ActiveAgent &, float, int)> _agent_render_function;

static int agents_per_row = 3;
static void tab_wrapper(const _agent_render_function &_f)
{
    { // agents previews (env)
        auto child_width = (ImGui::GetContentRegionAvail().x - (agents_per_row + 1) * 10) / agents_per_row;
        child_width = std::max(child_width, 320.0f);

        float y = ImGui::GetCursorPosY() + 10;
        float max_height = 0;
        for (int i = 0; i < Pipeline::PipelineState::activeAgents.size(); i++)
        {
            auto &agent = Pipeline::PipelineState::activeAgents[i];
            ImGui::PushID(i);
            ImGui::SetCursorPos({(i % agents_per_row) * (child_width + 10) + 10, y});
            ImGui::BeginGroup();
            _f(agent, child_width, i);
            ImGui::EndGroup();
            ImGui::PopID();
            auto size = ImGui::GetItemRectSize();
            max_height = std::max(max_height, size.y);
            if (i % agents_per_row == 0 && i != 0)
            {
                y += max_height + 10; // add some spacing
                max_height = 0;       // reset for next row
            }
        }
    }
}

static void _render_preview()
{

    for (int i = 0; i < Pipeline::PipelineState::activeAgents.size(); i++)
    {
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
                tab_wrapper([&](Pipeline::ActiveAgent &agent, float width, int index)
                            {
                    ImGui::BeginChild(agent.name, ImVec2(width, 0), true);
                    FontManager::pushFont("Bold");
                    ImGui::Text("%s"   , agent.name);
                    FontManager::popFont();

                    if (agent.methods[i]) {
                        _render_visualizable(Preview::previews[index]->method_visualizations[i], "No observations available.");
                    } else {
                        ImGui::Text("No method visualization available.");
                    }


                    if (agent.env_terminated || agent.env_truncated) {
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
        previews.back()->init(&agent);
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
