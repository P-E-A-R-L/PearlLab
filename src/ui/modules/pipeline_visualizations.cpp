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
        this->visualizable->init_members();

        if (visualizable->supports(VisualizationMethod::RGB_ARRAY)) {
            // std::cout << "VisualizedObject::init::_init_rgb_array" << std::endl;
            _init_rgb_array();
        }

        if (visualizable->supports(VisualizationMethod::GRAY_SCALE)) {
            // std::cout << "VisualizedObject::init::_init_gray" << std::endl;
            _init_gray();
        }

        if (visualizable->supports(VisualizationMethod::HEAT_MAP)) {
            // std::cout << "VisualizedObject::init::_init_heat_map" << std::endl;
            _init_heat_map();
        }

        if (visualizable->supports(VisualizationMethod::FEATURES)) {
            // std::cout << "VisualizedObject::init::_init_features" << std::endl;
            _init_features();
        }

        if (visualizable->supports(VisualizationMethod::BAR_CHART)) {
            // std::cout << "VisualizedObject::init::_init_bar_chart" << std::endl;
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
        if (!visualizable) return;

        visualizable->update_members();

        if (rgb_array_params) rgb_array_params->update_members();
        if (gray_params)      gray_params->update_members();
        if (heat_map_params)  heat_map_params->update_members();
        if (features_params)  features_params->update_members();
        if (bar_chart_params) bar_chart_params->update_members();

        if (rgb_array)        _update_rgb_array();
        if (gray)             _update_gray();
        if (heat_map)         _update_heat_map();
        if (features_params)  _update_features();
        if (bar_chart_params) _update_bar_chart();

        if (rgb_array == nullptr        && visualizable->supports(VisualizationMethod::RGB_ARRAY))
            _init_rgb_array();
        if (gray == nullptr             && visualizable->supports(VisualizationMethod::GRAY_SCALE))
            _init_gray();
        if (heat_map == nullptr         && visualizable->supports(VisualizationMethod::HEAT_MAP))
            _init_heat_map();
        if (features_params == nullptr  && visualizable->supports(VisualizationMethod::FEATURES))
            _init_features();
        if (bar_chart_params == nullptr && visualizable->supports(VisualizationMethod::BAR_CHART))
            _init_bar_chart();
    }

    void VisualizedObject::update_tex() {
        if (rgb_array_params != nullptr && rgb_array_buffer.changed) {
            std::lock_guard guard(lock_rgb);
            auto& [rgbData, width, height, channels, valid, changed] = rgb_array_buffer;
            if (rgb_array == nullptr) {
                rgb_array        = new GLTexture();
                rgb_array_viewer = new ImageViewer(rgb_array->id(), {0, 0});
            }

            if (rgb_array_buffer.valid) {
                if (width != rgb_array->width() || height != rgb_array->height()) { // size changed
                    rgb_array->set(rgbData, width, height, channels);
                } else {
                    rgb_array->update(rgbData);
                }

                rgb_array_viewer->imageSize = ImVec2(rgb_array->width(), rgb_array->height());
                changed = false;
            }
        }

        if (gray_params != nullptr && gray_buffer.changed) {
            std::lock_guard guard(lock_gray);
            auto& [rgbData, width, height, channels, valid, changed] = gray_buffer;
            if (gray == nullptr) {
                gray        = new GLTexture();
                gray_viewer = new ImageViewer(gray->id(), {0, 0});
            }

            if (valid) {
                if (width != gray->width() || height != gray->height()) { // size changed
                    gray->set(rgbData, width, height, channels);
                } else {
                    gray->update(rgbData);
                }

                gray_viewer->imageSize = ImVec2(gray->width(), gray->height());
                changed = false;
            }
        }

        if (heat_map_params != nullptr && heat_map_buffer.changed) {
            std::lock_guard guard(lock_heat_map);
            auto& [rgbData, width, height, channels, valid, changed] = heat_map_buffer;
            if (heat_map == nullptr) {
                heat_map        = new GLTexture();
                heat_map_viewer = new ImageViewer(heat_map->id(), {0, 0});
            }

            if (valid) {
                if (width != heat_map->width() || height != heat_map->height()) { // size changed
                    heat_map->set(rgbData, width, height, channels);
                } else {
                    heat_map->update(rgbData);
                }

                heat_map_viewer->imageSize = ImVec2(heat_map->width(), heat_map->height());
                changed = false;
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

        delete rgb_array_viewer;
        delete gray_viewer;
        delete heat_map_viewer;

        rgb_array = nullptr;
        rgb_array_params = nullptr;

        gray = nullptr;
        gray_params = nullptr;

        heat_map = nullptr;
        heat_map_params = nullptr;

        features_params = nullptr;
    }

    void VisualizedObject::_init_rgb_array() {
        if (!SafeWrapper::execute([&]{
            // std::cout << "visualizable->getVisualizationParamsType(VisualizationMethod::RGB)" << std::endl;
            auto type = visualizable->getVisualizationParamsType(VisualizationMethod::RGB_ARRAY);
            // std::cout << "new PyLiveObject()" << std::endl;
            rgb_array_params = new PyLiveObject();
            // std::cout << "type != std::nullopt && !type->is_none() && py::hasattr(*type, \"__bases__\")" << std::endl;
            if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
                // std::cout << "create custom param" << std::endl;
                py::tuple args(0);
                rgb_array_params->object = (*type)(*args);

                // std::cout << "PyScope::parseLoadedModule" << std::endl;
                PyScope::parseLoadedModule(py::getattr(rgb_array_params->object, "__class__"), *rgb_array_params);
            } else {
                // std::cout << "None" << std::endl;
                rgb_array_params->object = py::none();
            }

            // std::cout << "init_members" << std::endl;
            rgb_array_params->init_members();
        })) {
            Logger::error("Failed to initialize RGB Array visualization (params) for object: " + std::string(visualizable->moduleName));
            delete rgb_array_params;
            rgb_array_params = nullptr;
            rgb_array_buffer.valid = false;
            return;
        }

        // rgb_array        = new GLTexture();
        // rgb_array_viewer = new ImageViewer(rgb_array->id(), {0, 0});

        // std::cout << "_update_rgb_array" << std::endl;
        _update_rgb_array();
    }

    void VisualizedObject::_update_rgb_array() {
        SafeWrapper::execute([&]{
            py::gil_scoped_acquire acquire;
            // std::cout << "visualizable->getVisualization(VisualizationMethod::RGB_ARRAY, rgb_array_params->object)" << std::endl;
            auto data = visualizable->getVisualization(VisualizationMethod::RGB_ARRAY, rgb_array_params->object);
            if (data.has_value() && !data->is_none() && py::isinstance<py::array>(*data)) {
                const auto& py_data = *data;
                // std::cout << std::string(py::str(py_data)) << std::endl;
                // std::cout << "data->cast<py::array>()" << std::endl;
                py::array_t<float> arr = py_data.cast<py::array>();
                // std::cout << "data->cast<py::array>() done" << std::endl;
                if (arr.ndim() == 3 || arr.ndim() == 4) { // assuming RGB image / RGBA image
                    int width    = arr.shape(1);
                    int height   = arr.shape(0);
                    int channels = arr.shape(2);
                    if (channels == 3 || channels == 4) {

                        std::lock_guard guard(lock_rgb);
                        std::vector<unsigned char>& rgbData = rgb_array_buffer.data;
                        rgb_array_buffer.width    = width;
                        rgb_array_buffer.height   = height;
                        rgb_array_buffer.channels = channels;
                        rgb_array_buffer.valid    = true;
                        rgb_array_buffer.changed  = true;
                        // std::cout << "rgbData.resize(width * height * channels)" << std::endl;
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
                        //         // std::cout << "rgb - updated (size changed)" << std::endl;
                        //     } else {
                        //         rgb_array->update(rgbData);
                        //         // std::cout << "rgb - updated (same size)" << std::endl;
                        //     }
                        // }
                    } else {
                        Logger::error("Unsupported number of channels: " + std::to_string(channels));
                        rgb_array_buffer.valid = false;
                    }
                } else {
                    Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
                    rgb_array_buffer.valid = false;
                }
            } else {
                Logger::warning("No RGB Array visualization available for object: " + std::string(visualizable->moduleName));
                rgb_array_buffer.valid = false;
            }
        });

        // std::cout << "VisualizedObject::_update_rgb_array done" << std::endl;
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

            gray_params->init_members();
        })) {
            Logger::error("Failed to initialize Gray Scale visualization (params) for object: " + std::string(visualizable->moduleName));
            delete gray_params;
            gray_params = nullptr;
            gray_buffer.valid = false;
            return;
        }

        // gray = new GLTexture();
        // gray_viewer = new ImageViewer(gray->id(), {0, 0});

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

                    std::lock_guard guard(lock_gray);
                    std::vector<unsigned char>& rgbData = gray_buffer.data;
                    gray_buffer.width    = width;
                    gray_buffer.height   = height;
                    gray_buffer.channels = 3;
                    gray_buffer.valid    = true;
                    gray_buffer.changed  = true;
                    rgbData.resize(width * height * 3);

                    auto data_ptr = arr.data();
                    for (int i = 0; i < width * height; i++) { // convert to RGB
                        rgbData[i * 3 + 0]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                        rgbData[i * 3 + 1]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                        rgbData[i * 3 + 2]     = static_cast<unsigned char>(data_ptr[i * channels + 0] * 255.0f);
                    }

                } else {
                    Logger::error("Unsupported visualization shape: " + std::to_string(arr.ndim()) + "D");
                    gray_buffer.valid = false;
                }
            } else {
                Logger::warning("No Gray Scale visualization available for object: " + std::string(visualizable->moduleName));
                gray_buffer.valid = false;
            }
        });
    }

    void VisualizedObject::_init_heat_map() {
        if (!SafeWrapper::execute([&]{
            auto type = visualizable->getVisualizationParamsType(VisualizationMethod::HEAT_MAP);
            heat_map_params = new PyLiveObject();
            if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
                py::tuple args(0);
                heat_map_params->object = (*type)(*args);
                PyScope::parseLoadedModule(py::getattr(heat_map_params->object, "__class__"), *heat_map_params);
            } else {
                heat_map_params->object = py::none();
            }
            heat_map_params->init_members();
        })) {
            Logger::error("Failed to initialize Heat map visualization (params) for object: " + std::string(visualizable->moduleName));
            delete heat_map_params;
            heat_map_params = nullptr;
            return;
        }

        // heat_map = new GLTexture();
        // heat_map_viewer = new ImageViewer(heat_map->id(), {0, 0});

        _update_heat_map();
    }

    void VisualizedObject::_update_heat_map() {
        SafeWrapper::execute([&]{
            auto data = visualizable->getVisualization(VisualizationMethod::HEAT_MAP, heat_map_params->object);
            if (data.has_value()) {
                py::array_t<float> arr = data->cast<py::array>();
                if (arr.ndim() == 2) { // assuming Gray image
                    int width  = arr.shape(1);
                    int height = arr.shape(0);
                    int channels = 1;

                    std::lock_guard guard(lock_heat_map);
                    std::vector<unsigned char>& rgbData = heat_map_buffer.data;
                    heat_map_buffer.width    = width;
                    heat_map_buffer.height   = height;
                    heat_map_buffer.channels = 3;
                    heat_map_buffer.valid    = true;
                    heat_map_buffer.changed  = true;
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
                    heat_map_buffer.valid    = false;
                }
            } else {
                Logger::warning("No Heat map visualization available for object: " + std::string(visualizable->moduleName));
                heat_map_buffer.valid    = false;
            }
        });
    }

    void VisualizedObject::_init_features() {
        if (!SafeWrapper::execute([&]{
            auto type = visualizable->getVisualizationParamsType(VisualizationMethod::FEATURES);
            features_params = new PyLiveObject();
            if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
                py::tuple args(0);
                features_params->object = (*type)(*args);
                PyScope::parseLoadedModule(py::getattr(features_params->object, "__class__"), *features_params);
            } else {
                features_params->object = py::none();
            }

            features_params->init_members();
        })) {
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
                    std::lock_guard guard(lock_features);
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
        if (!SafeWrapper::execute([&] {
            auto type = visualizable->getVisualizationParamsType(VisualizationMethod::BAR_CHART);
            bar_chart_params = new PyLiveObject();
            if (type != std::nullopt && !type->is_none() && py::hasattr(*type, "__bases__")) {
                py::tuple args(0);
                bar_chart_params->object = (*type)(*args);
                PyScope::parseLoadedModule(py::getattr(bar_chart_params->object, "__class__"), *bar_chart_params);
            } else {
                bar_chart_params->object = py::none();
            }

            bar_chart_params->init_members();
        })) {
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
                    std::lock_guard guard(lock_features);
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

    void VisualizedAgent::init(Pipeline::ActiveAgent *agent, int agent_index) {
        this->agent       = agent;
        this->agent_index = agent_index;

        // std::cout << "Init members" << std::endl;
        this->agent->agent->init_members();

        if (agent->env) {
            // std::cout << "Env visuals" << std::endl;
            env_visualization = new VisualizedObject();
            env_visualization->init(agent->env);
        }

        for (auto &method : agent->methods) {
            if (method) {
                // std::cout << "Method visuals" << std::endl;
                auto vis_obj = new VisualizedObject();
                vis_obj->init(method);
                method_visualizations.push_back(vis_obj);
            }
        }
    }

    void VisualizedAgent::update() const {
        py::gil_scoped_acquire acquire{};

        agent->agent->update_members();

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