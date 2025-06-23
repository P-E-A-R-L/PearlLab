//
// Created by xabdomo on 6/23/25.
//

#include <iostream>
#include <mutex>

#include "logger.hpp"
#include "pipeline.hpp"
#include "../utility/gl_texture.hpp"
#include "../../backend/py_safe_wrapper.hpp"

namespace Pipeline {
    void VisualizedObject::init(PyVisualizable * obj) {
        py::gil_scoped_acquire acquire{};
        this->visualizable = obj;
        this->m_lock       = new std::mutex();

        if (visualizable->supports(VisualizationMethod::RGB_ARRAY)) {
            _init_rgb_array();
        }

        if (visualizable->supports(VisualizationMethod::GRAY_SCALE)) {
            _init_gray();
        }

        if (visualizable->supports(VisualizationMethod::HEAT_MAP)) {
            _init_heat_map();
        }

        if (visualizable->supports(VisualizationMethod::FEATURES)) {
            _init_features();
        }

        if (visualizable->supports(VisualizationMethod::BAR_CHART)) {
            _init_bar_chart();
        }
    }

    bool VisualizedObject::supports(VisualizationMethod method) const {
        switch (method){
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


    void VisualizedObject::update() {
        if (rgb_array)        _update_rgb_array();
        if (gray)             _update_gray();
        if (heat_map)         _update_heat_map();
        if (features_params)  _update_features();
        if (bar_chart_params) _update_bar_chart();

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

    void VisualizedObject::update_tex() {
        if (rgb_array != nullptr) {
            std::lock_guard guard(*m_lock);
            auto& [rgbData, width, height, channels] = rgb_array_buffer;
            if (width != rgb_array->width() || height != rgb_array->height()) { // size changed
                rgb_array->set(rgbData, width, height, channels);
            } else {
                rgb_array->update(rgbData);
            }
        }

        if (gray != nullptr) {
            std::lock_guard guard(*m_lock);
            auto& [rgbData, width, height, channels] = gray_buffer;
            if (width != gray->width() || height != gray->height()) { // size changed
                gray->set(rgbData, width, height, channels);
            } else {
                gray->update(rgbData);
            }
        }

        if (heat_map != nullptr) {
            std::lock_guard guard(*m_lock);
            auto& [rgbData, width, height, channels] = heat_map_buffer;
            if (width != heat_map->width() || height != heat_map->height()) { // size changed
                heat_map->set(rgbData, width, height, channels);
            } else {
                heat_map->update(rgbData);
            }
        }
    }

    VisualizedObject::~VisualizedObject() {
        visualizable = nullptr;

        delete rgb_array;
        delete rgb_array_params;

        delete gray;
        delete gray_params;

        delete heat_map;
        delete heat_map_params;

        delete features_params;
        delete bar_chart_params;

        rgb_array = nullptr;
        rgb_array_params = nullptr;

        gray = nullptr;
        gray_params = nullptr;

        heat_map = nullptr;
        heat_map_params = nullptr;

        features_params = nullptr;

        delete m_lock;
        m_lock = nullptr;
    }

    void VisualizedObject::_init_rgb_array() {
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

        rgb_array = new GLTexture();

        // if (!SafeWrapper::execute([&]{
        //
        //     auto data = visualizable->getVisualization(VisualizationMethod::RGB_ARRAY, rgb_array_params->object);
        //     if (data.has_value()) {
        //         py::array_t<float> arr = data->cast<py::array>();
        //         if (arr.ndim() == 3 || arr.ndim() == 4) { // assuming RGB image / RGBA image
        //             int width = arr.shape(1);
        //             int height = arr.shape(0);
        //             int channels = arr.shape(2);
        //             //Logger::info(std::format("RGB Array Size: <{}, {}>", width, height));
        //             Logger::info("RGB Array Size: <" + std::to_string(width) + ", " + std::to_string(height) + ">");
        //             if (channels == 3 || channels == 4) {
        //                 std::vector<unsigned char>& rgbData = rgb_array_buffer.data;
        //                 rgb_array_buffer.width    = width;
        //                 rgb_array_buffer.height   = height;
        //                 rgb_array_buffer.channels = channels;
        //
        //                 auto data_ptr = arr.data();
        //                 for (int i = 0; i < width * height; i++) {
        //                     rgbData[i * channels + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
        //                     rgbData[i * channels + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 1] * 255.0f);
        //                     rgbData[i * channels + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 2] * 255.0f);
        //
        //                     if (channels == 4)
        //                         rgbData[i * channels + 3] = static_cast<unsigned char>(data_ptr[i * channels + 3] * 255.0f);
        //                 }
        //                 // rgb_array->set(rgbData, width, height, channels);
        //             } else {
        //                 Logger::error("Unsupported number of channels: " + std::to_string(channels));
        //             }
        //         } else {
        //             Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
        //         }
        //     } else {
        //         Logger::warning("No RGB Array visualization available for object: " + std::string(visualizable->moduleName));
        //     }
        // })) {
        //     Logger::error("Failed to initialize RGB Array visualization (buffer) for object: " + std::string(visualizable->moduleName));
        //     delete rgb_array_params;
        //     rgb_array_params = nullptr;
        //     delete rgb_array;
        //     rgb_array = nullptr;
        //     return;
        // }

        _update_rgb_array();
    }

    void VisualizedObject::_update_rgb_array() {
        SafeWrapper::execute([&]{
            auto data = visualizable->getVisualization(VisualizationMethod::RGB_ARRAY, rgb_array_params->object);
            if (data.has_value()) {
                py::array_t<float> arr = data->cast<py::array>();
                if (arr.ndim() == 3 || arr.ndim() == 4) { // assuming RGB image / RGBA image
                    int width = arr.shape(1);
                    int height = arr.shape(0);
                    int channels = arr.shape(2);
                    if (channels == 3 || channels == 4) {

                        std::lock_guard guard(*m_lock);
                        std::vector<unsigned char>& rgbData = rgb_array_buffer.data;
                        rgb_array_buffer.width    = width;
                        rgb_array_buffer.height   = height;
                        rgb_array_buffer.channels = channels;
                        rgbData.resize(width * height * channels);

                        auto data_ptr = arr.data();
                        for (int i = 0; i < width * height; i++) {
                            rgbData[i * channels + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                            rgbData[i * channels + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 1] * 255.0f);
                            rgbData[i * channels + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 2] * 255.0f);

                            if (channels == 4)
                                rgbData[i * channels + 3] = static_cast<unsigned char>(data_ptr[i * channels + 3] * 255.0f);
                        }

                        // {
                        //     if (width != rgb_array->width() || height != rgb_array->height()) { // size changed
                        //         rgb_array->set(rgbData, width, height, channels);
                        //         std::cout << "rgb - updated (size changed)" << std::endl;
                        //     } else {
                        //         rgb_array->update(rgbData);
                        //         std::cout << "rgb - updated (same size)" << std::endl;
                        //     }
                        // }
                    } else {
                        Logger::error("Unsupported number of channels: " + std::to_string(channels));
                    }
                } else {
                    Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
                }
            } else {
                Logger::warning("No RGB Array visualization available for object: " + std::string(visualizable->moduleName));
            }
        });
    }

    void VisualizedObject::_init_gray() {
        if (!SafeWrapper::execute([&]{
            auto type = visualizable->getVisualizationParamsType(VisualizationMethod::GRAY_SCALE);
            gray_params = new PyLiveObject();
            if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
                py::tuple args(0);
                gray_params->object = (*type)(*args);
                PyScope::parseLoadedModule(py::getattr(gray_params->object, "__class__"), *gray_params);
            } else {
                gray_params->object = py::none();
            }
        })) {
            Logger::error("Failed to initialize Gray Scale visualization (params) for object: " + std::string(visualizable->moduleName));
            delete gray_params;
            gray_params = nullptr;
            return;
        }

        gray = new GLTexture();

        // if (!SafeWrapper::execute([&]{
        //
        //     auto data = visualizable->getVisualization(VisualizationMethod::GRAY_SCALE, gray_params->object);
        //     if (data.has_value()) {
        //         py::array_t<float> arr = data->cast<py::array>();
        //         if (arr.ndim() == 2) { // assuming Gray image
        //             int width = arr.shape(1);
        //             int height = arr.shape(0);
        //             int channels = 1;
        //             //Logger::info(std::format("Gray Size: <{}, {}>", width, height));
        //             Logger::info("Gray Size: <" + std::to_string(width) + ", " + std::to_string(height) + ">");
        //
        //             std::vector<unsigned char> rgbData(width * height * 3);
        //             auto data_ptr = arr.data();
        //             for (int i = 0; i < width * height; i++) { // convert to RGB
        //                 rgbData[i * 3 + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
        //                 rgbData[i * 3 + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
        //                 rgbData[i * 3 + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
        //             }
        //             gray->set(rgbData, width, height, 3);
        //         } else {
        //             Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
        //         }
        //     } else {
        //         Logger::warning("No Gray Scale visualization available for object: " + std::string(visualizable->moduleName));
        //     } }))
        // {
        //     Logger::error("Failed to initialize Gray Scale visualization (buffer) for object: " + std::string(visualizable->moduleName));
        //     delete gray_params;
        //     gray_params = nullptr;
        //     delete gray;
        //     gray = nullptr;
        //     return;
        // }

        _update_gray();
    }

    void VisualizedObject::_update_gray() {
        SafeWrapper::execute([&]{
            auto data = visualizable->getVisualization(VisualizationMethod::GRAY_SCALE, gray_params->object);
            if (data.has_value()) {
                py::array_t<float> arr = data->cast<py::array>();
                if (arr.ndim() == 2) { // assuming Gray image

                    int width = arr.shape(1);
                    int height = arr.shape(0);
                    int channels = 1;

                    std::lock_guard guard(*m_lock);
                    std::vector<unsigned char>& rgbData = gray_buffer.data;
                    gray_buffer.width    = width;
                    gray_buffer.height   = height;
                    gray_buffer.channels = channels;
                    rgbData.resize(width * height * 3);

                    auto data_ptr = arr.data();
                    for (int i = 0; i < width * height; i++) { // convert to RGB
                        rgbData[i * 3 + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                        rgbData[i * 3 + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                        rgbData[i * 3 + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                    }

                    // {
                    //     std::lock_guard guard(*m_lock);
                    //     if (width != gray->width() || height != gray->height()) {
                    //         gray->set(rgbData, width, height, 3);
                    //     } else {
                    //         gray->update(rgbData);
                    //     }
                    // }

                } else {
                    Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
                }
            } else {
                Logger::warning("No Gray Scale visualization available for object: " + std::string(visualizable->moduleName));
            }
        });
    }

    void VisualizedObject::_init_heat_map() {
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

        heat_map = new GLTexture();

        // if (!SafeWrapper::execute([&]
        //                           {
        //
        //     auto data = visualizable->getVisualization(VisualizationMethod::HEAT_MAP, heat_map_params->object);
        //     if (data.has_value()) {
        //         py::array_t<float> arr = data->cast<py::array>();
        //         if (arr.ndim() == 2) { // assuming Gray image
        //             int width = arr.shape(1);
        //             int height = arr.shape(0);
        //             int channels = 1;
        //             //Logger::info(std::format("Heat map Size: <{}, {}>", width, height));
        //             Logger::info("Heat map Size: <" + std::to_string(width) + ", " + std::to_string(height) + ">");
        //
        //             std::vector<unsigned char> rgbData(width * height * 3);
        //             auto data_ptr = arr.data();
        //             auto max = *std::max_element(data_ptr, data_ptr + width * height);
        //             auto min = *std::min_element(data_ptr, data_ptr + width * height);
        //
        //             for (int i = 0; i < width * height; i++) { // convert to RGB
        //                 auto weight = (data_ptr[i * channels + 0] - min) / (max - min);
        //                 rgbData[i * 3 + 0]     = static_cast<unsigned char>(weight       * 255.0f);
        //                 rgbData[i * 3 + 1]     = static_cast<unsigned char>(0);
        //                 rgbData[i * 3 + 2]     = static_cast<unsigned char>((1 - weight) * 255.0f);
        //             }
        //             heat_map->set(rgbData, width, height, 3);
        //         } else {
        //             Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
        //         }
        //     } else {
        //         Logger::warning("No Heat map visualization available for object: " + std::string(visualizable->moduleName));
        //     } }))
        // {
        //     Logger::error("Failed to initialize Heat map visualization (buffer) for object: " + std::string(visualizable->moduleName));
        //     delete heat_map_params;
        //     heat_map_params = nullptr;
        //     delete heat_map;
        //     heat_map = nullptr;
        //     return;
        // }

        _update_heat_map();
    }

    void VisualizedObject::_update_heat_map() {
        SafeWrapper::execute([&]{
            auto data = visualizable->getVisualization(VisualizationMethod::HEAT_MAP, heat_map_params->object);
            if (data.has_value()) {
                py::array_t<float> arr = data->cast<py::array>();
                if (arr.ndim() == 2) { // assuming Gray image
                    int width = arr.shape(1);
                    int height = arr.shape(0);
                    int channels = 1;

                    std::lock_guard guard(*m_lock);
                    std::vector<unsigned char>& rgbData = heat_map_buffer.data;
                    heat_map_buffer.width    = width;
                    heat_map_buffer.height   = height;
                    heat_map_buffer.channels = channels;
                    rgbData.resize(width * height * 3);

                    auto data_ptr = arr.data();
                    auto max = *std::max_element(data_ptr, data_ptr + width * height);
                    auto min = *std::min_element(data_ptr, data_ptr + width * height);

                    for (int i = 0; i < width * height; i++) { // convert to RGB
                        auto weight = (data_ptr[i * channels + 0] - min) / (max - min);
                        rgbData[i * 3 + 0]     = static_cast<unsigned char>(weight       * 255.0f);
                        rgbData[i * 3 + 1]     = static_cast<unsigned char>(0);
                        rgbData[i * 3 + 2]     = static_cast<unsigned char>((1 - weight) * 255.0f);
                    }

                    // {
                    //     std::lock_guard guard(*m_lock);
                    //     if (width != heat_map->width() || height != heat_map->height()) {
                    //         heat_map->set(rgbData, width, height, 3);
                    //     } else {
                    //         heat_map->update(rgbData);
                    //     }
                    // }
                } else {
                    Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
                }
            } else {
                Logger::warning("No Heat map visualization available for object: " + std::string(visualizable->moduleName));
            }
        });
    }

    void VisualizedObject::_init_features() {
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

    void VisualizedObject::_update_features() {

        SafeWrapper::execute([&]{
            auto data = visualizable->getVisualization(VisualizationMethod::FEATURES, features_params->object);
            if (data.has_value()) {

                py::dict dict = data->cast<py::dict>();
                {
                    std::lock_guard guard(*m_lock);
                    features.clear();
                    for (const auto item : dict) {
                        py::str key = py::str(item.first);
                        py::str value = py::str(item.second);
                        features[key] = value;
                    }
                }
            } else {
                Logger::warning("No Features visualization available for object: " + std::string(visualizable->moduleName));
            }
        });
    }

    void VisualizedObject::_init_bar_chart() {
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

    void VisualizedObject::_update_bar_chart() {


        SafeWrapper::execute([&]{
            auto data = visualizable->getVisualization(VisualizationMethod::BAR_CHART, bar_chart_params->object);
            if (data.has_value()) {
                py::dict dict = data->cast<py::dict>();
                {
                    std::lock_guard guard(*m_lock);
                    bar_chart.clear();
                    for (const auto item : dict) {
                        py::str key = py::str(item.first);
                        try {
                            float value = py::cast<float>(item.second);
                            bar_chart[key] = value;
                        } catch (...) {
                            Logger::error("Failed to process bar chart entry: " + std::string(key));
                        }
                    }
                }

            } else {
                Logger::warning("No Bar Chart visualization available");
            }
        });
    }

    void VisualizedAgent::init(Pipeline::ActiveAgent *agent) {
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

    void VisualizedAgent::update() const {
        py::gil_scoped_acquire acquire{};

        if (env_visualization){
            env_visualization->update();
        }

        for (auto &method_vis : method_visualizations){
            method_vis->update();
        }
    }

    void VisualizedAgent::update_tex() const {
        if (env_visualization){
            env_visualization->update_tex();
        }

        for (auto &method_vis : method_visualizations){
            method_vis->update_tex();
        }
    }

    VisualizedAgent::~VisualizedAgent() {
        delete env_visualization;
        for (auto &method_vis : method_visualizations) {
            delete method_vis;
        }

        method_visualizations.clear();
        env_visualization = nullptr;
    }


}