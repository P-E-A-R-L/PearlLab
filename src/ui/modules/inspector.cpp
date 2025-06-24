#include "inspector.hpp"

#include "imgui.h"
#include "pipeline.hpp"
#include "preview.hpp"
#include "../utility/helpers.hpp"
#include "../widgets/splitter.hpp"

void Inspector::init() {

}

static void _render_python_object(PyLiveObject* obj) {
    std::lock_guard guard(*obj->m_lock);

    for (int i = 0; i < obj->annotations.size(); ++i) {
        auto& anno    = obj->annotations[i];
        auto& pub = obj->public_members_values[i];

        auto editable = anno.editable && pub.type != OTHER;

        ImGui::PushID(i);
        ImGui::BeginDisabled(editable == false);

            switch (pub.type) {
                case INTEGER: {
                    if (anno.hasChoices) {
                        auto data = Helpers::toImGuiList(anno.choices);
                        int choice = 0;
                        for (auto& c: anno.choices) {
                            if (c == std::to_string(*pub.value.i)) {
                                break;
                            }
                            choice++;
                        }

                        // there isn't anything better to do tbh ..
                        if (choice == anno.choices.size()) choice = 0;

                        if (ImGui::Combo("##<empty>", &choice, data.first, data.second)) {
                            try {
                                 (*pub.value.i) = std::stoi(anno.choices[choice]);
                            } catch (...) {
                                // no need to handle the error
                            }
                        }
                    } else {
                        int start = -1;
                        int end = -1;
                        bool ranged = false;
                        if (anno.rangeStart != "None" && anno.rangeEnd != "None") {
                            try {
                                start = std::stoi(anno.rangeStart);
                                end   = std::stoi(anno.rangeEnd);
                                ranged = true;
                            } catch (...) {
                                // no need to handle the error
                            }
                        }

                        if (ranged) {
                            ImGui::DragInt(anno.attrName.c_str(), pub.value.i, 1, start, end);
                        } else {
                            ImGui::InputInt(anno.attrName.c_str(), pub.value.i);
                        }
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text(anno.disc.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                    break;
                }

                case FLOAT: {
                    break;
                }

                case STRING: {
                    break;
                }

                default: ;
            }

        ImGui::EndDisabled();
        ImGui::PopID();
    }

    if (obj->annotations.size() == 0) {
        ImGui::TextDisabled("This object has no annotations");
    }
}

static void _render_visualizable(Pipeline::VisualizedObject* vis) {
    auto header_color = ImVec4{166 / 255.0f, 25 / 255.0f, 163 / 255.0f, 255 / 255.0f};
    ImGui::TextColored(header_color, "[Object]");
    _render_python_object(vis->visualizable);

    if (vis->rgb_array_params && vis->rgb_array_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::TextColored(header_color, "[RGB Params]");
        _render_python_object(vis->rgb_array_params);
    }

    if (vis->gray_params && vis->gray_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::TextColored(header_color, "[Gray Params]");
        _render_python_object(vis->gray_params);
    }

    if (vis->heat_map_params && vis->heat_map_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::TextColored(header_color, "[HeatMap Params]");
        _render_python_object(vis->heat_map_params);
    }

    if (vis->features_params && vis->features_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::TextColored(header_color, "[Features Params]");
        _render_python_object(vis->features_params);
    }

    if (vis->bar_chart_params && vis->bar_chart_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::TextColored(header_color, "[BarChart Params]");
        _render_python_object(vis->bar_chart_params);
    }
}

static void _render_info() {
    auto agent = Preview::curr_selected_preview;
    ImGui::TextColored(ImVec4{93 / 255.0f, 25 / 255.0f, 166 / 255.0f, 255 / 255.0f}, agent->agent->name);
    if (ImGui::BeginItemTooltip()) {
        ImGui::TextDisabled(agent->agent->agent->moduleName.c_str());
        ImGui::EndTooltip();
    }
    _render_python_object(agent->agent->agent);

    // TODO
    // add scores / final score etc ..
}

static void _render_control() {
    auto agent = Preview::curr_selected_preview;
    ImGui::TextDisabled("Control goes here");
    // TODO
    // add agent controls, step this only, select action etc ..
}

static void _render_env_details() {
    auto agent = Preview::curr_selected_preview;

    ImGui::TextColored(ImVec4{166 / 255.0f, 159 / 255.0f, 25 / 255.0f, 255 / 255.0f}, Pipeline::envs[Pipeline::PipelineConfig::activeEnv].acceptor->_tag);
    if (ImGui::BeginItemTooltip()) {
        ImGui::TextDisabled(agent->agent->env->moduleName.c_str());
        ImGui::EndTooltip();
    }
    _render_visualizable(agent->env_visualization);
}

static void _render_methods_details() {
    auto agent = Preview::curr_selected_preview;
    bool first = true;
    for (int i = 0; i < agent->method_visualizations.size(); ++i) {
        ImGui::PushID(i);
        if (auto& method = agent->method_visualizations[i]) {
            if (!first) {
                ImGui::Dummy({1, 5});
            }
            first = false;

            auto& pipMethod = Pipeline::PipelineConfig::pipelineMethods[i];
            ImGui::TextColored(ImVec4{31 / 255.0f, 171 / 255.0f, 73 / 255.0f, 255 / 255.0f}, pipMethod.name);
            if (ImGui::BeginItemTooltip()) {
                ImGui::TextDisabled(method->visualizable->moduleName.c_str());
                ImGui::EndTooltip();
            }
            _render_visualizable(method);
        }
        ImGui::PopID();
    }
}

static void _render_content() {
    static std::vector<float> sizes = {0.1, 0.3, 0.3, 0.3};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        Splitter::Vertical(sizes, [](int i) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
                if (i == 0) _render_info();
                if (i == 1) _render_control();
                if (i == 2) _render_env_details();
                if (i == 3) _render_methods_details();
            ImGui::PopStyleVar();
        });
    ImGui::PopStyleVar();
}

void Inspector::render() {
    if (!Pipeline::isExperimenting()) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::Begin("Inspector", nullptr);

    if (Preview::curr_selected_preview == nullptr) {
        std::string msg = "Select Agent to preview details";
        auto size = ImGui::GetContentRegionAvail();
        auto text_size = ImGui::CalcTextSize(msg.c_str());

        auto x = (size.x - text_size.x) / 2;
        auto y = (size.y - text_size.y) / 2;

        ImGui::SetCursorPos({x, y});
        ImGui::Text(msg.c_str());
    } else {
        _render_content();
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void Inspector::destroy() {

}
