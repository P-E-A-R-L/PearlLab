//
// Created by xabdomo on 6/18/25.
//

#ifndef PREVIEW_HPP
#define PREVIEW_HPP
#include <map>

#include "pipeline.hpp"
#include "../utility/gl_texture.hpp"


namespace Preview {
    struct VisualizedObject {
        PyVisualizable* visualizable;

        GLTexture* rgb_array           = nullptr;
        GLTexture* gray                = nullptr;
        GLTexture* heat_map            = nullptr;
        std::map<std::string, float>   features;

        PyLiveObject* rgb_array_params = nullptr;
        PyLiveObject* gray_params      = nullptr;
        PyLiveObject* heat_map_params  = nullptr;
        PyLiveObject* features_params  = nullptr;

        void init(PyVisualizable*);
        [[nodiscard]] bool supports(VisualizationMethod method) const;
        void update() const;

        ~VisualizedObject();

    private:
        void _init_rgb_array();
        void _update_rgb_array() const;
    };

    struct VisualizedAgent {
        Pipeline::ActiveAgent* agent;
        VisualizedObject* env_visualization = nullptr;
        std::vector<VisualizedObject*> method_visualizations;

        void init(Pipeline::ActiveAgent* agent);
        void update();

        ~VisualizedAgent();
    };

    extern std::vector<Preview::VisualizedAgent*> previews;

    void init();
    void render();
    void onStart();
    void onStop();
    void destroy();
};



#endif //PREVIEW_HPP
